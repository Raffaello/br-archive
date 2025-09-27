#include <encoders/bra_mtf.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define BRA_MTF_ALPHABET_SIZE 256

// Initialize MTF table with values 0-255
static void mtf_init_table(uint8_t* table)
{
    for (int i = 0; i < BRA_MTF_ALPHABET_SIZE; i++)
        table[i] = (uint8_t) i;
}

// Find position of value in table and move it to front
static uint8_t mtf_encode_symbol(uint8_t* table, uint8_t symbol)
{
    uint8_t position = 0;

    // Find the symbol in the table
    while (table[position] != symbol)
        position++;

    // Move symbol to front by shifting others right (TODO this is inefficient, doubly linked list would be better)
    uint8_t temp = table[position];
    for (uint8_t i = position; i > 0; i--)
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
    for (uint8_t i = position; i > 0; i--)
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

    // Initialize MTF table (TODO this is a constant, pointless to reinit every time)
    uint8_t mtf_table[BRA_MTF_ALPHABET_SIZE];
    mtf_init_table(mtf_table);

    // Encode each symbol
    for (size_t i = 0; i < buf_size; i++)
        out_buf[i] = mtf_encode_symbol(mtf_table, buf[i]);

    return out_buf;
}

uint8_t* bra_mtf_decode(const uint8_t* buf, const size_t buf_size)
{
    assert(buf != NULL);
    assert(buf_size > 0);

    // Allocate output buffer
    uint8_t* out_buf = malloc(buf_size);
    if (!out_buf)
        return NULL;

    // Initialize MTF table (TODO this is a constant, pointless to reinit every time)
    uint8_t mtf_table[BRA_MTF_ALPHABET_SIZE];
    mtf_init_table(mtf_table);

    // Decode each position
    for (size_t i = 0; i < buf_size; i++)
    {
        uint8_t position = buf[i];
        out_buf[i]       = mtf_table[position];    // Get symbol at this position
        mtf_decode_symbol(mtf_table, position);    // Move it to front
    }

    return out_buf;
}
