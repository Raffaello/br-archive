#include <io/lib_bra_io_file.h>

#include <lib_bra.h>
#include <lib_bra_private.h>

#include <fs/bra_fs_c.h>
#include <log/bra_log.h>

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

_Static_assert(BRA_MAX_PATH_LENGTH > UINT8_MAX, "BRA_MAX_PATH_LENGTH must be greater than bra_meta_file_t.name_size max value");

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief the last encoded or decoded directory.
 *
 * @note To understand if the next dir is a sub-dir or a sibling dir
 *       there is always the full relative path.
 *
 * @todo the sub-dir though must be processed like the file contained in a directory
 *
 * @bug  having a global variable can't be thread safe
 */
static char         g_last_dir[BRA_MAX_PATH_LENGTH];
static unsigned int g_last_dir_size;
static const char   g_end_messages[][5] = {" OK ", "SKIP"};

void bra_io_file_error(bra_io_file_t* bf, const char* verb)
{
    assert(bf != NULL);
    assert(verb != NULL);

    const char* fn = (bf->fn != NULL) ? bf->fn : "N/A";
    bra_log_error("unable to %s %s file (errno %d: %s)", verb, fn, errno, strerror(errno));
    bra_io_file_close(bf);
}

inline void bra_io_file_open_error(bra_io_file_t* bf)
{
    bra_io_file_error(bf, "open");
}

inline void bra_io_file_read_error(bra_io_file_t* bf)
{
    bra_io_file_error(bf, "read");
}

inline void bra_io_file_seek_error(bra_io_file_t* bf)
{
    bra_io_file_error(bf, "seek");
}

inline void bra_io_file_write_error(bra_io_file_t* bf)
{
    bra_io_file_error(bf, "write");
}

bool bra_io_file_is_elf(const char* fn)
{
    bra_io_file_t f;

    if (!bra_io_file_open(&f, fn, "rb"))
        return false;

    // 0x7F,'E','L','F'
    const size_t MAGIC_SIZE = 4;
    char         magic[MAGIC_SIZE];
    if (fread(magic, sizeof(char), MAGIC_SIZE, f.f) != MAGIC_SIZE)
    {
        bra_io_file_read_error(&f);
        return false;
    }

    bra_io_file_close(&f);
    const bool ret = magic[0] == 0x7F && magic[1] == 'E' && magic[2] == 'L' && magic[3] == 'F';
    if (ret)
        bra_log_info("ELF file detected");

    return ret;
}

bool bra_io_file_is_pe_exe(const char* fn)
{
    bra_io_file_t f;
    if (!bra_io_file_open(&f, fn, "rb"))
        return false;

    // 'M' 'Z'
    const size_t MAGIC_SIZE = 2;
    char         magic[MAGIC_SIZE];
    if (fread(magic, sizeof(char), MAGIC_SIZE, f.f) != MAGIC_SIZE)
    {
    BRA_IS_EXE_ERROR:
        bra_io_file_read_error(&f);
        return false;
    }

    if (magic[0] != 'M' || magic[1] != 'Z')
    {
        bra_io_file_close(&f);
        return false;
    }

    // Read the PE header offset from the DOS header
    if (!bra_io_file_seek(&f, 0x3C, SEEK_SET))
    {
    BRA_IS_EXE_SEEK_ERROR:
        bra_io_file_seek_error(&f);
        return false;
    }

    uint32_t pe_offset;
    if (fread(&pe_offset, sizeof(uint32_t), 1, f.f) != 1)
        goto BRA_IS_EXE_ERROR;

    if (!bra_io_file_seek(&f, pe_offset, SEEK_SET))
        goto BRA_IS_EXE_SEEK_ERROR;

    const size_t PE_MAGIC_SIZE = 4;
    char         pe_magic[PE_MAGIC_SIZE];
    if (fread(pe_magic, sizeof(char), PE_MAGIC_SIZE, f.f) != PE_MAGIC_SIZE)
        goto BRA_IS_EXE_ERROR;

    bra_io_file_close(&f);
    const bool ret = pe_magic[0] == 'P' && pe_magic[1] == 'E' && pe_magic[2] == '\0' && pe_magic[3] == '\0';
    if (ret)
        bra_log_info("PE/EXE file detected");

    return ret;
}

bool bra_io_file_is_sfx(const char* fn)
{
    return bra_io_file_is_elf(fn) || bra_io_file_is_pe_exe(fn);
}

bool bra_io_file_open(bra_io_file_t* bf, const char* fn, const char* mode)
{
    if (fn == NULL || bf == NULL || mode == NULL)
        return false;

    bf->f  = fopen(fn, mode);    // open file
    bf->fn = bra_strdup(fn);     // copy filename
    if (bf->f == NULL || bf->fn == NULL)
    {
        bra_io_file_open_error(bf);
        return false;
    }

    return true;
}

void bra_io_file_close(bra_io_file_t* bf)
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

bool bra_io_file_sfx_open(bra_io_file_t* f, const char* fn, const char* mode)
{
    if (!bra_io_file_open(f, fn, mode))
        return false;

    // check if it can have the footer
    if (!bra_io_file_seek(f, 0, SEEK_END))
    {
        bra_io_file_seek_error(f);
        return false;
    }
    if (bra_io_file_tell(f) < (int64_t) sizeof(bra_io_footer_t))
    {
        bra_log_error("%s-SFX module too small (missing footer placeholder): %s", BRA_NAME, f->fn);
        bra_io_file_close(f);
        return false;
    }

    // Position at the footer start
    if (!bra_io_file_seek(f, -1L * (int64_t) sizeof(bra_io_footer_t), SEEK_END))
    {
        bra_io_file_seek_error(f);
        return false;
    }

    return true;
}

bool bra_io_file_seek(bra_io_file_t* f, const int64_t offs, const int origin)
{
    assert_bra_io_file_t(f);

    // return fseek(f->f, offs, origin) == 0;

#if defined(_WIN32) || defined(_WIN64)
    return _fseeki64(f->f, offs, origin) == 0;
#elif defined(__APPLE__) || defined(__linux__) || defined(__unix__)
    return fseeko(f->f, offs, origin) == 0;
#else
#error "not supported"
#endif
}

int64_t bra_io_file_tell(bra_io_file_t* f)
{
    assert_bra_io_file_t(f);

    // return ftell(f->f);

#if defined(_WIN32) || defined(_WIN64)
    return _ftelli64(f->f);
#elif defined(__APPLE__) || defined(__linux__) || defined(__unix__)
    return ftello(f->f);
#else
#error "not supported"
#endif
}

bool bra_io_file_read_header(bra_io_file_t* bf, bra_io_header_t* out_bh)
{
    assert_bra_io_file_t(bf);
    assert(out_bh != NULL);

    // Good point to clean the variable, even if it is not needed.
    memset(g_last_dir, 0, sizeof(g_last_dir));
    g_last_dir_size = 0;

    if (fread(out_bh, sizeof(bra_io_header_t), 1, bf->f) != 1)
    {
        bra_io_file_read_error(bf);
        return false;
    }

    // check header magic
    if (out_bh->magic != BRA_MAGIC)
    {
        bra_log_error("Not valid %s file: %s", BRA_NAME, bf->fn);
        bra_io_file_close(bf);
        return false;
    }

    return true;
}

bool bra_io_file_write_header(bra_io_file_t* f, const uint32_t num_files)
{
    assert_bra_io_file_t(f);

    const bra_io_header_t header = {
        .magic = BRA_MAGIC,
        // .version   = BRA_ARCHIVE_VERSION,
        .num_files = num_files,
    };

    if (fwrite(&header, sizeof(bra_io_header_t), 1, f->f) != 1)
    {
        bra_io_file_write_error(f);
        return false;
    }

    return true;
}

bool bra_io_file_read_footer(bra_io_file_t* f, bra_io_footer_t* bf_out)
{
    assert_bra_io_file_t(f);
    assert(bf_out != NULL);

    memset(bf_out, 0, sizeof(bra_io_footer_t));
    if (fread(bf_out, sizeof(bra_io_footer_t), 1, f->f) != 1)
    {
        bra_io_file_read_error(f);
        return false;
    }

    // check footer magic
    if (bf_out->magic != BRA_FOOTER_MAGIC || bf_out->header_offset <= 0)
    {
        bra_log_error("corrupted or not valid %s-SFX file: %s", BRA_NAME, f->fn);
        bra_io_file_close(f);
        return false;
    }

    return true;
}

bool bra_io_file_write_footer(bra_io_file_t* f, const int64_t header_offset)
{
    assert_bra_io_file_t(f);
    assert(header_offset > 0);

    bra_io_footer_t bf = {
        .magic         = BRA_FOOTER_MAGIC,
        .header_offset = header_offset,
    };

    if (fwrite(&bf, sizeof(bra_io_footer_t), 1, f->f) != 1)
    {
        bra_io_file_write_error(f);
        return false;
    }

    return true;
}

bool bra_io_file_sfx_open_and_read_footer_header(const char* fn, bra_io_header_t* out_bh, bra_io_file_t* f)
{
    assert(fn != NULL);
    assert(out_bh != NULL);
    assert(f != NULL);

    if (!bra_io_file_sfx_open(f, fn, "rb"))
        return false;

    bra_io_footer_t bf;
    bf.header_offset = 0;
    bf.magic         = 0;

    if (!bra_io_file_read_footer(f, &bf))
        return false;

    // Ensure header_offset is before the footer and that the header fits.
    const int64_t footer_start = bra_io_file_tell(f) - (int64_t) sizeof(bra_io_footer_t);
    if (footer_start <= 0 ||
        bf.header_offset <= 0 ||
        bf.header_offset >= footer_start - (int64_t) sizeof(bra_io_header_t))
    {
        bra_log_error("corrupted or not valid %s-SFX file (header_offset out of bounds): %s",
                      BRA_NAME,
                      f->fn);
        bra_io_file_close(f);
        return false;
    }

    // read header and check
    if (!bra_io_file_seek(f, bf.header_offset, SEEK_SET))
    {
        bra_io_file_seek_error(f);
        return false;
    }

    if (!bra_io_file_read_header(f, out_bh))
        return false;

    return true;
}

bool bra_io_file_read_meta_file(bra_io_file_t* f, bra_meta_file_t* mf)
{
    assert_bra_io_file_t(f);
    assert(mf != NULL);

    char     buf[BRA_MAX_PATH_LENGTH];
    unsigned buf_size = 0;

    mf->name      = NULL;
    mf->name_size = 0;
    mf->data_size = 0;

    // 1. attributes
    if (fread(&mf->attributes, sizeof(uint8_t), 1, f->f) != 1)
    {
    BRA_IO_READ_ERR:
        bra_io_file_read_error(f);
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
    if (fread(&buf_size, sizeof(uint8_t), 1, f->f) != 1)
        goto BRA_IO_READ_ERR;
    if (buf_size == 0 || buf_size >= BRA_MAX_PATH_LENGTH)
        goto BRA_IO_READ_ERR;

    // 3. filename
    if (fread(buf, sizeof(char), buf_size, f->f) != buf_size)
        goto BRA_IO_READ_ERR;

    buf[buf_size] = '\0';

    // 4. data size
    if (mf->attributes == BRA_ATTR_DIR)
    {
        // NOTE: for directory doesn't have data-size nor data,
        // NOTE: here if it is a sub-dir
        //       it could cut some extra chars, and be constructed from the other dir
        //       but the file won't be able to reconstruct its full relative path.
        //   SO: I can't optimize sub-dir length with this struct
        //       I must replicate the parent-dir too
        // TODO: unless dir attribute has a 2nd bit to tell sub-dir or dir
        //       but then must track the sub-dir (postponed for now until recursive)
        strncpy(g_last_dir, buf, buf_size);
        g_last_dir_size      = buf_size;
        g_last_dir[buf_size] = '\0';

        mf->name_size = (uint8_t) buf_size;
        mf->name      = malloc(sizeof(char) * (buf_size + 1));    // !< one extra for '\0'
        if (mf->name == NULL)
            goto BRA_IO_READ_ERR;

        strncpy(mf->name, buf, buf_size);
    }
    else if (mf->attributes == BRA_ATTR_FILE)
    {
        if (fread(&mf->data_size, sizeof(uint64_t), 1, f->f) != 1)
            goto BRA_IO_READ_ERR;

        const size_t total_size =
            g_last_dir_size + buf_size + (g_last_dir_size > 0);    // one extra for '/'
        if (total_size == 0 || total_size > UINT8_MAX)
            goto BRA_IO_READ_ERR;

        mf->name_size = (uint8_t) total_size;
        mf->name      = malloc(sizeof(char) * (mf->name_size + 1));    // !< one extra for '\0'
        if (mf->name == NULL)
            goto BRA_IO_READ_ERR;

        char* b = NULL;
        if (g_last_dir_size > 0)
        {
            // strncpy(&mf->name[g_last_dir_size + 1], buf, buf_size);
            strncpy(mf->name, g_last_dir, g_last_dir_size);
            mf->name[g_last_dir_size] = '/';
            b                         = &mf->name[g_last_dir_size + 1];
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

bool bra_io_file_write_meta_file(bra_io_file_t* f, const bra_meta_file_t* mf)
{
    assert_bra_io_file_t(f);
    assert(mf != NULL);
    assert(mf->name != NULL);

    char    buf[BRA_MAX_PATH_LENGTH];
    uint8_t buf_size;

    const size_t len = strnlen(mf->name, BRA_MAX_PATH_LENGTH + 1);
    if (len != mf->name_size || len == 0 || len > BRA_MAX_PATH_LENGTH)
        goto BRA_IO_WRITE_ERR;

    // Processing data
    switch (mf->attributes)
    {
    case BRA_ATTR_FILE:
    {
        if (g_last_dir_size >= mf->name_size)
        {
            // In this case is a parent/sibling folder.
            // but it should have read a dir instead.
            // error because two consecutive files in different
            // folders are been submitted.
            goto BRA_IO_WRITE_ERR;
        }

        if (strncmp(mf->name, g_last_dir, g_last_dir_size) != 0)    // g_last_dir doesn't have '/'
            goto BRA_IO_WRITE_ERR;

        size_t l = g_last_dir_size;    // strnlen(g_last_dir, BRA_MAX_PATH_LENGTH);
        if (mf->name[l] == '/')        // or when l == 0
            ++l;                       // skip also '/'

        buf_size = mf->name_size - l;
        strncpy(buf, &mf->name[l], buf_size);
        break;
    }
    case BRA_ATTR_DIR:
    {
        // As a safe-guard, but pointless otherwise
        if (mf->data_size != 0)
            goto BRA_IO_WRITE_ERR;

        buf_size = mf->name_size;
        strncpy(buf, mf->name, buf_size);
        buf[buf_size] = '\0';
        strncpy(g_last_dir, buf, buf_size);
        g_last_dir[buf_size] = '\0';
        g_last_dir_size      = buf_size;
    }
    break;
    default:
        goto BRA_IO_WRITE_ERR;
    }

    // 1. attributes
    if (fwrite(&mf->attributes, sizeof(uint8_t), 1, f->f) != 1)
    {
    BRA_IO_WRITE_ERR:
        bra_io_file_write_error(f);
        return false;
    }

    // 2. filename size
    if (fwrite(&buf_size, sizeof(uint8_t), 1, f->f) != 1)
        goto BRA_IO_WRITE_ERR;

    // 3. filename
    if (fwrite(buf, sizeof(char), buf_size, f->f) != buf_size)
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

bool bra_io_file_copy_file_chunks(bra_io_file_t* dst, bra_io_file_t* src, const uint64_t data_size)
{
    assert_bra_io_file_t(dst);
    assert_bra_io_file_t(src);

    char buf[BRA_MAX_CHUNK_SIZE];

    for (uint64_t i = 0; i < data_size;)
    {
        uint32_t s = bra_min(BRA_MAX_CHUNK_SIZE, data_size - i);

        // read source chunk
        if (fread(buf, sizeof(char), s, src->f) != s)
        {
            bra_io_file_read_error(src);
            bra_io_file_close(dst);
            return false;
        }

        // write source chunk
        if (fwrite(buf, sizeof(char), s, dst->f) != s)
        {
            bra_io_file_write_error(dst);
            bra_io_file_close(src);
            return false;
        }

        i += s;
    }

    return true;
}

bool bra_io_file_skip_data(bra_io_file_t* f, const uint64_t data_size)
{
    assert_bra_io_file_t(f);

    return bra_io_file_seek(f, data_size, SEEK_CUR);
}

bool bra_io_file_encode_and_write_to_disk(bra_io_file_t* f, const char* fn)
{
    assert_bra_io_file_t(f);
    assert(fn != NULL);

    // 1. attributes
    bra_attr_t attributes;
    if (!bra_fs_file_attributes(fn, &attributes))
    {
        bra_log_error("%s has unknown attribute", fn);
    BRA_IO_WRITE_CLOSE_ERROR:
        bra_io_file_close(f);
        return false;
    }

    // TODO: can just use it as an index and printing the right word instead.
    //       avoiding to do a switch statement using a bit mask to manage eventual errors
    switch (attributes)
    {
    case BRA_ATTR_DIR:
        // bra_log_printf("Archiving dir :  " BRA_PRINTF_FMT_FILENAME, fn);
        bra_log_printf("Archiving dir :  ");
        break;
    case BRA_ATTR_FILE:
        // bra_log_printf("Archiving file:  " BRA_PRINTF_FMT_FILENAME, fn);
        bra_log_printf("Archiving file:  ");

        break;
    default:
        goto BRA_IO_WRITE_CLOSE_ERROR;
    }

    _bra_print_string_max_length(fn, strlen(fn), BRA_PRINTF_FMT_FILENAME_MAX_LENGTH);

    // 2. file name length
    const size_t fn_len = strnlen(fn, BRA_MAX_PATH_LENGTH);
    // NOTE: fn_len >= BRA_MAX_PATH_LENGTH is redundant but as a safeguard
    //       in case changing BRA_MAX_PATH_LENGTH to be UINT8_MAX
    //       there is a static assert for that now.
    if (fn_len > UINT8_MAX /*|| fn_len >= BRA_MAX_PATH_LENGTH*/)
    {
        bra_log_error("filename too long: %s", fn);
        goto BRA_IO_WRITE_CLOSE_ERROR;
    }

    if (fn_len == 0)
    {
        bra_log_error("filename missing: %s", fn);
        goto BRA_IO_WRITE_CLOSE_ERROR;
    }

    const uint8_t fn_size = (uint8_t) fn_len;
    uint64_t      ds;
    if (!bra_fs_file_size(fn, &ds))
        goto BRA_IO_WRITE_CLOSE_ERROR;

    bra_meta_file_t mf;
    mf.attributes = attributes;
    mf.name_size  = fn_size;
    mf.data_size  = ds;
    mf.name       = bra_strdup(fn);
    if (mf.name == NULL)
        goto BRA_IO_WRITE_CLOSE_ERROR;

    const bool res = bra_io_file_write_meta_file(f, &mf);
    bra_meta_file_free(&mf);
    if (!res)
        return false;    // f closed already

    // 4. data (this should be paired with data_size to avoid confusion
    //          instead is in the meta file write function
    //          NOTE: that data_size is written only for file in there)
    switch (attributes)
    {
    case BRA_ATTR_DIR:
        // NOTE: Directory doesn't have the data part
        break;
    case BRA_ATTR_FILE:
    {
        bra_io_file_t f2;

        memset(&f2, 0, sizeof(bra_io_file_t));
        if (!bra_io_file_open(&f2, fn, "rb"))
            goto BRA_IO_WRITE_CLOSE_ERROR;

        if (!bra_io_file_copy_file_chunks(f, &f2, ds))
            return false;    // f, f2 closed already

        bra_io_file_close(&f2);
    }
    break;
    }

    bra_log_printf(" [  %-4.4s  ]\n", g_end_messages[0]);
    return true;
}

bool bra_io_file_decode_and_write_to_disk(bra_io_file_t* f, bra_fs_overwrite_policy_e* overwrite_policy)
{
    assert_bra_io_file_t(f);
    assert(overwrite_policy != NULL);

    const char*     end_msg;    // 'OK  ' | 'SKIP'
    bra_meta_file_t mf;
    if (!bra_io_file_read_meta_file(f, &mf))
        return false;

    if (!bra_validate_meta_filename(&mf))
    {
    BRA_IO_DECODE_ERR:
        bra_meta_file_free(&mf);
        bra_io_file_error(f, "decode");
        return false;
    }

    // 4. read and write in chunk data
    // NOTE: nothing to extract for a directory, but only to create it
    switch (mf.attributes)
    {
    case BRA_ATTR_FILE:
    {
        const uint64_t ds = mf.data_size;
        if (!bra_fs_file_exists_ask_overwrite(mf.name, overwrite_policy, false))
        {
            end_msg = g_end_messages[1];
            bra_log_printf("Skipping file:   " BRA_PRINTF_FMT_FILENAME, mf.name);
            bra_meta_file_free(&mf);
            if (!bra_io_file_skip_data(f, ds))
            {
                bra_io_file_read_error(f);
                return false;
            }
        }
        else
        {
            bra_io_file_t f2;

            end_msg = g_end_messages[0];
            bra_log_printf("Extracting file: " BRA_PRINTF_FMT_FILENAME, mf.name);
            // NOTE: the directory must have been created in the previous entry,
            //       otherwise this will fail to create the file.
            //       The archive ensures the last used directory is created first,
            //       and then its files follow.
            //       So, no need to create the parent directory for each file each time.
            if (!bra_io_file_open(&f2, mf.name, "wb"))
                goto BRA_IO_DECODE_ERR;

            bra_meta_file_free(&mf);
            if (!bra_io_file_copy_file_chunks(&f2, f, ds))
                return false;

            bra_io_file_close(&f2);
        }
    }
    break;
    case BRA_ATTR_DIR:
    {
        if (bra_fs_dir_exists(mf.name))
        {
            end_msg = g_end_messages[1];
            bra_log_printf("Dir exists:   " BRA_PRINTF_FMT_FILENAME, mf.name);
        }
        else
        {
            end_msg = g_end_messages[0];
            bra_log_printf("Creating dir: " BRA_PRINTF_FMT_FILENAME, mf.name);

            if (!bra_fs_dir_make(mf.name))
                goto BRA_IO_DECODE_ERR;
        }

        bra_meta_file_free(&mf);
    }
    break;
    default:
        goto BRA_IO_DECODE_ERR;
        break;
    }

    bra_log_printf(" [  %-4.4s  ]\n", end_msg);
    return true;
}
