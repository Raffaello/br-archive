#include <io/lib_bra_io_file.h>

#include <lib_bra.h>
#include <lib_bra_private.h>

#include <encoders/bra_bwt.h>
#include <encoders/bra_mtf.h>

#include <fs/bra_fs_c.h>
#include <log/bra_log.h>

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>      // UINT8_MAX, uint{8,32,64}_t
#include <stdio.h>       // FILE, fopen/fread/fwrite
#include <inttypes.h>    // PRIu64

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

_Static_assert(BRA_MAX_PATH_LENGTH > UINT8_MAX, "BRA_MAX_PATH_LENGTH must be greater than bra_meta_entry_t.name_size max value");
_Static_assert(sizeof(bra_io_header_t) == 8, "bra_io_header_t must be 8 bytes");
_Static_assert(sizeof(bra_io_footer_t) == 12, "bra_io_footer_t must be 12 bytes");

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int _bra_io_file_magic_is_elf(bra_io_file_t* f)
{
    enum
    {
        MAGIC_SIZE = 4
    };

    assert_bra_io_file_t(f);

    // 0x7F,'E','L','F'
    uint8_t magic[MAGIC_SIZE];
    if (fread(magic, sizeof(uint8_t), MAGIC_SIZE, f->f) != MAGIC_SIZE)
    {
        bra_io_file_read_error(f);
        return -1;
    }

    return magic[0] == 0x7F && magic[1] == 'E' && magic[2] == 'L' && magic[3] == 'F' ? 1 : 0;
}

static int _bra_io_file_magic_is_pe_exe(bra_io_file_t* f)
{
    enum
    {
        MAGIC_SIZE    = 2,
        PE_MAGIC_SIZE = 4
    };

    assert_bra_io_file_t(f);

    // 'M' 'Z'
    uint8_t magic[MAGIC_SIZE];
    if (fread(magic, sizeof(uint8_t), MAGIC_SIZE, f->f) != MAGIC_SIZE)
    {
    _BRA_IO_FILE_MAGIC_IS_PE_EXE_READ_ERROR:
        bra_io_file_read_error(f);
        return -1;
    }

    if (magic[0] != 'M' || magic[1] != 'Z')
        return 0;

    // Read the PE header offset from the DOS header
    if (!bra_io_file_seek(f, 0x3C, SEEK_SET))
    {
    _BRA_IO_FILE_MAGIC_IS_PE_EXE_SEEK_ERROR:
        bra_io_file_seek_error(f);
        return -1;
    }

    uint32_t pe_offset;
    if (fread(&pe_offset, sizeof(uint32_t), 1, f->f) != 1)
        goto _BRA_IO_FILE_MAGIC_IS_PE_EXE_READ_ERROR;

    if (pe_offset < 0x40)    // DOS header is at least 64 bytes
        return 0;

    if (!bra_io_file_seek(f, pe_offset, SEEK_SET))
        goto _BRA_IO_FILE_MAGIC_IS_PE_EXE_SEEK_ERROR;

    uint8_t pe_magic[PE_MAGIC_SIZE];
    if (fread(pe_magic, sizeof(uint8_t), PE_MAGIC_SIZE, f->f) != PE_MAGIC_SIZE)
        goto _BRA_IO_FILE_MAGIC_IS_PE_EXE_READ_ERROR;

    return pe_magic[0] == 'P' && pe_magic[1] == 'E' && pe_magic[2] == '\0' && pe_magic[3] == '\0' ? 1 : 0;
}

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
    assert(fn != NULL);

    bra_io_file_t f;

    if (!bra_io_file_open(&f, fn, "rb"))
        return false;

    int res = _bra_io_file_magic_is_elf(&f);
    bra_io_file_close(&f);
    if (res == 1)
        bra_log_info("ELF file detected");
    return res == 1;
}

bool bra_io_file_is_pe_exe(const char* fn)
{
    assert(fn != NULL);

    bra_io_file_t f;
    if (!bra_io_file_open(&f, fn, "rb"))
        return false;

    const int res = _bra_io_file_magic_is_pe_exe(&f);
    bra_io_file_close(&f);
    if (res == 1)
        bra_log_info("PE/EXE file detected");
    return res == 1;
}

bool bra_io_file_can_be_sfx(const char* fn)
{
    assert(fn != NULL);

    bra_io_file_t f;
    if (!bra_io_file_open(&f, fn, "rb"))
        return false;

    int res = _bra_io_file_magic_is_elf(&f);
    if (res == -1)
        return false;

    if (res == 1)
    {
        bra_io_file_close(&f);
        return true;
    }

    if (!bra_io_file_seek(&f, 0, SEEK_SET))
    {
        bra_io_file_seek_error(&f);
        return false;
    }

    res = _bra_io_file_magic_is_pe_exe(&f);
    bra_io_file_close(&f);
    return res == 1;
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

bool bra_io_file_read_chunk(bra_io_file_t* src, void* buf, const size_t buf_size)
{
    assert_bra_io_file_t(src);
    assert(buf != NULL);
    assert(buf_size > 0);

    if (fread(buf, sizeof(char), buf_size, src->f) != buf_size)
    {
        bra_io_file_read_error(src);
        return false;
    }

    return true;
}

bool bra_io_file_read_file_chunks(bra_io_file_t* src, const uint64_t data_size, bra_meta_entry_t* me)
{
    assert_bra_io_file_t(src);
    assert(me != NULL);

    char buf[BRA_MAX_CHUNK_SIZE];

    for (uint64_t i = 0; i < data_size;)
    {
        const uint32_t s = _bra_min(BRA_MAX_CHUNK_SIZE, data_size - i);

        // update CRC32
        switch (BRA_ATTR_COMP(me->attributes))
        {
        case BRA_ATTR_COMP_STORED:
            if (!bra_io_file_read_chunk(src, buf, s))
                return false;

            me->crc32 = bra_crc32c(buf, s, me->crc32);
            break;
        case BRA_ATTR_COMP_COMPRESSED:
        {
            // TODO: review
            bra_bwt_index_t primary_index = 0;
            if (!fread(&primary_index, sizeof(bra_bwt_index_t), 1, src->f))    // read and ignore primary index
            {
                bra_log_error("unable to read primary index from %s", src->fn);
                return false;
            }

            if (!bra_io_file_read_chunk(src, buf, s))
                return false;

            uint8_t* buf_mtf = bra_mtf_decode((uint8_t*) buf, s);
            if (buf_mtf == NULL)
            {
                bra_log_error("bra_mtf_decode() failed: %s (chunk: %" PRIu64 ")", src->fn, i);
                return false;
            }
            uint8_t* buf_bwt = bra_bwt_decode((uint8_t*) buf_mtf, s, primary_index);
            if (buf_bwt == NULL)
            {
                bra_log_error("bra_bwt_decode() failed: %s (chunk: %" PRIu64 ")", src->fn, i);
                free(buf_mtf);
                return false;
            }

            me->crc32 = bra_crc32c(buf_bwt, s, me->crc32);
            free(buf_mtf);
            free(buf_bwt);
        }
        break;
        default:
            bra_log_critical("invalid compression type for file: %u", BRA_ATTR_COMP(me->attributes));
            return false;
        }

        i += s;
    }

    return true;
}

bool bra_io_file_copy_file_chunks(bra_io_file_t* dst, bra_io_file_t* src, const uint64_t data_size, bra_meta_entry_t* me)
{
    assert_bra_io_file_t(dst);
    assert_bra_io_file_t(src);
    assert(me != NULL);

    char buf[BRA_MAX_CHUNK_SIZE];

    for (uint64_t i = 0; i < data_size;)
    {
        const uint32_t s = _bra_min(BRA_MAX_CHUNK_SIZE, data_size - i);

        // read source chunk
        if (!bra_io_file_read_chunk(src, buf, s))
        {
            bra_io_file_close(dst);
            return false;
        }

        // update CRC32
        me->crc32 = bra_crc32c(buf, s, me->crc32);

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

    const bool res = bra_io_file_seek(f, data_size, SEEK_CUR);
    if (!res)
        bra_io_file_seek_error(f);

    return res;
}

bool bra_io_file_compress_file_chunks(bra_io_file_t* dst, bra_io_file_t* src, const uint64_t data_size, bra_meta_entry_t* me)
{
    assert_bra_io_file_t(dst);
    assert_bra_io_file_t(src);
    assert(me != NULL);

    char     buf[BRA_MAX_CHUNK_SIZE];
    uint8_t* buf_bwt = NULL;
    uint8_t* buf_mtf = NULL;

    for (uint64_t i = 0; i < data_size;)
    {
        const uint32_t s = _bra_min(BRA_MAX_CHUNK_SIZE, data_size - i);

        // read source chunk
        if (!bra_io_file_read_chunk(src, buf, s))
        {
            bra_io_file_close(dst);
            return false;
        }

        // update CRC32
        me->crc32 = bra_crc32c(buf, s, me->crc32);

        // compress BWT+MTF(+RLE)
        // TODO: do the version accepting a pre-allocated buffer
        //       as it is always the same size as the input doing in chunks will avoid to allocate/free
        //       for each chunk.
        bra_bwt_index_t primary_index = 0;
        buf_bwt                       = bra_bwt_encode((uint8_t*) buf, s, &primary_index);
        if (buf_bwt == NULL)
        {
            bra_log_error("bra_bwt_encode() failed: %s (chunk: %" PRIu64 ")", src->fn, i);
            goto BRA_IO_FILE_COMPRESS_FILE_CHUNKS_ERR;
        }

        buf_mtf = bra_mtf_encode(buf_bwt, s);
        if (buf_mtf == NULL)
        {
            bra_log_error("bra_mtf_encode() failed: %s (chunk: %" PRIu64 ")", src->fn, i);
            goto BRA_IO_FILE_COMPRESS_FILE_CHUNKS_ERR;
        }

        // write primary index
        if (fwrite(&primary_index, sizeof(bra_bwt_index_t), 1, dst->f) != 1)
        {
            bra_log_error("unable to write primary index to %s", dst->fn);
            goto BRA_IO_FILE_COMPRESS_FILE_CHUNKS_ERR;
        }

        // write source chunk
        if (fwrite(buf_mtf, sizeof(char), s, dst->f) != s)
            goto BRA_IO_FILE_COMPRESS_FILE_CHUNKS_ERR;

        free(buf_bwt);
        buf_bwt = NULL;
        free(buf_mtf);
        buf_mtf = NULL;

        i += s;
    }

    return true;

BRA_IO_FILE_COMPRESS_FILE_CHUNKS_ERR:
    if (buf_bwt != NULL)
        free(buf_bwt);
    if (buf_mtf != NULL)
        free(buf_mtf);
    bra_io_file_close(dst);
    bra_io_file_close(src);
    return false;
}

bool bra_io_file_decompress_file_chunks(bra_io_file_t* dst, bra_io_file_t* src, const uint64_t data_size, bra_meta_entry_t* me)
{
    assert_bra_io_file_t(dst);
    assert_bra_io_file_t(src);
    assert(me != NULL);

    char     buf[BRA_MAX_CHUNK_SIZE];
    uint8_t* buf_bwt = NULL;
    uint8_t* buf_mtf = NULL;

    for (uint64_t i = 0; i < data_size;)
    {
        const uint32_t s = _bra_min(BRA_MAX_CHUNK_SIZE, data_size - i);

        // read primary index
        bra_bwt_index_t primary_index = 0;
        if (fread(&primary_index, sizeof(bra_bwt_index_t), 1, src->f) != 1)
        {
            goto BRA_IO_FILE_DECOMPRESS_FILE_CHUNKS_ERR;
        }

        if (primary_index >= s)
        {
            bra_log_error("invalid primary index (%u) for chunk size %" PRIu32 " in %s", primary_index, s, src->fn);
            goto BRA_IO_FILE_DECOMPRESS_FILE_CHUNKS_ERR;
        }

        // read source chunk
        if (!bra_io_file_read_chunk(src, buf, s))
        {
            goto BRA_IO_FILE_DECOMPRESS_FILE_CHUNKS_ERR;
        }

        // decompress MTF+BWT
        buf_mtf = bra_mtf_decode((uint8_t*) buf, s);
        if (buf_mtf == NULL)
        {
            bra_log_error("bra_mtf_decode() failed: %s (chunk: %" PRIu64 ")", src->fn, i);
            goto BRA_IO_FILE_DECOMPRESS_FILE_CHUNKS_ERR;
        }

        buf_bwt = bra_bwt_decode((uint8_t*) buf_mtf, s, primary_index);
        if (buf_bwt == NULL)
        {
            bra_log_error("bra_bwt_decode() failed: %s (chunk: %" PRIu64 ")", src->fn, i);
            goto BRA_IO_FILE_DECOMPRESS_FILE_CHUNKS_ERR;
        }

        // update CRC32
        me->crc32 = bra_crc32c(buf_bwt, s, me->crc32);

        // write source chunk
        if (fwrite(buf_bwt, sizeof(char), s, dst->f) != s)
            goto BRA_IO_FILE_DECOMPRESS_FILE_CHUNKS_ERR;

        free(buf_bwt);
        buf_bwt = NULL;
        free(buf_mtf);
        buf_mtf = NULL;

        i += s;
    }

    return true;

BRA_IO_FILE_DECOMPRESS_FILE_CHUNKS_ERR:
    if (buf_bwt != NULL)
        free(buf_bwt);
    if (buf_mtf != NULL)
        free(buf_mtf);

    bra_io_file_close(dst);
    bra_io_file_close(src);
    return false;
}
