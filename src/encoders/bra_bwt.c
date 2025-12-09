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

/**
 * @brief BWT transform helper.
 */
typedef struct bwt_suffix_ctx_t
{
    bra_bwt_index_t*      index;     //!< index
    const uint8_t*        data;      //!< data
    const bra_bwt_index_t length;    //!< length

} bwt_suffix_ctx_t;

//////////////////////////////////////////////////////////////////////////////////////////

#if defined(linux) || defined(__BSD__)
static int bwt_suffix_context_compare(const void* a, const void* b, void* context)
#else
static int bwt_suffix_context_compare(void* context, const void* a, const void* b)
#endif
{
    const bwt_suffix_ctx_t* ctx = (const bwt_suffix_ctx_t*) context;

    const bra_bwt_index_t sa = *(const bra_bwt_index_t*) a;
    const bra_bwt_index_t sb = *(const bra_bwt_index_t*) b;

    for (bra_bwt_index_t i = 0; i < ctx->length; ++i)
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

uint8_t* bra_bwt_encode(const uint8_t* buf, const bra_bwt_index_t buf_size, bra_bwt_index_t* primary_index)
{
    assert(buf != NULL);
    assert(buf_size > 0);
    assert(primary_index != NULL);

    // Allocate suffix array for all rotations
    bwt_suffix_ctx_t suffix_ctx = {.index = NULL, .data = buf, .length = buf_size};
    suffix_ctx.index            = malloc(buf_size * sizeof(bra_bwt_index_t));
    if (!suffix_ctx.index)
        return NULL;

    // Initialize suffix array with all possible rotations
    for (bra_bwt_index_t i = 0; i < buf_size; i++)
        suffix_ctx.index[i] = i;

    // Sort all rotations lexicographically
    BRA_QSORT(suffix_ctx.index, buf_size, sizeof(bra_bwt_index_t), bwt_suffix_context_compare, &suffix_ctx);

    // Allocate output buffer
    uint8_t* out_buf = (uint8_t*) malloc(buf_size);
    if (out_buf == NULL)
        goto BRA_BWT_ENCODE_EXIT;

    // Generate BWT by taking the last character of each sorted rotation
    *primary_index = 0;
    for (bra_bwt_index_t i = 0; i < buf_size; i++)
    {
        // Last character position in this rotation
        bra_bwt_index_t last_pos = (suffix_ctx.index[i] + buf_size - 1) % buf_size;
        out_buf[i]               = buf[last_pos];

        // Track where the original string (rotation starting at 0) ended up
        if (suffix_ctx.index[i] == 0)
            *primary_index = i;
    }

BRA_BWT_ENCODE_EXIT:
    free(suffix_ctx.index);
    return out_buf;
}

uint8_t* bra_bwt_decode(const uint8_t* buf, const bra_bwt_index_t buf_size, const bra_bwt_index_t primary_index)
{
    assert(buf != NULL);
    assert(buf_size > 0);
    assert(primary_index < buf_size);

    // Count character frequencies
    bra_bwt_index_t count[BRA_ALPHABET_SIZE] = {0};
    for (bra_bwt_index_t i = 0; i < buf_size; i++)
        count[buf[i]]++;

    // Calculate first occurrence positions (cumulative counts)
    bra_bwt_index_t first_occurrence[BRA_ALPHABET_SIZE] = {0};
    for (bra_bwt_index_t i = 1; i < BRA_ALPHABET_SIZE; i++)
        first_occurrence[i] = first_occurrence[i - 1] + count[i - 1];

    // Build transform array (maps last column positions to first column positions)
    bra_bwt_index_t* transform = malloc(buf_size * sizeof(bra_bwt_index_t));
    if (!transform)
        return NULL;

    // Create the transform mapping using first occurrence positions
    bra_bwt_index_t temp_first[BRA_ALPHABET_SIZE];
    memcpy(temp_first, first_occurrence, BRA_ALPHABET_SIZE * sizeof(bra_bwt_index_t));

    for (bra_bwt_index_t i = 0; i < buf_size; i++)
    {
        const uint8_t c            = buf[i];
        transform[temp_first[c]++] = i;
    }

    // Allocate output buffer
    uint8_t* out_buf = (uint8_t*) malloc(buf_size);
    if (out_buf == NULL)
        goto BRA_BWT_DECODE_EXIT;

    // Follow the transform chain starting from primary index to reconstruct original string
    bra_bwt_index_t index = primary_index;
    for (bra_bwt_index_t i = 0; i < buf_size; i++)
    {
        index      = transform[index];
        out_buf[i] = buf[index];
    }

BRA_BWT_DECODE_EXIT:
    free(transform);
    return out_buf;
}

void bra_bwt_decode2(const uint8_t* buf, const bra_bwt_index_t buf_size, const bra_bwt_index_t primary_index, bra_bwt_index_t* transform, uint8_t* out_buf)
{
    assert(buf != NULL);
    assert(buf_size > 0);
    assert(transform != NULL);
    assert(out_buf != NULL);
    assert(primary_index < buf_size);

    // Count character frequencies
    bra_bwt_index_t count[BRA_ALPHABET_SIZE] = {0};
    for (bra_bwt_index_t i = 0; i < buf_size; i++)
        count[buf[i]]++;

    // Calculate first occurrence positions (cumulative counts)
    bra_bwt_index_t first_occurrence[BRA_ALPHABET_SIZE] = {0};
    for (bra_bwt_index_t i = 1; i < BRA_ALPHABET_SIZE; i++)
        first_occurrence[i] = first_occurrence[i - 1] + count[i - 1];

    // Build transform array (maps last column positions to first column positions)
    // bra_bwt_index_t* transform = malloc(buf_size * sizeof(bra_bwt_index_t));
    // if (!transform)
    // return NULL;

    // Create the transform mapping using first occurrence positions
    bra_bwt_index_t temp_first[BRA_ALPHABET_SIZE];
    memcpy(temp_first, first_occurrence, BRA_ALPHABET_SIZE * sizeof(bra_bwt_index_t));

    for (bra_bwt_index_t i = 0; i < buf_size; i++)
    {
        const uint8_t c            = buf[i];
        transform[temp_first[c]++] = i;
    }

    // Allocate output buffer
    // uint8_t* out_buf = (uint8_t*) malloc(buf_size);
    // if (out_buf == NULL)
    //     goto BRA_BWT_DECODE_EXIT;

    // Follow the transform chain starting from primary index to reconstruct original string
    bra_bwt_index_t index = primary_index;
    for (bra_bwt_index_t i = 0; i < buf_size; i++)
    {
        index      = transform[index];
        out_buf[i] = buf[index];
    }

    // BRA_BWT_DECODE_EXIT:
    // free(transform);
    // return out_buf;
}
