#include <encoders/bra_mtf.h>
#include <lib_bra_defs.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

// Initialize MTF table with values 0-255
static inline void mtf_init_table(uint8_t* table)
{
    for (int i = 0; i < BRA_ALPHABET_SIZE; i++)
        table[i] = (uint8_t) i;
}

// Find position of value in table and move it to front
static uint8_t mtf_encode_symbol(uint8_t* table, const uint8_t symbol)
{
    uint8_t position = 0;

    // Find the symbol in the table
    while (table[position] != symbol)
        position++;

    // Move symbol to front by shifting others right (TODO this is inefficient, doubly linked list would be better)
    uint8_t temp = table[position];
    for (uint8_t i = position; i > 0; --i)
        table[i] = table[i - 1];

    table[0] = temp;

    return position;
}

// Move symbol at given position to front
static void mtf_decode_symbol(uint8_t* table, uint8_t position)
{
    // Get the symbol at this position
    uint8_t temp = table[position];

    // Shift symbols to the right (TODO inefficient, doubly linked list would be better)
    for (uint8_t i = position; i > 0; --i)
        table[i] = table[i - 1];

    // Move symbol to front
    table[0] = temp;
}

uint8_t* bra_mtf_encode(const uint8_t* buf, const size_t buf_size)
{
    assert(buf != NULL);
    assert(buf_size > 0);

    // Allocate output buffer
    uint8_t* out_buf = malloc(buf_size);
    if (!out_buf)
        return NULL;

    if (!bra_mtf_encode2(buf, buf_size, out_buf))
    {
        free(out_buf);
        return NULL;
    }

    return out_buf;
}

bool bra_mtf_encode2(const uint8_t* buf, const size_t buf_size, uint8_t* out_buf)
{
    assert(buf != NULL);
    assert(buf_size > 0);
    assert(out_buf != NULL);

    // Initialize MTF table
    uint8_t mtf_table[BRA_ALPHABET_SIZE];
    mtf_init_table(mtf_table);

    // Encode each symbol
    for (size_t i = 0; i < buf_size; ++i)
        out_buf[i] = mtf_encode_symbol(mtf_table, buf[i]);

    return true;
}

uint8_t* bra_mtf_decode(const uint8_t* buf, const size_t buf_size)
{
    assert(buf != NULL);
    assert(buf_size > 0);

    // Allocate output buffer
    uint8_t* out_buf = malloc(buf_size);
    if (!out_buf)
        return NULL;

    bra_mtf_decode2(buf, buf_size, out_buf);
    return out_buf;
}

void bra_mtf_decode2(const uint8_t* buf, const size_t buf_size, uint8_t* out_buf)
{
    assert(buf != NULL);
    assert(out_buf != NULL);
    assert(buf_size > 0);

    // Initialize MTF table
    uint8_t mtf_table[BRA_ALPHABET_SIZE];
    mtf_init_table(mtf_table);

    // Decode each position
    for (size_t i = 0; i < buf_size; ++i)
    {
        const uint8_t position = buf[i];
        out_buf[i]             = mtf_table[position];    // Get symbol at this position
        mtf_decode_symbol(mtf_table, position);          // Move it to front
    }
}
