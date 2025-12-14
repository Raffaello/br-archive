#include "lib_bra_io_file_chunks.h"

#include <lib_bra_private.h>
#include <lib_bra_defs.h>

#include <io/lib_bra_io_chunk_header.h>    // todo incorporate it in here
#include <io/lib_bra_io_file.h>
#include <log/bra_log.h>
#include <utils/lib_bra_crc32c.h>

#include <encoders/bra_bwt.h>
#include <encoders/bra_mtf.h>
#include <encoders/bra_huffman.h>

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static inline bool bra_io_file_read_file_chunks_stored(bra_io_file_t* src, const uint64_t data_size, bra_meta_entry_t* me)
{
    char buf[BRA_MAX_CHUNK_SIZE];

    for (uint64_t i = 0; i < data_size;)
    {
        const uint32_t s = _bra_min(BRA_MAX_CHUNK_SIZE, data_size - i);

        // read source chunk
        if (!bra_io_file_chunks_read(src, buf, s))
            return false;

        // update CRC32
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
        if (!bra_io_file_chunks_read_chunk_header(src, &chunk_header))
            return false;

        if (!bra_io_chunk_header_validate(&chunk_header))
        {
            bra_log_error("chunk header in %s is not valid", src->fn);
            goto BRA_IO_FILE_READ_FILE_CHUNKS_COMPRESSED_ERROR;
        }

        // read source chunk
        if (!bra_io_file_chunks_read(src, buf, chunk_header.huffman.encoded_size))
            return false;

        // decode huffman
        uint32_t s  = 0;
        buf_huffman = bra_huffman_decode(&chunk_header.huffman, buf, &s);
        if (buf_huffman == NULL)
        {
            bra_log_error("unable to decode huffman file: %s ", src->fn);
            goto BRA_IO_FILE_READ_FILE_CHUNKS_COMPRESSED_ERROR;
        }

        if (chunk_header.primary_index >= s)
        {
            bra_log_error("invalid primary index (%" PRIu32 ") for chunk size %" PRIu32 " in %s", chunk_header.primary_index, s, src->fn);
            goto BRA_IO_FILE_READ_FILE_CHUNKS_COMPRESSED_ERROR;
        }

        bra_mtf_decode2((uint8_t*) buf_huffman, s, buf_mtf);
        bra_bwt_decode2(buf_mtf, s, chunk_header.primary_index, buf_transform, buf_bwt);

        // TODO: it would be better on the uncompressed data to compute CRC32.
        //       this must be reviewed.
        me->crc32 = bra_crc32c(&chunk_header, sizeof(bra_io_chunk_header_t), me->crc32);
        me->crc32 = bra_crc32c(buf, chunk_header.huffman.encoded_size, me->crc32);

        free(buf_huffman);
        buf_huffman = NULL;

        i += chunk_header.huffman.encoded_size + sizeof(bra_io_chunk_header_t);
    }

    return true;

BRA_IO_FILE_READ_FILE_CHUNKS_COMPRESSED_ERROR:
    if (buf_huffman != NULL)
    {
        free(buf_huffman);
        buf_huffman = NULL;
    }

    bra_io_file_close(src);
    return false;
}

/////////////////////////////////////////////////////////////////////////

bool bra_io_file_chunks_read_chunk_header(bra_io_file_t* src, bra_io_chunk_header_t* chunk_header)
{
    assert_bra_io_file_t(src);
    assert(chunk_header != NULL);

    if (fread(chunk_header, sizeof(bra_io_chunk_header_t), 1, src->f) != 1)
    {
        bra_log_error("unable to read chunk header from %s", src->fn);
        bra_io_file_read_error(src);
        return false;
    }

    return true;
}

bool bra_io_file_chunks_write_chunk_header(bra_io_file_t* dst, const bra_io_chunk_header_t* chunk_header)
{
    assert_bra_io_file_t(dst);
    assert(chunk_header != NULL);

    if (fwrite(chunk_header, sizeof(bra_io_chunk_header_t), 1, dst->f) != 1)
    {
        bra_log_error("unable to write chunk header to %s", dst->fn);
        bra_io_file_write_error(dst);
        return false;
    }

    return true;
}

bool bra_io_file_chunks_read(bra_io_file_t* src, void* buf, const size_t buf_size)
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

bool bra_io_file_chunks_read_file(bra_io_file_t* src, const uint64_t data_size, bra_meta_entry_t* me)
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

bool bra_io_file_chunks_copy_file(bra_io_file_t* dst, bra_io_file_t* src, const uint64_t data_size, bra_meta_entry_t* me)
{
    assert_bra_io_file_t(dst);
    assert_bra_io_file_t(src);
    assert(me != NULL);

    char buf[BRA_MAX_CHUNK_SIZE];

    for (uint64_t i = 0; i < data_size;)
    {
        const uint32_t s = _bra_min(BRA_MAX_CHUNK_SIZE, data_size - i);

        // read source chunk
        if (!bra_io_file_chunks_read(src, buf, s))
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

bool bra_io_file_chunks_compress_file(bra_io_file_t* dst, bra_io_file_t* src, const uint64_t data_size, bra_meta_entry_t* me)
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
        if (!bra_io_file_chunks_read(src, buf, s))
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
            bra_io_file_write_error(&tmpfile);
            goto BRA_IO_FILE_COMPRESS_FILE_CHUNKS_ERR;
        }

        // write source chunk
        if (fwrite(buf_huffman->data, sizeof(char), buf_huffman->meta.encoded_size, tmpfile.f) != buf_huffman->meta.encoded_size)
        {
            bra_io_file_write_error(&tmpfile);
            goto BRA_IO_FILE_COMPRESS_FILE_CHUNKS_ERR;
        }

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
        {
            bra_io_file_write_error(dst);
            goto BRA_IO_FILE_COMPRESS_FILE_CHUNKS_ERR;
        }

        res = bra_io_file_chunks_copy_file(dst, &tmpfile, tmpfile_size, me);
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

bool bra_io_file_chunks_decompress_file(bra_io_file_t* dst, bra_io_file_t* src, const uint64_t data_size, bra_meta_entry_t* me)
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
        if (!bra_io_file_chunks_read_chunk_header(src, &chunk_header))
            goto BRA_IO_FILE_DECOMPRESS_FILE_CHUNKS_ERR;

        if (!bra_io_chunk_header_validate(&chunk_header))
        {
            bra_log_error("chunk header not valid in %s", src->fn);
            goto BRA_IO_FILE_DECOMPRESS_FILE_CHUNKS_ERR;
        }

        // read source chunk
        if (!bra_io_file_chunks_read(src, buf, chunk_header.huffman.encoded_size))
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
        {
            bra_io_file_write_error(dst);
            goto BRA_IO_FILE_DECOMPRESS_FILE_CHUNKS_ERR;
        }

        // update CRC32
        // TODO: this is the same as the test function.
        // TODO2: actually the test function is basically a copy of this function
        //        without the above write, so those can be joined into 1 function with a bool test argument for instance.
        me->crc32 = bra_crc32c(&chunk_header, sizeof(bra_io_chunk_header_t), me->crc32);
        // me->crc32 = bra_crc32c(buf_bwt, s, me->crc32);
        me->crc32 = bra_crc32c(buf, chunk_header.huffman.encoded_size, me->crc32);

        free(buf_huffman);
        buf_huffman = NULL;

        i += chunk_header.huffman.encoded_size + sizeof(bra_io_chunk_header_t);
    }

    return true;

BRA_IO_FILE_DECOMPRESS_FILE_CHUNKS_ERR:
    bra_io_file_close(dst);
    bra_io_file_close(src);
    if (buf_huffman != NULL)
        free(buf_huffman);

    return false;
}
