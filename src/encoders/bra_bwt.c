#include <encoders/bra_bwt.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define BRA_BWT_ALPHABET 256

// Simple suffix structure for comparison-based sorting
typedef struct
{
    size_t         index;
    const uint8_t* data;
    size_t         length;
} bwt_suffix_t;

// Compare cyclic suffixes lexicographically
static int bwt_suffix_compare(const void* a, const void* b)
{
    const bwt_suffix_t* sa = (const bwt_suffix_t*) a;
    const bwt_suffix_t* sb = (const bwt_suffix_t*) b;

    for (size_t i = 0; i < sa->length; i++)
    {
        size_t idx_a = (sa->index + i) % sa->length;
        size_t idx_b = (sb->index + i) % sb->length;

        uint8_t byte_a = sa->data[idx_a];
        uint8_t byte_b = sb->data[idx_b];

        if (byte_a < byte_b)
            return -1;
        if (byte_a > byte_b)
            return 1;
    }

    return 0;
}

uint8_t* bra_bwt_encode(const uint8_t* buf, const size_t buf_size, size_t* primary_index)
{
    assert(buf != NULL);
    assert(buf_size > 0);
    assert(primary_index != NULL);

    // Allocate suffix array for all rotations
    bwt_suffix_t* suffixes = malloc(buf_size * sizeof(bwt_suffix_t));
    if (!suffixes)
        return NULL;

    // Initialize suffix array with all possible rotations
    for (size_t i = 0; i < buf_size; i++)
    {
        suffixes[i].index  = i;
        suffixes[i].data   = buf;
        suffixes[i].length = buf_size;
    }

    // Sort all rotations lexicographically
    qsort(suffixes, buf_size, sizeof(bwt_suffix_t), bwt_suffix_compare);

    // Allocate output buffer
    uint8_t* out_buf = (uint8_t*) malloc(buf_size);
    if (out_buf == NULL)
    {
        free(suffixes);
        return NULL;
    }

    // Generate BWT by taking the last character of each sorted rotation
    *primary_index = 0;
    for (size_t i = 0; i < buf_size; i++)
    {
        // Last character position in this rotation
        size_t last_pos = (suffixes[i].index + buf_size - 1) % buf_size;
        out_buf[i]      = buf[last_pos];

        // Track where the original string (rotation starting at 0) ended up
        if (suffixes[i].index == 0)
            *primary_index = i;
    }

    free(suffixes);
    return out_buf;
}

uint8_t* bra_bwt_decode(const uint8_t* buf, const size_t buf_size, const size_t primary_index)
{
    assert(buf != NULL);
    assert(buf_size > 0);
    assert(primary_index < buf_size);


    // Count character frequencies
    size_t count[BRA_BWT_ALPHABET] = {0};
    for (size_t i = 0; i < buf_size; i++)
        count[buf[i]]++;

    // Calculate first occurrence positions (cumulative counts)
    size_t first_occurrence[BRA_BWT_ALPHABET] = {0};
    for (int i = 1; i < BRA_BWT_ALPHABET; i++)
        first_occurrence[i] = first_occurrence[i - 1] + count[i - 1];

    // Build transform array (maps last column positions to first column positions)
    size_t* transform = malloc(buf_size * sizeof(size_t));
    if (!transform)
        return NULL;

    // Create the transform mapping using first occurrence positions
    size_t temp_first[BRA_BWT_ALPHABET];
    memcpy(temp_first, first_occurrence, BRA_BWT_ALPHABET * sizeof(size_t));

    for (size_t i = 0; i < buf_size; i++)
    {
        const uint8_t c            = buf[i];
        transform[temp_first[c]++] = i;
    }

    // Allocate output buffer
    uint8_t* out_buf = (uint8_t*) malloc(buf_size);
    if (out_buf == NULL)
    {
        free(transform);
        return NULL;
    }

    // Follow the transform chain starting from primary index to reconstruct original string
    size_t index = primary_index;
    for (size_t i = 0; i < buf_size; i++)
    {
        index      = transform[index];
        out_buf[i] = buf[index];
    }

    free(transform);
    return out_buf;
}
