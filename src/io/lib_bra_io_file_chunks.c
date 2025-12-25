#include "lib_bra_io_file_chunks.h"

#include <lib_bra_private.h>
#include <lib_bra_defs.h>

#include <io/lib_bra_io_file.h>
#include <io/lib_bra_io_file_meta_entries.h>
#include <log/bra_log.h>
#include <utils/lib_bra_crc32c.h>

#include <encoders/bra_bwt.h>
#include <encoders/bra_mtf.h>
#include <encoders/bra_huffman.h>

#include <inttypes.h>
#include <stdlib.h>
#include <assert.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static bool bra_io_file_chunks_header_validate(const bra_io_chunk_header_t* chunk_header)
{
    if (chunk_header == NULL)
        return false;

    if (chunk_header->huffman.encoded_size > BRA_MAX_CHUNK_SIZE)
        return false;
    if (chunk_header->huffman.orig_size > BRA_MAX_CHUNK_SIZE)
        return false;
    if (chunk_header->huffman.encoded_size == 0)
        return false;
    if (chunk_header->huffman.orig_size == 0)
        return false;

    return true;
}

/////////////////////////////////////////////////////////////////////////

bool bra_io_file_chunks_read_header(bra_io_file_t* src, bra_io_chunk_header_t* chunk_header)
{
    assert_bra_io_file_t(src);
    assert(chunk_header != NULL);

    if (!bra_io_file_read(src, chunk_header, sizeof(bra_io_chunk_header_t)))
    {
        bra_log_error("unable to read chunk header from %s", src->fn);
        return false;
    }

    return true;
}

bool bra_io_file_chunks_write_header(bra_io_file_t* dst, const bra_io_chunk_header_t* chunk_header)
{
    assert_bra_io_file_t(dst);
    assert(chunk_header != NULL);

    if (!bra_io_file_write(dst, chunk_header, sizeof(bra_io_chunk_header_t)))
    {
        bra_log_error("unable to write chunk header to %s", dst->fn);
        return false;
    }

    return true;
}

bool bra_io_file_chunks_read_file(bra_io_file_t* src, const uint64_t data_size, bra_meta_entry_t* me, const bool decode)
{
    assert_bra_io_file_t(src);
    assert(me != NULL);

    switch (BRA_ATTR_COMP(me->attributes))
    {
    case BRA_ATTR_COMP_STORED:
        return bra_io_file_chunks_copy_file(NULL, src, data_size, me, decode);
    case BRA_ATTR_COMP_COMPRESSED:
        return bra_io_file_chunks_decompress_file(NULL, src, data_size, me, decode);
    default:
        bra_log_critical("invalid compression type for file: %u", BRA_ATTR_COMP(me->attributes));
        return false;
    }
}

bool bra_io_file_chunks_copy_file(bra_io_file_t* dst, bra_io_file_t* src, const uint64_t data_size, bra_meta_entry_t* me, const bool compute_crc32)
{
    assert_bra_io_file_t(src);

    if (dst != NULL)
    {
        if (dst->f == NULL || dst->fn == NULL)
            goto BRA_IO_FILE_CHUNKS_COPY_FILE_ERROR;
    }

    if (compute_crc32 && me == NULL)
    {
        bra_log_critical("can't compute crc32: me is null");
        goto BRA_IO_FILE_CHUNKS_COPY_FILE_ERROR;
    }

    char buf[BRA_MAX_CHUNK_SIZE];

    for (uint64_t i = 0; i < data_size;)
    {
        const uint32_t s = _bra_min(BRA_MAX_CHUNK_SIZE, data_size - i);

        // read source chunk
        if (!bra_io_file_read(src, buf, s))
            goto BRA_IO_FILE_CHUNKS_COPY_FILE_ERROR;

        // update CRC32
        if (compute_crc32)
            me->crc32 = bra_crc32c(buf, s, me->crc32);

        // write source chunk
        if (dst != NULL)
        {
            if (!bra_io_file_write(dst, buf, s))
                goto BRA_IO_FILE_CHUNKS_COPY_FILE_ERROR;
        }

        i += s;
    }

    return true;

BRA_IO_FILE_CHUNKS_COPY_FILE_ERROR:
    if (dst != NULL)
        bra_io_file_close(dst);
    bra_io_file_close(src);
    return false;
}

bool bra_io_file_chunks_compress_file(bra_io_file_t* dst, bra_io_file_t* src, const uint64_t data_size, bra_meta_entry_t* me)
{
    assert_bra_io_file_t(dst);
    assert_bra_io_file_t(src);
    assert(me != NULL);

    uint8_t              buf[BRA_MAX_CHUNK_SIZE];
    uint8_t*             buf_bwt     = NULL;
    uint8_t*             buf_mtf     = NULL;
    bra_huffman_chunk_t* buf_huffman = NULL;
    uint32_t             crc32       = BRA_CRC32C_INIT;

    // NOTE: compress a file is done in a temporary file:
    //      if it is smaller than the original file append it to the archive.
    //      otherwise change the attribute to store and redo the whole file
    //      processing including metadata due to CRC32
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
        if (!bra_io_file_read(src, buf, s))
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
        buf_bwt                            = bra_bwt_encode(buf, s, &chunk_header.primary_index);
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

        // CRC32
        crc32 = bra_crc32c(&chunk_header, sizeof(chunk_header), crc32);
        crc32 = bra_crc32c(buf, s, crc32);

        // write chunk header
        if (!bra_io_file_chunks_write_header(&tmpfile, &chunk_header))
            goto BRA_IO_FILE_COMPRESS_FILE_CHUNKS_ERR;

        // write source chunk
        if (!bra_io_file_write(&tmpfile, buf_huffman->data, buf_huffman->meta.encoded_size))
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

        uint64_t num_chunks = (data_size / BRA_MAX_CHUNK_SIZE);
        if (data_size % BRA_MAX_CHUNK_SIZE > 0)
            ++num_chunks;

        // update file size
        bra_meta_entry_file_t* mef = (bra_meta_entry_file_t*) me->entry_data;
        mef->data_size             = tmpfile_size;
        me->crc32                  = bra_crc32c(&tmpfile_size, sizeof(tmpfile_size), me->crc32);
        me->crc32                  = bra_crc32c_combine(me->crc32, crc32, data_size + (num_chunks * sizeof(bra_io_chunk_header_t)));
        if (!bra_io_file_meta_entry_write_file_entry(dst, me))
            goto BRA_IO_FILE_COMPRESS_FILE_CHUNKS_ERR;

        res = bra_io_file_chunks_copy_file(dst, &tmpfile, tmpfile_size, me, false);
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

bool bra_io_file_chunks_decompress_file(bra_io_file_t* dst, bra_io_file_t* src, const uint64_t data_size, bra_meta_entry_t* me, const bool decode)
{
    // TODO: it could be reduce the number of buffers in use
    // with a ping-pong technique

    assert_bra_io_file_t(src);
    assert(me != NULL);

    if (dst != NULL)
    {
        if (dst->f == NULL || dst->fn == NULL)
            goto BRA_IO_FILE_DECOMPRESS_FILE_CHUNKS_ERR;
    }

    uint8_t         buf[BRA_MAX_CHUNK_SIZE];
    uint8_t         buf_bwt[BRA_MAX_CHUNK_SIZE];
    uint8_t         buf_mtf[BRA_MAX_CHUNK_SIZE];
    bra_bwt_index_t buf_trans[BRA_MAX_CHUNK_SIZE];
    uint8_t*        buf_huffman    = NULL;
    uint64_t        file_orig_size = 0;

    for (uint64_t i = 0; i < data_size;)
    {
        // read chunk header
        bra_io_chunk_header_t chunk_header = {.primary_index = 0};
        if (!bra_io_file_chunks_read_header(src, &chunk_header))
            goto BRA_IO_FILE_DECOMPRESS_FILE_CHUNKS_ERR;

        if (!bra_io_file_chunks_header_validate(&chunk_header))
        {
            bra_log_error("chunk header not valid in %s", src->fn);
            goto BRA_IO_FILE_DECOMPRESS_FILE_CHUNKS_ERR;
        }

        file_orig_size += chunk_header.huffman.orig_size;

        // read source chunk
        if (!bra_io_file_read(src, buf, chunk_header.huffman.encoded_size))
            goto BRA_IO_FILE_DECOMPRESS_FILE_CHUNKS_ERR;

        // decode huffman
        if (decode)
        {
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


            // update CRC32
            me->crc32 = bra_crc32c(&chunk_header, sizeof(bra_io_chunk_header_t), me->crc32);
            me->crc32 = bra_crc32c(buf_bwt, s, me->crc32);

            free(buf_huffman);
            buf_huffman = NULL;

            // write source chunk
            if (dst != NULL)
            {
                if (!bra_io_file_write(dst, buf_bwt, s))
                    goto BRA_IO_FILE_DECOMPRESS_FILE_CHUNKS_ERR;
            }
        }


        i += chunk_header.huffman.encoded_size + sizeof(bra_io_chunk_header_t);
    }

    me->_compression_ratio = (float) ((double) data_size / (double) file_orig_size);
    return true;

BRA_IO_FILE_DECOMPRESS_FILE_CHUNKS_ERR:
    if (dst != NULL)
        bra_io_file_close(dst);

    bra_io_file_close(src);
    if (buf_huffman != NULL)
        free(buf_huffman);

    return false;
}
