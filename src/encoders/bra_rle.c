#include <encoders/bra_rle.h>

#include <log/bra_log.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

// _Static_assert(sizeof(int) >= sizeof(bra_rle_counts_t), "review for loop with int k");

// static bra_rle_chunk_t* bra_encode_rle_alloc_node(uint64_t* num_rle_chunks, bra_rle_chunk_t* cur_rle)
// {
//     assert(num_rle_chunks != NULL);

// bra_rle_chunk_t* e = calloc(1, sizeof(bra_rle_chunk_t));
// if (e == NULL)
//     return NULL;

// if (cur_rle != NULL)
//     cur_rle->pNext = e;
// ++(*num_rle_chunks);
// return e;
// }

// bool bra_encode_rle(const uint8_t* buf, const size_t buf_size, uint64_t* num_rle_chunks, bra_rle_chunk_t** chunk_head)
// {
//     assert(buf != NULL);
//     assert(num_rle_chunks != NULL);

// if (buf_size == 0)
//     return false;

// uint64_t         num_chunks = *num_rle_chunks;
// bra_rle_chunk_t* rle        = *chunk_head;
// bra_rle_chunk_t* prev;
// size_t           i;

// if (rle == NULL)
// {
//     rle = bra_encode_rle_alloc_node(&num_chunks, NULL);
//     if (rle == NULL)
//         return false;

// *chunk_head = rle;
// rle->counts = 0;
// rle->value  = buf[0];
// i           = 1;
// }
// else
// {
// // find last element
// i = 0;
// while (rle->pNext != NULL)
// {
//     rle = rle->pNext;
// }
// }

// for (; i < buf_size; ++i)
// {
//     if (buf[i] == rle->value)
//     {
//         if (rle->counts == BRA_MAX_RLE_COUNTS)
//         {
//             prev = rle;
//             assert(prev->pNext == NULL);
//             rle = bra_encode_rle_alloc_node(&num_chunks, prev);
//             if (rle == NULL)
//                 return false;

// rle->value = prev->value;
// }
// else
// rle->counts++;
// }
// else
// {
// prev = rle;
// assert(prev->pNext == NULL);
// rle = bra_encode_rle_alloc_node(&num_chunks, prev);
// if (rle == NULL)
// return false;

// rle->value = buf[i];
// }
// }

// *num_rle_chunks = num_chunks;
// return true;
// }

// bool bra_decode_rle(bra_rle_chunk_t** cur_rle, uint8_t* buf, const size_t buf_size, size_t* buf_i)
// {
//     assert(cur_rle != NULL);
//     assert(buf != NULL);
//     assert(buf_i != NULL);

// if (*buf_i >= buf_size)
//     return false;    // this is an error and it might be hidden

// size_t           i   = *buf_i;
// bra_rle_chunk_t* rle = *cur_rle;

// if (rle == NULL)
//     return false;

// while (rle != NULL)
// {
//     // NOTE: rle_data.counts==0 means 1 time
//     for (int k = 0; k <= (int) rle->counts; ++k)
//     {
//         if (i < buf_size)
//             buf[i++] = rle->value;
//         else
//         {
//             rle->counts -= (uint8_t) k;
//             goto BRA_DECODE_RLE_END;
//         }
//     }

// rle = rle->pNext;
// }

// BRA_DECODE_RLE_END:
//     *buf_i   = i;
//     *cur_rle = rle;
//     return true;
// }

// bool bra_encode_rle_free_list(bra_rle_chunk_t** rle_head)
// {
//     if (rle_head == NULL)
//         return false;

// bra_rle_chunk_t* rle = *rle_head;
// while (rle != NULL)
// {
//     bra_rle_chunk_t* tmp = rle;
//     rle                  = rle->pNext;
//     free(tmp);
// }

// *rle_head = NULL;
// return true;
// }

static inline size_t _bra_rle_encoding_compute_size_detect_run(const uint8_t* buf, const size_t buf_size, const size_t i)
{
    assert(buf != NULL);

    size_t run = 1;
    while (i + run < buf_size && buf[i + run] == buf[i] && run < BRA_RLE_MAX_RUNS)
        ++run;

    return run;
}

static inline size_t _bra_rle_encode_compute_size(const uint8_t* buf, const size_t buf_size)
{
    assert(buf != NULL);

    size_t size = 0;
    for (size_t i = 0; i < buf_size;)
    {
        // counting repeated bytes
        const size_t run = _bra_rle_encoding_compute_size_detect_run(buf, buf_size, i);
        if (run >= BRA_RLE_MIN_RUNS)
        {
            // run block
            size += 2;
            i    += run;
        }
        else
        {
            // literal block
            // 1st time already checked that is a literal run
            size_t lit_size  = run;
            i               += run;
            while (i < buf_size)
            {
                if (_bra_rle_encoding_compute_size_detect_run(buf, buf_size, i) >= BRA_RLE_MIN_RUNS)
                    break;

                ++i;
                if (++lit_size == BRA_RLE_MAX_RUNS)
                    break;
            }

            size += 1 + lit_size;
        }
    }

    return size;
}

static inline size_t _bra_rle_decode_compute_size(const uint8_t* buf, const size_t buf_size)
{
    assert(buf != NULL);

    size_t size = 0;
    for (size_t i = 0; i < buf_size;)
    {
        const int8_t control = buf[i++];
        if (control >= 0)
        {
            // literal block
            const int count = control + 1;

            // safety check
            if (i + count > buf_size)
                return 0;    // error

            size += count;
            i    += count;
        }
        else if (control >= BRA_RLE_CTL_RUNS)    // ?
        {
            // run block
            const int count = 1 - control;

            // safety check
            if (i == buf_size)
                return 0;    // error

            size += count;
            ++i;
        }
        else
        {
            // control == -128
        }
    }

    return size;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool bra_rle_encode(const uint8_t* buf, const size_t buf_size, uint8_t** out_buf, size_t* out_buf_size)
{
    assert(buf != NULL);
    assert(buf_size > 0);
    assert(out_buf != NULL);
    assert(out_buf_size != NULL);

    // estimate the out_buf_size doing a pre-scan of the data
    *out_buf_size = 0;
    *out_buf      = NULL;

    size_t   s = _bra_rle_encode_compute_size(buf, buf_size);
    uint8_t* b = malloc(sizeof(uint8_t) * s);
    if (b == NULL)
        return false;

    // encode the data
    uint8_t* p = b;
    for (size_t i = 0; i < buf_size;)
    {
        const size_t run = _bra_rle_encoding_compute_size_detect_run(buf, buf_size, i);
        if (run >= BRA_RLE_MIN_RUNS)
        {
            // run block
            const int8_t control  = -(run - 1);
            *p++                  = control;
            *p++                  = buf[i];
            i                    += run;
        }
        else
        {
            // literal block
            size_t  lit_start   = i;
            uint8_t lit_length  = run;
            i                  += run;
            while (i < buf_size)
            {
                if (_bra_rle_encoding_compute_size_detect_run(buf, buf_size, i) > BRA_RLE_MIN_RUNS)
                    break;

                ++i;
                if (++lit_length == BRA_RLE_MAX_RUNS)
                    break;
            }

            *p++ = (lit_length - 1);
            memcpy(p, &buf[lit_start], lit_length);
            p += lit_length;
        }
    }

    *out_buf_size = s;
    *out_buf      = b;
    return true;
}

bool bra_rle_decode(const uint8_t* buf, const size_t buf_size, uint8_t** out_buf, size_t* out_buf_size)
{
    assert(buf != NULL);
    assert(out_buf != NULL);
    assert(out_buf_size != NULL);

    *out_buf      = NULL;
    *out_buf_size = 0;

    // estimate length
    const size_t s = _bra_rle_decode_compute_size(buf, buf_size);
    if (s == 0)
        return false;

    uint8_t* b = malloc(sizeof(uint8_t) * s);
    if (b == NULL)
        return false;

    uint8_t* p = b;


    // decode the data
    for (size_t i = 0; i < buf_size;)
    {
        const int8_t control = (int8_t) buf[i++];
        if (control >= 0)
        {
            // literal block
            const int count = control + 1;

            // safety check can be skipped
            // as it has been performed already when determine the out_buf_size
            if (i + count > buf_size)
                goto BRA_RLE_DECODE_ERROR;

            memcpy(p, &buf[i], count);
            i += count;
            p += count;
        }
        else if (control >= BRA_RLE_CTL_RUNS)
        {
            // run block
            const int count = 1 - control;
            if (i >= buf_size)
                goto BRA_RLE_DECODE_ERROR;

            const uint8_t v = buf[i++];
            memset(p, v, count);
            p += count;
        }
        else
        {
        }
    }

    *out_buf_size = s;
    *out_buf      = b;
    return true;

BRA_RLE_DECODE_ERROR:
    free(b);
    return false;
}
