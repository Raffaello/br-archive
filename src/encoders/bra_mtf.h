#pragma once

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Encode data using Move-to-Front (MTF) algorithm.
 *
 * @todo improve the implementation using double linked list for efficiency.
 *
 * The Move-to-Front algorithm maintains a list of all possible byte values
 * (0-255) and for each input symbol, outputs its current position in the list,
 * then moves that symbol to the front. This creates many small values (0-3)
 * when there's locality in the data, making it highly compressible.
 *
 * Commonly used after BWT to further improve compression ratios.
 *
 * @param buf      Input data buffer to transform
 * @param buf_size Size of input data in bytes (must be > 0)
 * @return         Allocated output buffer with MTF-encoded data, or NULL on failure
 *
 * @note Caller is responsible for freeing the returned buffer.
 * @note Output size is always equal to input size.
 * @note Each output byte represents a position in the MTF table (0-255).
 *
 * @warning Input buffer must not be NULL and buf_size must be > 0.
 */
uint8_t* bra_mtf_encode(const uint8_t* buf, const size_t buf_size);

/**
 * @brief Decode MTF-encoded data back to original.
 *
 * Reverses the Move-to-Front transformation by maintaining the same symbol
 * list and for each input position, outputting the symbol at that position,
 * then moving it to the front of the list.
 *
 * @param buf      MTF-encoded data buffer (positions 0-255)
 * @param buf_size Size of encoded data in bytes (must be > 0)
 * @return         Allocated output buffer with original data, or NULL on failure
 *
 * @note Caller is responsible for freeing the returned buffer.
 * @note Output size is always equal to input size.
 *
 * @warning Input buffer must not be NULL and buf_size must be > 0.
 * @warning All input values must be valid positions (0-255).
 */
uint8_t* bra_mtf_decode(const uint8_t* buf, const size_t buf_size);

/**
 * @brief
 *
 * @param buf
 * @param buf_size
 * @param out_buf
 */
void bra_mtf_decode2(const uint8_t* buf, const size_t buf_size, uint8_t* out_buf);
