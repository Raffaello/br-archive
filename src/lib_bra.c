#include <lib_bra.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#define assert_bra_file_t(x) assert((x) != NULL && (x)->f != NULL && (x)->fn != NULL)

static inline uint64_t bra_min(const uint64_t a, const uint64_t b)
{
    return a < b ? a : b;
}

/////////////////////////////////////////////////////////////////////////////


/**
 * @brief strdup()
 * @todo remove when switching to C23
 *
 * @param str
 * @return char*
 */
static char* bra_strdup(const char* str)
{
    const size_t sz = strlen(str) + 1;
    char*        c  = malloc(sz);

    if (c == NULL)
        return NULL;

    memcpy(c, str, sizeof(char) * sz);
    return c;
}

void bra_io_read_error(bra_file_t* bf)
{
    printf("ERROR: unable to read %s %s file\n", bf->fn, BRA_NAME);
    bra_io_close(bf);
}

bool bra_io_open(bra_file_t* bf, const char* fn, const char* mode)
{
    assert(bf != NULL);
    assert(fn != NULL);
    assert(mode != NULL);

    bf->f  = fopen(fn, mode);    // open file
    bf->fn = bra_strdup(fn);     // copy filename
    if (bf->f == NULL || bf->fn == NULL)
    {
        printf("ERROR: unable to open file %s\n", fn);
        bra_io_close(bf);
        return false;
    }

    return true;
}

void bra_io_close(bra_file_t* bf)
{
    assert(bf != NULL);

    if (bf->fn != NULL)
    {
        free(bf->fn);
        bf->fn = NULL;
    }

    if (bf->f != NULL)
    {
        fclose(bf->f);
        bf->f = NULL;
    }
}

bool bra_io_seek(bra_file_t* f, const int64_t offs, const int origin)
{
    assert_bra_file_t(f);

    return fseeko64(f->f, offs, origin) == 0;
}

int64_t bra_io_tell(bra_file_t* f)
{
    assert_bra_file_t(f);

    return ftello64(f->f);
}

bool bra_io_read_header(bra_file_t* bf, bra_header_t* out_bh)
{
    assert_bra_file_t(bf);
    assert(out_bh != NULL);

    if (fread(out_bh, sizeof(bra_header_t), 1, bf->f) != 1)
    {
        bra_io_read_error(bf);
        return false;
    }

    // check header magic
    if (out_bh->magic != BRA_MAGIC)
    {
        printf("ERROR: Not valid %s file\n", BRA_NAME);
        bra_io_close(bf);
        return false;
    }

    return true;
}

bool bra_io_write_header(bra_file_t* bf, const uint32_t num_files)
{
    assert_bra_file_t(bf);

    const bra_header_t header = {
        .magic     = BRA_MAGIC,
        .num_files = num_files,
    };

    if (fwrite(&header, sizeof(bra_header_t), 1, bf->f) != 1)
    {
        printf("ERROR: unable to write %s %s file\n", bf->fn, BRA_NAME);
        bra_io_close(bf);
        return false;
    }

    return true;
}

bool bra_io_read_footer(bra_file_t* f, bra_footer_t* bf_out)
{
    assert_bra_file_t(f);
    assert(bf_out != NULL);

    memset(bf_out, 0, sizeof(bra_footer_t));
    if (fread(bf_out, sizeof(bra_footer_t), 1, f->f) != 1)
    {
        bra_io_read_error(f);
        return false;
    }

    // check footer magic
    if (bf_out->magic != BRA_FOOTER_MAGIC)
    {
        printf("ERROR: corrupted or not valid BRA-SFX file: %s\n", f->fn);
        bra_io_close(f);
        return false;
    }

    return true;
}

bool bra_io_write_footer(bra_file_t* f, const int64_t data_offset)
{
    assert_bra_file_t(f);
    assert(data_offset > 0);

    bra_footer_t bf = {
        .magic       = BRA_FOOTER_MAGIC,
        .data_offset = data_offset,
    };

    if (fwrite(&bf, sizeof(bra_footer_t), 1, f->f) != 1)
    {
        printf("ERROR: unable to write footer in %s.\n", f->fn);
        bra_io_close(f);
        return false;
    }

    return true;
}

bool bra_io_copy_file_chunks(bra_file_t* dst, bra_file_t* src, const uint64_t data_size)
{
    assert_bra_file_t(dst);
    assert_bra_file_t(src);

    char buf[MAX_BUF_SIZE];

    for (uint64_t i = 0; i < data_size;)
    {
        uint32_t s = bra_min(MAX_BUF_SIZE, data_size - i);

        // read source chunk
        if (fread(buf, sizeof(char), s, src->f) != s)
        {
            printf("ERROR: unable to read %s file\n", src->fn);
            bra_io_close(dst);
            bra_io_close(src);
            return false;
        }

        // write source chunk
        if (fwrite(buf, sizeof(char), s, dst->f) != s)
        {
            printf("ERROR: writing to file: %s\n", dst->fn);
            bra_io_close(dst);
            bra_io_close(src);
            return false;
        }

        i += s;
    }

    return true;
}

bool bra_io_decode_and_write_to_disk(bra_file_t* f)
{
    assert_bra_file_t(f);

    char out_fn[UINT8_MAX + 1];

    // 1. filename size
    uint8_t fn_size = 0;
    if (fread(&fn_size, sizeof(uint8_t), 1, f->f) != 1)
    {
    BRA_IO_READ_ERR:
        bra_io_read_error(f);
        return false;
    }

    // 2. filename
    if (fread(out_fn, sizeof(uint8_t), fn_size, f->f) != fn_size)
        goto BRA_IO_READ_ERR;

    out_fn[fn_size] = '\0';

    // 2.1 sanitize output path: reject absolute or parent traversal
    //     POSIX absolute, Windows drive letter, and leading backslash
    if (out_fn[0] == '/' || out_fn[0] == '\\' ||
        (fn_size >= 2 && ((out_fn[1] == ':' && ((out_fn[0] >= 'A' && out_fn[0] <= 'Z') || (out_fn[0] >= 'a' && out_fn[0] <= 'z'))))))
    {
        printf("ERROR: absolute output path: %s\n", out_fn);
        goto BRA_IO_READ_ERR;
    }
    // Reject common traversal patterns
    if (strstr(out_fn, "/../") != NULL || strstr(out_fn, "\\..\\") != NULL ||
        strncmp(out_fn, "../", 3) == 0 || strncmp(out_fn, "..\\", 3) == 0)
    {
        printf("ERROR: invalid output path (contains '..'): %s\n", out_fn);
        goto BRA_IO_READ_ERR;
    }

    // 3. data size
    uint64_t ds = 0;
    if (fread(&ds, sizeof(uint64_t), 1, f->f) != 1)
        goto BRA_IO_READ_ERR;

    // 4. read and write in chunk data
    printf("Extracting file: %s ...", out_fn);
    bra_file_t f2;
    if (!bra_io_open(&f2, out_fn, "wb"))
    {
        printf("ERROR: unable to write file: %s\n", out_fn);
        goto BRA_IO_READ_ERR;
    }

    if (!bra_io_copy_file_chunks(&f2, f, ds))
        return false;

    bra_io_close(&f2);
    printf("OK\n");
    return true;
}
