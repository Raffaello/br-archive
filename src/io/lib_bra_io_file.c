#include <io/lib_bra_io_file.h>

#include <lib_bra.h>
#include <lib_bra_private.h>

#include <encoders/bra_bwt.h>
#include <encoders/bra_mtf.h>
#include <encoders/bra_huffman.h>

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

static inline bool bra_io_file_read_file_chunks_stored(bra_io_file_t* src, const uint64_t data_size, bra_meta_entry_t* me)
{
    char buf[BRA_MAX_CHUNK_SIZE];

    for (uint64_t i = 0; i < data_size;)
    {
        const uint32_t s = _bra_min(BRA_MAX_CHUNK_SIZE, data_size - i);

        // update CRC32
        if (!bra_io_file_read_chunk(src, buf, s))
            return false;

        me->crc32 = bra_crc32c(buf, s, me->crc32);

        i += s;
    }

    return true;
}

static inline bool bra_io_file_read_file_chunks_compressed(bra_io_file_t* src, const uint64_t data_size, bra_meta_entry_t* me)
{
    uint8_t         buf[BRA_MAX_CHUNK_SIZE];
    uint8_t         buf_mtf[BRA_MAX_CHUNK_SIZE];
    uint8_t         buf_bwt[BRA_MAX_CHUNK_SIZE];
    bra_bwt_index_t buf_transform[BRA_MAX_CHUNK_SIZE];
    uint8_t*        buf_huffman = NULL;

    for (uint64_t i = 0; i < data_size;)
    {
        bra_io_chunk_header_t chunk_header = {.primary_index = 0};
        if (!bra_io_file_read_chunk_header(src, &chunk_header))
            return false;

        // sanity checks
        if (chunk_header.huffman.encoded_size > BRA_MAX_CHUNK_SIZE)
        {
            bra_log_error("encoded chunk size (%" PRIu32 ") exceeds maximum (%" PRIu32 ") in %s",
                          chunk_header.huffman.encoded_size,
                          (uint32_t) BRA_MAX_CHUNK_SIZE,
                          src->fn);
            bra_io_file_read_error(src);
            return false;
        }

        if (chunk_header.huffman.orig_size > BRA_MAX_CHUNK_SIZE)
        {
            bra_log_error("decoded chunk size (%" PRIu32 ") exceeds maximum (%" PRIu32 ") in %s",
                          chunk_header.huffman.orig_size,
                          (uint32_t) BRA_MAX_CHUNK_SIZE,
                          src->fn);
            bra_io_file_read_error(src);
            return false;
        }
        // ----

        // read source chunk
        if (!bra_io_file_read_chunk(src, buf, chunk_header.huffman.encoded_size))
            return false;

        // decode huffman
        uint32_t s  = 0;
        buf_huffman = bra_huffman_decode(&chunk_header.huffman, buf, &s);
        if (buf_huffman == NULL)
        {
            bra_log_error("unable to decode huffman file: %s ", src->fn);
            bra_io_file_read_error(src);
            return false;
        }

        if (chunk_header.primary_index >= s)
        {
            free(buf_huffman);
            buf_huffman = NULL;
            bra_log_error("invalid primary index (%" PRIu32 ") for chunk size %" PRIu32 " in %s", chunk_header.primary_index, s, src->fn);
            bra_io_file_read_error(src);
            return false;
        }

        bra_mtf_decode2((uint8_t*) buf_huffman, s, buf_mtf);
        bra_bwt_decode2(buf_mtf, s, chunk_header.primary_index, buf_transform, buf_bwt);

        // TODO: it would be better on the uncompressed data to compute CRC32.
        //       this must be reviewed.
        me->crc32 = bra_crc32c(&chunk_header, sizeof(bra_io_chunk_header_t), me->crc32);
        me->crc32 = bra_crc32c(buf, chunk_header.huffman.encoded_size, me->crc32);

        free(buf_huffman);
        buf_huffman = NULL;

        i += s;
    }

    return true;
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

bool bra_io_file_tmp_open(bra_io_file_t* f)
{
    f->f  = tmpfile();
    f->fn = _bra_strdup("");
    if (f->f == NULL || f->fn == NULL)
    {
        bra_io_file_close(f);
        bra_log_error("unable to create tmpfile");
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

bool bra_io_file_read_chunk_header(bra_io_file_t* src, bra_io_chunk_header_t* chunk_header)
{
    assert_bra_io_file_t(src);
    assert(chunk_header != NULL);

    if (!fread(chunk_header, sizeof(bra_io_chunk_header_t), 1, src->f))
    {
        bra_log_error("unable to read chunk header from %s", src->fn);
        bra_io_file_read_error(src);
        return false;
    }

    return true;
}

bool bra_io_file_read_file_chunks(bra_io_file_t* src, const uint64_t data_size, bra_meta_entry_t* me)
{
    assert_bra_io_file_t(src);
    assert(me != NULL);

    switch (BRA_ATTR_COMP(me->attributes))
    {
    case BRA_ATTR_COMP_STORED:
        return bra_io_file_read_file_chunks_stored(src, data_size, me);
    case BRA_ATTR_COMP_COMPRESSED:
        return bra_io_file_read_file_chunks_compressed(src, data_size, me);
    default:
        bra_log_critical("invalid compression type for file: %u", BRA_ATTR_COMP(me->attributes));
        return false;
    }
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

    char                 buf[BRA_MAX_CHUNK_SIZE];
    uint8_t*             buf_bwt     = NULL;
    uint8_t*             buf_mtf     = NULL;
    bra_huffman_chunk_t* buf_huffman = NULL;

    // NOTE: compress a file is done in a temporary file:
    //      if it is smaller than the original file append it to the archive.
    //      otherwise change the attribute to store and redo the whole file
    //      processing including metadata due to CRC32
    // TODO: it could be improved a bit, but the CRC32 must be recomputed as
    //       it is also taking into account the metadata entry
    // TODO: CRC32 should be done after the archiving/storing/compression operation on the whole entry.
    //       as it is a merely a footer so compute it only at the very end when adding it to the archive.
    //       This implies to do the same for the store counterpart.
    bra_io_file_t tmpfile;
    if (!bra_io_file_tmp_open(&tmpfile))
    {
        bra_log_error("unable to compress file: %s", src->fn);
        return false;
    }

    for (uint64_t i = 0; i < data_size;)
    {
        const uint32_t s = _bra_min(BRA_MAX_CHUNK_SIZE, data_size - i);

        // TODO: create a metafunction bra_io_file_process_file_chunks accepting the function to do the operation
        //       copy, read, compress, decompress
        //       this will avoid to duplicate the loop code.
        bra_log_printf("%3u%%", (unsigned int) (i * 100 / data_size));
        bra_log_printf("\b\b\b\b");

        // read source chunk
        if (!bra_io_file_read_chunk(src, buf, s))
        {
            bra_io_file_close(&tmpfile);
            bra_io_file_close(dst);
            return false;
        }

        // compress BWT+MTF(+RLE)+huffman
        // TODO: do the version accepting a pre-allocated buffer
        //       as it is always the same size as the input doing in chunks will avoid to allocate/free
        //       for each chunk.
        bra_io_chunk_header_t chunk_header = {.primary_index = 0};
        buf_bwt                            = bra_bwt_encode((uint8_t*) buf, s, &chunk_header.primary_index);
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

        // huffman encoding
        buf_huffman = bra_huffman_encode(buf_mtf, s);
        if (buf_huffman == NULL)
        {
            bra_log_error("bra_huffman_encode() failed: %s (chunk: %" PRIu64 ")", src->fn, i);
            goto BRA_IO_FILE_COMPRESS_FILE_CHUNKS_ERR;
        }

        chunk_header.huffman = buf_huffman->meta;

        // update CRC32 (It will be computed when appending the encoded data to the archive instead...)
        // TODO: it might be better to compute the CRC32 on the original data.
        //       so when testing/decompressing it will be checked with the original data and not the one compressed.
        //       it would be more reliable but this need to be addressed separately.
        // me->crc32 = bra_crc32c(&chunk_header, sizeof(chunk_header), me->crc32);
        // me->crc32 = bra_crc32c(buf_huffman->data, chunk_header.huffman.encoded_size, me->crc32);

        // write chunk header
        if (fwrite(&chunk_header, sizeof(bra_io_chunk_header_t), 1, tmpfile.f) != 1)
        {
            bra_log_error("unable to write primary index to %s", tmpfile.fn);
            goto BRA_IO_FILE_COMPRESS_FILE_CHUNKS_ERR;
        }

        // write source chunk
        if (fwrite(buf_huffman->data, sizeof(char), buf_huffman->meta.encoded_size, tmpfile.f) != buf_huffman->meta.encoded_size)
            goto BRA_IO_FILE_COMPRESS_FILE_CHUNKS_ERR;

        free(buf_bwt);
        buf_bwt = NULL;
        free(buf_mtf);
        buf_mtf = NULL;
        bra_huffman_chunk_free(buf_huffman);
        buf_huffman = NULL;

        i += s;
    }

    // Check if the tmpfile is smaller than original file:
    const int64_t tmpfile_size = bra_io_file_tell(&tmpfile);
    if (tmpfile_size < 0)
        goto BRA_IO_FILE_COMPRESS_FILE_CHUNKS_ERR;

    bool res = true;
    if ((uint64_t) tmpfile_size >= data_size)
    {
        res            = false;
        me->attributes = BRA_ATTR_SET_COMP(me->attributes, BRA_ATTR_COMP_STORED);
    }
    else
    {
        if (!bra_io_file_seek(&tmpfile, 0, SEEK_SET))
            goto BRA_IO_FILE_COMPRESS_FILE_CHUNKS_ERR;

        // update file size
        bra_meta_entry_file_t* mef = (bra_meta_entry_file_t*) me->entry_data;
        mef->data_size             = tmpfile_size;
        me->crc32                  = bra_crc32c(&mef->data_size, sizeof(uint64_t), me->crc32);
        if (fwrite(&mef->data_size, sizeof(uint64_t), 1, dst->f) != 1)
            goto BRA_IO_FILE_COMPRESS_FILE_CHUNKS_ERR;

        res = bra_io_file_copy_file_chunks(dst, &tmpfile, tmpfile_size, me);
    }

    bra_io_file_close(&tmpfile);
    return res;

BRA_IO_FILE_COMPRESS_FILE_CHUNKS_ERR:
    bra_io_file_close(&tmpfile);
    if (buf_bwt != NULL)
        free(buf_bwt);
    if (buf_mtf != NULL)
        free(buf_mtf);

    bra_huffman_chunk_free(buf_huffman);

    bra_io_file_close(dst);
    bra_io_file_close(src);
    return false;
}

bool bra_io_file_decompress_file_chunks(bra_io_file_t* dst, bra_io_file_t* src, const uint64_t data_size, bra_meta_entry_t* me)
{
    assert_bra_io_file_t(dst);
    assert_bra_io_file_t(src);
    assert(me != NULL);

    uint8_t         buf[BRA_MAX_CHUNK_SIZE];
    uint8_t         buf_bwt[BRA_MAX_CHUNK_SIZE];
    uint8_t         buf_mtf[BRA_MAX_CHUNK_SIZE];
    bra_bwt_index_t buf_trans[BRA_MAX_CHUNK_SIZE];
    uint8_t*        buf_huffman = NULL;

    for (uint64_t i = 0; i < data_size;)
    {
        // read chunk header
        bra_io_chunk_header_t chunk_header = {.primary_index = 0};
        if (!bra_io_file_read_chunk_header(src, &chunk_header))
            goto BRA_IO_FILE_DECOMPRESS_FILE_CHUNKS_ERR;

        // sanity checks
        if (chunk_header.huffman.encoded_size > BRA_MAX_CHUNK_SIZE)
        {
            bra_log_error("encoded chunk size (%" PRIu32 ") exceeds maximum (%" PRIu32 ") in %s",
                          chunk_header.huffman.encoded_size,
                          (uint32_t) BRA_MAX_CHUNK_SIZE,
                          src->fn);
            goto BRA_IO_FILE_DECOMPRESS_FILE_CHUNKS_ERR;
        }
        // ---

        if (chunk_header.huffman.orig_size > BRA_MAX_CHUNK_SIZE)
        {
            bra_log_error("decoded chunk size (%" PRIu32 ") exceeds maximum (%" PRIu32 ") in %s",
                          chunk_header.huffman.orig_size,
                          (uint32_t) BRA_MAX_CHUNK_SIZE,
                          src->fn);
            goto BRA_IO_FILE_DECOMPRESS_FILE_CHUNKS_ERR;
        }

        // read source chunk
        if (!bra_io_file_read_chunk(src, buf, chunk_header.huffman.encoded_size))
            goto BRA_IO_FILE_DECOMPRESS_FILE_CHUNKS_ERR;

        // decode huffman
        uint32_t s  = 0;
        buf_huffman = bra_huffman_decode(&chunk_header.huffman, buf, &s);
        if (buf_huffman == NULL)
        {
            bra_log_error("unable to decode huffman file: %s ", src->fn);
            goto BRA_IO_FILE_DECOMPRESS_FILE_CHUNKS_ERR;
        }

        if (chunk_header.primary_index >= s)
        {
            bra_log_error("invalid primary index (%u) for chunk size %" PRIu32 " in %s", chunk_header.primary_index, s, src->fn);
            goto BRA_IO_FILE_DECOMPRESS_FILE_CHUNKS_ERR;
        }

        // decompress MTF+BWT
        bra_mtf_decode2(buf_huffman, s, buf_mtf);
        bra_bwt_decode2(buf_mtf, s, chunk_header.primary_index, buf_trans, buf_bwt);


        // write source chunk
        if (fwrite(buf_bwt, sizeof(char), s, dst->f) != s)
            goto BRA_IO_FILE_DECOMPRESS_FILE_CHUNKS_ERR;

        // update CRC32
        // TODO: this is the same as the test function.
        // TODO2: actually the test function is basically a copy of this function
        //        without the above write, so those can be joined into 1 function with a bool test argument for instance.
        me->crc32 = bra_crc32c(&chunk_header, sizeof(bra_io_chunk_header_t), me->crc32);
        // me->crc32 = bra_crc32c(buf_bwt, s, me->crc32);
        me->crc32 = bra_crc32c(buf, chunk_header.huffman.encoded_size, me->crc32);

        free(buf_huffman);
        buf_huffman = NULL;

        i += s;
    }

    return true;

BRA_IO_FILE_DECOMPRESS_FILE_CHUNKS_ERR:
    bra_io_file_close(dst);
    bra_io_file_close(src);
    if (buf_huffman != NULL)
        free(buf_huffman);

    return false;
}
