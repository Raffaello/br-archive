#include <encoders/bra_rle.h>

#include <log/bra_log.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

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
        else if (control >= BRA_RLE_CTL_RUNS)
        {
            // run block
            const int count = 1 - control;

            // safety check
            if (i >= buf_size)
                return 0;    // error

            size += count;
            ++i;
        }
        else    // control == -128 (no-op)
        {
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

    size_t s = _bra_rle_encode_compute_size(buf, buf_size);
    if (s == 0)
        return false;

    // if (s > buf_size)
    //     return false;

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
                if (_bra_rle_encoding_compute_size_detect_run(buf, buf_size, i) >= BRA_RLE_MIN_RUNS)
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
        else    // control == -128 (no-op)
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
