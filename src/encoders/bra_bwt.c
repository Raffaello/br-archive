#if defined(linux) || defined(__BSD__)
#define _GNU_SOURCE
#define BRA_QSORT qsort_r
#else
#define BRA_QSORT qsort_s
#endif

#include <encoders/bra_bwt.h>
#include <lib_bra_defs.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

typedef struct bwt_suffix_ctx_t
{
    size_t*        index;
    const uint8_t* data;
    const size_t   length;

} bwt_suffix_ctx_t;

//////////////////////////////////////////////////////////////////////////////////////////

#if defined(linux) || defined(__BSD__)
static int bwt_suffix_context_compare(const void* a, const void* b, void* context)
#else
static int bwt_suffix_context_compare(void* context, const void* a, const void* b)
#endif
{
    const bwt_suffix_ctx_t* ctx = (const bwt_suffix_ctx_t*) context;

    const size_t sa = *(const size_t*) a;
    const size_t sb = *(const size_t*) b;

    for (size_t i = 0; i < ctx->length; ++i)
    {
        const uint8_t byte_a = ctx->data[(sa + i) % ctx->length];
        const uint8_t byte_b = ctx->data[(sb + i) % ctx->length];

        if (byte_a < byte_b)
            return -1;
        if (byte_a > byte_b)
            return 1;
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////

uint8_t* bra_bwt_encode(const uint8_t* buf, const size_t buf_size, size_t* primary_index)
{
    assert(buf != NULL);
    assert(buf_size > 0);
    assert(primary_index != NULL);

    // Allocate suffix array for all rotations
    bwt_suffix_ctx_t suffix_ctx = {.index = NULL, .data = buf, .length = buf_size};
    suffix_ctx.index            = malloc(buf_size * sizeof(size_t));
    if (!suffix_ctx.index)
        return NULL;

    // Initialize suffix array with all possible rotations
    for (size_t i = 0; i < buf_size; i++)
        suffix_ctx.index[i] = i;

    // Sort all rotations lexicographically
    BRA_QSORT(suffix_ctx.index, buf_size, sizeof(size_t), bwt_suffix_context_compare, &suffix_ctx);

    // Allocate output buffer
    uint8_t* out_buf = (uint8_t*) malloc(buf_size);
    if (out_buf == NULL)
        goto BRA_BWT_ENCODE_EXIT;

    // Generate BWT by taking the last character of each sorted rotation
    *primary_index = 0;
    for (size_t i = 0; i < buf_size; i++)
    {
        // Last character position in this rotation
        size_t last_pos = (suffix_ctx.index[i] + buf_size - 1) % buf_size;
        out_buf[i]      = buf[last_pos];

        // Track where the original string (rotation starting at 0) ended up
        if (suffix_ctx.index[i] == 0)
            *primary_index = i;
    }

BRA_BWT_ENCODE_EXIT:
    free(suffix_ctx.index);
    return out_buf;
}

uint8_t* bra_bwt_decode(const uint8_t* buf, const size_t buf_size, const size_t primary_index)
{
    assert(buf != NULL);
    assert(buf_size > 0);
    assert(primary_index < buf_size);

    // Count character frequencies
    size_t count[BRA_ALPHABET_SIZE] = {0};
    for (size_t i = 0; i < buf_size; i++)
        count[buf[i]]++;

    // Calculate first occurrence positions (cumulative counts)
    size_t first_occurrence[BRA_ALPHABET_SIZE] = {0};
    for (int i = 1; i < BRA_ALPHABET_SIZE; i++)
        first_occurrence[i] = first_occurrence[i - 1] + count[i - 1];

    // Build transform array (maps last column positions to first column positions)
    size_t* transform = malloc(buf_size * sizeof(size_t));
    if (!transform)
        return NULL;

    // Create the transform mapping using first occurrence positions
    size_t temp_first[BRA_ALPHABET_SIZE];
    memcpy(temp_first, first_occurrence, BRA_ALPHABET_SIZE * sizeof(size_t));

    for (size_t i = 0; i < buf_size; i++)
    {
        const uint8_t c            = buf[i];
        transform[temp_first[c]++] = i;
    }

    // Allocate output buffer
    uint8_t* out_buf = (uint8_t*) malloc(buf_size);
    if (out_buf == NULL)
        goto BRA_BWT_DECODE_EXIT;

    // Follow the transform chain starting from primary index to reconstruct original string
    size_t index = primary_index;
    for (size_t i = 0; i < buf_size; i++)
    {
        index      = transform[index];
        out_buf[i] = buf[index];
    }

BRA_BWT_DECODE_EXIT:
    free(transform);
    return out_buf;
}
