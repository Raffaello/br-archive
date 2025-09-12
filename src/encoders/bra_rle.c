#include <encoders/bra_rle.h>

#include <log/bra_log.h>

#include <assert.h>
#include <stdlib.h>

static bra_rle_chunk_t* bra_encode_rle_alloc_node(uint64_t* num_rle_chunks, bra_rle_chunk_t* cur)
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

bool bra_encode_rle(const char* buf, const size_t buf_size, uint64_t* num_rle_chunks, bra_rle_chunk_t** chunk_head)
{
    assert(buf != NULL);
    assert(num_rle_chunks != NULL);

    if (buf_size == 0)
        return false;

    size_t           num_chunks = *num_rle_chunks;
    bra_rle_chunk_t* rle        = *chunk_head;
    bra_rle_chunk_t* prev;
    size_t           i;

    if (rle == NULL)
    {
        rle = bra_encode_rle_alloc_node(&num_chunks, NULL);
        if (rle == NULL)
            return false;

        *chunk_head = rle;
        rle->counts = 0;
        rle->value  = buf[0];
        i           = 1;
    }
    else
    {
        // find last element
        i = 0;
        while (rle->pNext != NULL)
        {
            rle = rle->pNext;
        }
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
