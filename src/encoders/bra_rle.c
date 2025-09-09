#include "../lib_bra.h"

#include <bra_log.h>

#include <assert.h>
#include <stdlib.h>

static bool bra_encode_rle_realloc(size_t* num_rle_chunks, bra_rle_chunk_t* out_rle_data[])
{
    assert(num_rle_chunks != NULL);
    assert(out_rle_data != NULL);

    size_t           num_chunks = *num_rle_chunks;
    bra_rle_chunk_t* new_rle    = NULL;

    if (num_chunks == SIZE_MAX)
    {
        bra_log_critical("unable to encode RLA, out of chunks");
        return false;
    }

    ++num_chunks;
    new_rle = realloc(*out_rle_data, sizeof(bra_rle_chunk_t) * num_chunks);
    if (new_rle == NULL)
    {
        bra_log_critical("RLE encoding out of memory");
        return false;
    }

    *out_rle_data   = new_rle;
    *num_rle_chunks = num_chunks;
    return true;
}

static bool bra_encode_rle_next(size_t* j_, size_t* num_rle_chunks, bra_rle_chunk_t* out_rle_data[])
{
    assert(j_ != NULL);
    assert(num_rle_chunks != NULL);
    assert(out_rle_data != NULL);

    size_t           num_chunks = *num_rle_chunks;
    bra_rle_chunk_t* rle        = *out_rle_data;
    size_t           j          = *j_;

    // need another chunk
    ++j;
    if (j == num_chunks)
    {
        if (!bra_encode_rle_realloc(&num_chunks, &rle))
            return false;
    }

    assert(j < num_chunks);
    // rle[j].value  = rle[j - 1].value;
    rle[j].counts   = 0;    // this is just counting as 1
    *j_             = j;
    *num_rle_chunks = num_chunks;
    return true;
}

bool bra_encode_rle(const char* buf, const size_t buf_size, size_t* num_rle_chunks, bra_rle_chunk_t* out_rle_data[])
{
    assert(buf != NULL);
    assert(num_rle_chunks != NULL);
    assert(out_rle_data != NULL);

    size_t           num_chunks = *num_rle_chunks;
    bra_rle_chunk_t* rle        = *out_rle_data;
    size_t           i;
    size_t           j;

    if (num_chunks > 0)
    {
        // starting from the first in the buffer, and the last of the chunks.
        i = 0;
        j = num_chunks - 1;
    }
    else
    {
        if (!bra_encode_rle_realloc(&num_chunks, &rle))
            goto BRA_RLE_ENCODING_ERROR;

        rle[0] = (bra_rle_chunk_t) {.counts = 0, .value = buf[0]};
        i      = 1;
        j      = 0;
    }

    for (; i < buf_size; ++i)
    {
        if (buf[i] == rle[j].value)
        {
            if (rle[j].counts == BRA_MAX_RLE_COUNTS)
            {
                // need another chunk
                if (!bra_encode_rle_next(&j, &num_chunks, &rle))
                    goto BRA_RLE_ENCODING_ERROR;

                rle[j].value = rle[j - 1].value;
            }
            else
                rle[j].counts++;
        }
        else
        {
            // it is a different char, going to the next chunk
            if (!bra_encode_rle_next(&j, &num_chunks, &rle))
                goto BRA_RLE_ENCODING_ERROR;

            rle[j].value = buf[i];
        }
    }

    *out_rle_data   = rle;
    *num_rle_chunks = num_chunks;
    return true;

BRA_RLE_ENCODING_ERROR:
    if (*out_rle_data != NULL)
    {
        free(*out_rle_data);
        *out_rle_data = NULL;
    }
    *num_rle_chunks = 0;
    return false;
}

bool bra_decode_rle(const size_t num_rle_chunks, const bra_rle_chunk_t rle_data[], char* buf, const size_t buf_size)
{
    // TODO
    return false;
}