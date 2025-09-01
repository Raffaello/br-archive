#include <lib_bra.h>
#include <bra_fs.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#define assert_bra_io_file_t(x) assert((x) != NULL && (x)->f != NULL && (x)->fn != NULL)

char    g_last_dir[UINT8_MAX];
uint8_t g_last_dir_size;

static inline uint64_t bra_min(const uint64_t a, const uint64_t b)
{
    return a < b ? a : b;
}

static inline bool bra_validate_meta_filename(const bra_meta_file_t* mf)
{
    // sanitize output path: reject absolute or parent traversal
    // POSIX absolute, Windows drive letter, and leading backslash
    if (mf->name[0] == '/' || mf->name[0] == '\\' ||
        (mf->name_size >= 2 &&
         ((mf->name[1] == ':' &&
           ((mf->name[0] >= 'A' && mf->name[0] <= 'Z') ||
            (mf->name[0] >= 'a' && mf->name[0] <= 'z'))))))
    {
        printf("ERROR: absolute output path: %s\n", mf->name);
        return false;
    }
    // 2.2 Reject common traversal patterns
    if (strstr(mf->name, "/../") != NULL || strstr(mf->name, "\\..\\") != NULL ||
        strncmp(mf->name, "../", 3) == 0 || strncmp(mf->name, "..\\", 3) == 0)
    {
        printf("ERROR: invalid output path (contains '..'): %s\n", mf->name);
        return false;
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////


char* bra_strdup(const char* str)
{
    if (str == NULL)
        return NULL;

    const size_t sz = strlen(str) + 1;
    char*        c  = malloc(sz);

    if (c == NULL)
        return NULL;

    memcpy(c, str, sizeof(char) * sz);
    return c;
}

void bra_io_read_error(bra_io_file_t* bf)
{
    printf("ERROR: unable to read %s %s file\n", bf->fn, BRA_NAME);
    bra_io_close(bf);
}

bool bra_io_open(bra_io_file_t* bf, const char* fn, const char* mode)
{
    if (fn == NULL || bf == NULL || mode == NULL)
        return false;

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

void bra_io_close(bra_io_file_t* bf)
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

bool bra_io_seek(bra_io_file_t* f, const int64_t offs, const int origin)
{
    assert_bra_io_file_t(f);

    // return fseek(f->f, offs, origin) == 0;

#if defined(_WIN32) || defined(_WIN64)
    return _fseeki64(f->f, offs, origin) == 0;
#elif defined(__APPLE__) || defined(__linux__) || defined(__linux)
    return fseeko(f->f, offs, origin) == 0;
#else
#error "not supported"
#endif
}

int64_t bra_io_tell(bra_io_file_t* f)
{
    assert_bra_io_file_t(f);

    // return ftell(f->f);

#if defined(_WIN32) || defined(_WIN64)
    return _ftelli64(f->f);
#elif defined(__APPLE__) || defined(__linux__) || defined(__linux)
    return ftello(f->f);
#else
#error "not supported"
#endif
}

bool bra_io_read_header(bra_io_file_t* bf, bra_io_header_t* out_bh)
{
    assert_bra_io_file_t(bf);
    assert(out_bh != NULL);

    // Good point to clean the variable, even if it is not needed.
    memset(g_last_dir, 0, sizeof(g_last_dir));

    if (fread(out_bh, sizeof(bra_io_header_t), 1, bf->f) != 1)
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

bool bra_io_write_header(bra_io_file_t* f, const uint32_t num_files)
{
    assert_bra_io_file_t(f);

    const bra_io_header_t header = {
        .magic = BRA_MAGIC,
        // .version   = BRA_ARCHIVE_VERSION,
        .num_files = num_files,
    };

    if (fwrite(&header, sizeof(bra_io_header_t), 1, f->f) != 1)
    {
        printf("ERROR: unable to write %s %s file\n", f->fn, BRA_NAME);
        bra_io_close(f);
        return false;
    }

    return true;
}

bool bra_io_read_footer(bra_io_file_t* f, bra_io_footer_t* bf_out)
{
    assert_bra_io_file_t(f);
    assert(bf_out != NULL);

    memset(bf_out, 0, sizeof(bra_io_footer_t));
    if (fread(bf_out, sizeof(bra_io_footer_t), 1, f->f) != 1)
    {
        bra_io_read_error(f);
        return false;
    }

    // check footer magic
    if (bf_out->magic != BRA_FOOTER_MAGIC)
    {
        printf("ERROR: corrupted or not valid %s-SFX file: %s\n", BRA_NAME, f->fn);
        bra_io_close(f);
        return false;
    }

    return true;
}

bool bra_io_write_footer(bra_io_file_t* f, const int64_t header_offset)
{
    assert_bra_io_file_t(f);
    assert(header_offset > 0);

    bra_io_footer_t bf = {
        .magic         = BRA_FOOTER_MAGIC,
        .header_offset = header_offset,
    };

    if (fwrite(&bf, sizeof(bra_io_footer_t), 1, f->f) != 1)
    {
        printf("ERROR: unable to write footer in %s.\n", f->fn);
        bra_io_close(f);
        return false;
    }

    return true;
}

bool bra_io_read_meta_file(bra_io_file_t* f, bra_meta_file_t* mf)
{
    assert_bra_io_file_t(f);
    assert(mf != NULL);

    // TODO: load the filename and filesize
    // in the buffer first
    // then adjust it if needed for files with g_last_dir
    // then copy those values into mf
    char    buf[UINT8_MAX];
    uint8_t buf_size = 0;

    mf->name      = NULL;
    mf->name_size = 0;
    mf->data_size = 0;

    // 1. attributes
    if (fread(&mf->attributes, sizeof(uint8_t), 1, f->f) != 1)
    {
    BRA_IO_READ_ERR:
        bra_io_read_error(f);
        return false;
    }

    switch (mf->attributes)
    {
    case BRA_ATTR_FILE:
        // [[fallthrough]];
    case BRA_ATTR_DIR:
        break;
    default:
        goto BRA_IO_READ_ERR;
    }

    // 2. filename size
    if (fread(&buf_size, sizeof(uint8_t), 1, f->f) != 1 || buf_size == 0)
        goto BRA_IO_READ_ERR;

    // 3. filename
    if (fread(buf, sizeof(char), buf_size, f->f) != buf_size)
    {
    BRA_IO_READ_ERR_MF:
        bra_meta_file_free(mf);
        goto BRA_IO_READ_ERR;
    }

    buf[buf_size] = '\0';

    // 4. data size
    // NOTE: for directory not saving data_size, nor data,
    //       unless data_size will be valuable for specific directory info
    if (mf->attributes == BRA_ATTR_DIR)
    {
        strncpy(g_last_dir, buf, buf_size);
        g_last_dir_size = buf_size;

        mf->name_size = buf_size;
        mf->name      = malloc(sizeof(char) * (mf->name_size + 1));    // !< one extra for '\0'
        if (mf->name == NULL)
            goto BRA_IO_READ_ERR;

        strncpy(mf->name, buf, mf->name_size);
    }
    else if (mf->attributes == BRA_ATTR_FILE)
    {
        if (fread(&mf->data_size, sizeof(uint64_t), 1, f->f) != 1)
            goto BRA_IO_READ_ERR_MF;

        mf->name_size = g_last_dir_size + buf_size;
        if (g_last_dir_size > 0)
            ++mf->name_size;                                      // one extra for '/'

        mf->name = malloc(sizeof(char) * (mf->name_size + 1));    // !< one extra for '\0'
        if (mf->name == NULL)
            goto BRA_IO_READ_ERR;

        strncpy(mf->name, g_last_dir, g_last_dir_size);
        char* b = NULL;
        if (g_last_dir_size > 0)
        {
            mf->name[g_last_dir_size] = '/';
            // strncpy(&mf->name[g_last_dir_size + 1], buf, buf_size);
            b = &mf->name[g_last_dir_size + 1];
        }
        else
        {
            // strncpy(mf->name, buf, buf_size);
            b = mf->name;
        }
        strncpy(b, buf, buf_size);
    }

    mf->name[mf->name_size] = '\0';
    return true;
}

bool bra_io_write_meta_file(bra_io_file_t* f, const bra_meta_file_t* mf)
{
    assert_bra_io_file_t(f);
    assert(mf != NULL);
    assert(mf->name != NULL);

    char    buf[UINT8_MAX];
    uint8_t buf_size;

    const size_t len = strnlen(mf->name, UINT8_MAX + 1);
    if (len != mf->name_size || len == 0 || len > UINT8_MAX)
        goto BRA_IO_WRITE_ERR;

    // Processing data
    switch (mf->attributes)
    {
    case BRA_ATTR_FILE:
    {
        if (g_last_dir_size >= mf->name_size)
            goto BRA_IO_WRITE_ERR;

        if (strncmp(mf->name, g_last_dir, g_last_dir_size) != 0)    // g_last_dir doesn't have '/'
            goto BRA_IO_WRITE_ERR;

        size_t l = g_last_dir_size;    // strnlen(g_last_dir, UINT8_MAX);
        if (mf->name[l] == '/')        // or when l == 0
            ++l;                       // skip also '/'

        buf_size = mf->name_size - l;
        strncpy(buf, &mf->name[l], buf_size);
        break;
    }
    case BRA_ATTR_DIR:
    {
        if (mf->data_size != 0)
            goto BRA_IO_WRITE_ERR;

        buf_size = mf->name_size;
        strncpy(buf, mf->name, buf_size);
        strncpy(g_last_dir, buf, buf_size);
    }
    break;
    default:
        goto BRA_IO_WRITE_ERR;
    }

    // 1. attributes
    if (fwrite(&mf->attributes, sizeof(uint8_t), 1, f->f) != 1)
    {
    BRA_IO_WRITE_ERR:
        bra_io_close(f);
        printf("ERROR: Writing file: %s\n", mf->name);
        return false;
    }

    // 2. filename size
    if (fwrite(&buf_size, sizeof(uint8_t), 1, f->f) != 1)
        goto BRA_IO_WRITE_ERR;

    // 3. filename
    if (fwrite(buf, sizeof(char), mf->name_size, f->f) != mf->name_size)
        goto BRA_IO_WRITE_ERR;

    // 4. data size
    // NOTE: for directory makes sense to be zero, but it could be used for something else.
    //       actually for directory would be better not saving it at all if it is always zero.
    if (mf->attributes == BRA_ATTR_FILE)
    {
        if (fwrite(&mf->data_size, sizeof(uint64_t), 1, f->f) != 1)
            goto BRA_IO_WRITE_ERR;
    }

    return true;
}

void bra_meta_file_free(bra_meta_file_t* mf)
{
    assert(mf != NULL);

    mf->data_size  = 0;
    mf->name_size  = 0;
    mf->attributes = 0;
    if (mf->name != NULL)
    {
        free(mf->name);
        mf->name = NULL;
    }
}

bool bra_io_copy_file_chunks(bra_io_file_t* dst, bra_io_file_t* src, const uint64_t data_size)
{
    assert_bra_io_file_t(dst);
    assert_bra_io_file_t(src);

    char buf[MAX_CHUNK_SIZE];

    for (uint64_t i = 0; i < data_size;)
    {
        uint32_t s = bra_min(MAX_CHUNK_SIZE, data_size - i);

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

bool bra_io_skip_data(bra_io_file_t* f, const uint64_t data_size)
{
    assert_bra_io_file_t(f);

    return bra_io_seek(f, data_size, SEEK_CUR);
}

bool bra_io_decode_and_write_to_disk(bra_io_file_t* f)
{
    assert_bra_io_file_t(f);

    bra_meta_file_t mf;
    if (!bra_io_read_meta_file(f, &mf))
        return false;

    if (!bra_validate_meta_filename(&mf))
    {
    BRA_IO_READ_ERR:
        bra_meta_file_free(&mf);
        bra_io_read_error(f);
        return false;
    }

    // 4. read and write in chunk data
    // NOTE: nothing to extract for a directory, but only to create it
    switch (mf.attributes)
    {
    case BRA_ATTR_FILE:
    {
        printf("Extracting file: %s ...", mf.name);

        bra_io_file_t f2;
        if (!bra_io_open(&f2, mf.name, "wb"))
        {
            printf("ERROR: unable to write file: %s\n", mf.name);
            goto BRA_IO_READ_ERR;
        }

        const uint64_t ds = mf.data_size;
        bra_meta_file_free(&mf);
        if (!bra_io_copy_file_chunks(&f2, f, ds))
            return false;

        bra_io_close(&f2);
    }
    break;
    case BRA_ATTR_DIR:
    {
        printf("Creating dir: %s", mf.name);
        if (!bra_fs_dir_make(mf.name))
            goto BRA_IO_READ_ERR;

        bra_meta_file_free(&mf);
    }
    break;
    default:
        goto BRA_IO_READ_ERR;
        break;
    }

    printf("OK\n");
    return true;
}
