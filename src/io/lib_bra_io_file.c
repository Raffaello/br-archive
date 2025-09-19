#include <io/lib_bra_io_file.h>

#include <lib_bra.h>
#include <lib_bra_private.h>

#include <fs/bra_fs_c.h>
#include <log/bra_log.h>

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>    // UINT8_MAX, uint{8,32,64}_t
#include <stdio.h>     // FILE, fopen/fread/fwrite

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

_Static_assert(BRA_MAX_PATH_LENGTH > UINT8_MAX, "BRA_MAX_PATH_LENGTH must be greater than bra_meta_entry_t.name_size max value");
_Static_assert(sizeof(bra_io_header_t) == 8, "bra_io_header_t must be 8 bytes");
_Static_assert(sizeof(bra_io_footer_t) == 12, "bra_io_footer_t must be 12 bytes");

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void bra_io_file_error(bra_io_file_t* f, const char* verb)
{
    assert(f != NULL);
    assert(verb != NULL);

    const char* fn = (f->fn != NULL) ? f->fn : "N/A";
    bra_log_error("unable to %s %s file (errno %d: %s)", verb, fn, errno, strerror(errno));
    bra_io_file_close(f);
}

inline void bra_io_file_open_error(bra_io_file_t* f)
{
    bra_io_file_error(f, "open");
}

inline void bra_io_file_read_error(bra_io_file_t* f)
{
    bra_io_file_error(f, "read");
}

inline void bra_io_file_seek_error(bra_io_file_t* f)
{
    bra_io_file_error(f, "seek");
}

inline void bra_io_file_write_error(bra_io_file_t* f)
{
    bra_io_file_error(f, "write");
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

bool bra_io_file_open(bra_io_file_t* f, const char* fn, const char* mode)
{
    if (fn == NULL || f == NULL || mode == NULL)
        return false;

    f->f  = fopen(fn, mode);    // open file
    f->fn = _bra_strdup(fn);    // copy filename
    if (f->f == NULL || f->fn == NULL)
    {
        bra_io_file_open_error(f);
        return false;
    }

    return true;
}

void bra_io_file_close(bra_io_file_t* f)
{
    assert(f != NULL);

    if (f->fn != NULL)
    {
        free(f->fn);
        f->fn = NULL;
    }

    if (f->f != NULL)
    {
        fclose(f->f);
        f->f = NULL;
    }
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

bool bra_io_file_copy_file_chunks(bra_io_file_t* dst, bra_io_file_t* src, const uint64_t data_size)
{
    assert_bra_io_file_t(dst);
    assert_bra_io_file_t(src);

    char buf[BRA_MAX_CHUNK_SIZE];

    for (uint64_t i = 0; i < data_size;)
    {
        const uint64_t s = _bra_min(BRA_MAX_CHUNK_SIZE, data_size - i);

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
