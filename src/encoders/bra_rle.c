#include "../lib_bra.h"

#include <bra_log.h>

#include <assert.h>
#include <stdlib.h>

static bra_rle_chunk_t* bra_encode_rle_alloc_node(size_t* num_rle_chunks, bra_rle_chunk_t* cur)
{
    assert(num_rle_chunks != NULL);

    bra_rle_chunk_t* e = calloc(1, sizeof(bra_rle_chunk_t));
    if (e == NULL)
        return NULL;

    if (cur != NULL)
        cur->pNext = e;
    ++(*num_rle_chunks);
    return e;
}

bool bra_encode_rle(const char* buf, const size_t buf_size, size_t* num_rle_chunks, bra_rle_chunk_t** chunk_head)
{
    assert(buf != NULL);
    assert(num_rle_chunks != NULL);

    size_t           num_chunks = *num_rle_chunks;
    bra_rle_chunk_t* rle        = *chunk_head;
    bra_rle_chunk_t* prev;
    size_t           i;


    if (rle == NULL)
    {
        prev = NULL;
        rle  = bra_encode_rle_alloc_node(&num_chunks, prev);
        assert(prev == NULL);
        if (rle == NULL)
            return false;

        *chunk_head = rle;
        rle->counts = 0;
        rle->value  = buf[0];
        i           = 1;
    }
    else
    {
        i = 0;
        do
        {
            prev = rle;
            rle  = rle->pNext;
        }
        while (rle != NULL);
    }

    for (; i < buf_size; ++i)
    {
        if (buf[i] == rle->value)
        {
            if (rle->counts == BRA_MAX_RLE_COUNTS)
            {
                prev = rle;
                assert(prev->pNext == NULL);
                rle = bra_encode_rle_alloc_node(&num_chunks, prev);
                if (rle == NULL)
                    return false;

                rle->value = prev->value;
            }
            else
                rle->counts++;
        }
        else
        {
            prev = rle;
            assert(prev->pNext == NULL);
            rle = bra_encode_rle_alloc_node(&num_chunks, prev);
            if (rle == NULL)
                return false;

            rle->value = buf[i];
        }
    }

    *num_rle_chunks = num_chunks;
    return true;
}

bool bra_decode_rle(bra_rle_chunk_t** cur, char* buf, const size_t buf_size, size_t* buf_i)
{
    assert(cur != NULL);
    assert(buf != NULL);
    assert(buf_i != NULL);


    if (*buf_i >= buf_size)
        return false;

    size_t           i   = *buf_i;
    bra_rle_chunk_t* rle = *cur;

    if (rle == NULL)
        return false;

    while (rle != NULL)
    {
        // NOTE: rle_data.counts==0 means 1 time
        for (int k = 0; k <= rle->counts; ++k)
        {
            if (i < buf_size)
                buf[i++] = rle->value;
            else
            {
                rle->counts -= k;
                goto BRA_DECODE_RLE_END;
            }
        }

        rle = rle->pNext;
    }

BRA_DECODE_RLE_END:
    *buf_i = i;
    *cur   = rle;
    return true;
}

bool bra_encode_rle_free_list(bra_rle_chunk_t** rle_head)
{
    if (rle_head == NULL)
        return false;

    bra_rle_chunk_t* rle = *rle_head;
    while (rle != NULL)
    {
        bra_rle_chunk_t* tmp = rle;
        rle                  = rle->pNext;
        free(tmp);
    }

    *rle_head = NULL;
    return true;
}

////////////////////////

static bool bra_encode_rle_array_realloc(size_t* num_rle_chunks, bra_rle_chunk_t* out_rle_data[])
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

static bool bra_encode_rle_array_next(size_t* j_, size_t* num_rle_chunks, bra_rle_chunk_t* out_rle_data[])
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
        if (!bra_encode_rle_array_realloc(&num_chunks, &rle))
            return false;
    }

    assert(j < num_chunks);
    rle[j].counts   = 0;    // this is just counting as 1
    *j_             = j;
    *num_rle_chunks = num_chunks;
    *out_rle_data   = rle;
    return true;
}

bool bra_encode_rle_array(const char* buf, const size_t buf_size, size_t* num_rle_chunks, bra_rle_chunk_t* out_rle_data[])
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
        if (!bra_encode_rle_array_realloc(&num_chunks, &rle))
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
                if (!bra_encode_rle_array_next(&j, &num_chunks, &rle))
                    goto BRA_RLE_ENCODING_ERROR;

                rle[j].value = rle[j - 1].value;
            }
            else
                rle[j].counts++;
        }
        else
        {
            // it is a different char, going to the next chunk
            if (!bra_encode_rle_array_next(&j, &num_chunks, &rle))
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

bool bra_decode_rle_array(const size_t num_rle_chunks, size_t* cur_rle_chunk, bra_rle_chunk_t rle_data[], char* buf, const size_t buf_size, size_t* buf_i)
{
    assert(rle_data != NULL);
    assert(buf != NULL);
    assert(cur_rle_chunk != NULL);
    assert(buf_i != NULL);

    if (num_rle_chunks == 0 || *cur_rle_chunk >= num_rle_chunks || *buf_i >= buf_size)
        return false;

    size_t j = *cur_rle_chunk;
    size_t i = *buf_i;

    for (; j < num_rle_chunks; ++j)
    {
        // NOTE: rle_data.counts==0 means 1 time
        for (int k = 0; k <= rle_data[j].counts; ++k)
        {
            if (i < buf_size)
                buf[i++] = rle_data[j].value;
            else
            {
                rle_data[j].counts -= k;
                goto BRA_DECODE_RLE_END;
            }
        }
    }


BRA_DECODE_RLE_END:
    *cur_rle_chunk = j;
    *buf_i         = i;
    return true;
}
