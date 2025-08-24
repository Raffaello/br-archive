#include <lib_bra.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>

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

static void bra_io_read_error(bra_file_t* bf)
{
    printf("ERROR: unable to read %s %s file\n", bf->fn, BRA_NAME);
    bra_io_close(bf);
}

static inline uintmax_t min(const uintmax_t a, const uintmax_t b)
{
    return a < b ? a : b;
}

bool bra_io_open(bra_file_t* bf, const char* fn, const char* mode)
{
    assert(bf != NULL);
    assert(fn != NULL);
    assert(mode != NULL);

    bf->fn = NULL;
    bf->f  = fopen(fn, mode);
    if (bf->f == NULL)
    {
    BRA_IO_OPEN_ERROR:
        printf("unable to open file %s\n", fn);
        return false;
    }

    // copy filename
    bf->fn = bra_strdup(fn);
    if (bf->fn == NULL)
    {
        bra_io_close(bf);
        goto BRA_IO_OPEN_ERROR;
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

bool bra_io_read_header(bra_file_t* bf, bra_header_t* out_bh)
{
    assert(bf != NULL);
    assert(bf->f != NULL);
    assert(bf->fn != NULL);
    assert(out_bh != NULL);

    if (fread(out_bh, sizeof(bra_header_t), 1, bf->f) != 1)
    {
        bra_io_read_error(bf);
        return false;
    }

    // check header magic
    if (out_bh->magic != BRA_MAGIC)
    {
        printf("ERROR: Not valid %s file\n", BRA_FILE_EXT);
        bra_io_close(bf);
        return false;
    }

    return true;
}

bool bra_io_write_header(bra_file_t* bf, const uint32_t num_files)
{
    assert(bf != NULL);
    assert(bf->f != NULL);
    assert(bf->fn != NULL);

    const bra_header_t header = {
        .magic     = BRA_MAGIC,
        .num_files = num_files,
    };

    if (fwrite(&header, sizeof(bra_header_t), 1, bf->f) != 1)
    {
        printf("unable to write %s %s file\n", bf->fn, BRA_NAME);
        bra_io_close(bf);
        return false;
    }

    return true;
}

bool bra_io_read_footer(bra_file_t* f, bra_footer_t* bf_out)
{
    assert(f != NULL);
    assert(f->f != NULL);
    assert(f->fn != NULL);
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

bool bra_io_write_footer(bra_file_t* f, const unsigned long data_offset)
{
    assert(f != NULL);
    assert(f->f != NULL);
    assert(f->fn != NULL);
    assert(data_offset > 0);

    bra_footer_t bf = {
        .magic       = BRA_FOOTER_MAGIC,
        .data_offset = data_offset,
    };

    if (fwrite(&bf, sizeof(bra_footer_t), 1, f->f) != 1)
    {
        printf("ERROR: unable to write footer in %s.\n", f->fn);
        return false;
    }

    return true;
}

bool bra_io_copy_file_chunks(bra_file_t* dst, bra_file_t* src, const uintmax_t data_size)
{
    char buf[MAX_BUF_SIZE];

    for (uintmax_t i = 0; i < data_size;)
    {
        uint32_t s = min(MAX_BUF_SIZE, data_size - i);

        // read source chunk
        if (fread(buf, sizeof(char), s, src->f) != s)
        {
            printf("ERROR: unable to read %s file\n", src->fn);
            return false;
        }

        // write source chunk
        if (fwrite(buf, sizeof(char), s, dst->f) != s)
        {
            printf("ERROR: writing to file: %s\n", dst->fn);
            return false;
        }

        i += s;
    }

    return true;
}

bool bra_io_decode_and_write_to_disk(bra_file_t* f)
{
    assert(f != NULL);
    assert(f->f != NULL);
    assert(f->fn != NULL);

    char buf[MAX_BUF_SIZE];
    // 1. filename size
    uint8_t fn_size = 0;
    if (fread(&fn_size, sizeof(uint8_t), 1, f->f) != 1)
    {
    BRA_IO_READ_ERR:
        bra_io_read_error(f);
        return false;
    }

    // 2. filename
    if (fread(buf, sizeof(uint8_t), fn_size, f->f) != fn_size)
        goto BRA_IO_READ_ERR;

    buf[fn_size] = '\0';

    char* out_fn = bra_strdup(buf);
    if (out_fn == NULL)
        goto BRA_IO_READ_ERR;

    // 3. data size
    uintmax_t ds = 0;
    if (fread(&ds, sizeof(uintmax_t), 1, f->f) != 1)
    {
    BRA_IO_READ_ERR_DUP:
        free(out_fn);
        goto BRA_IO_READ_ERR;
    }

    // 4. read and write in chunk data
    printf("ERROR: Extracting file: %s ...\n", out_fn);
    bra_file_t f2;
    if (!bra_io_open(&f2, out_fn, "wb"))
    {
        printf("ERROR: unable to write file: %s\n", out_fn);
        goto BRA_IO_READ_ERR_DUP;
    }

    if (!bra_io_copy_file_chunks(&f2, f, ds))
    {
        bra_io_close(f);
        bra_io_close(&f2);
        free(out_fn);
        return false;
    }

    bra_io_close(&f2);
    free(out_fn);
    printf("OK\n");
    return true;
}
