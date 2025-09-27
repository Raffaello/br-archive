#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Encode data using Burrows-Wheeler Transform (BWT).
 *
 * Performs the Burrows-Wheeler Transform on input data by creating all cyclic
 * rotations of the input, sorting them lexicographically, and taking the last
 * character of each sorted rotation. This transformation clusters similar
 * characters together, making the output highly compressible with subsequent
 * algorithms like Move-to-Front and entropy coding.
 *
 * The BWT is reversible using the primary index, which indicates the position
 * of the original string in the sorted rotation matrix.
 *
 * @param buf Input data buffer to transform (must not be NULL)
 * @param buf_size Size of input data in bytes (must be > 0)
 * @param primary_index Pointer to store primary index for decoding (must not be NULL)
 * @return Allocated output buffer containing BWT-transformed data, or NULL on failure
 *
 * @note Caller is responsible for freeing the returned buffer.
 * @note Output size is always equal to input size.
 * @note Primary index is required for reversible decoding with bra_bwt_decode().
 * @note Currently uses O(n² log n) comparison-based sorting.
 *
 * @todo Implement SA-IS algorithm to achieve O(n) time complexity.
 *
 * @warning Input buffer and primary_index must not be NULL.
 * @warning buf_size must be greater than 0.
 *
 * @example
 * @code
 * const uint8_t* input = (uint8_t*)"BANANA";
 * size_t primary_index;
 * uint8_t* bwt_output = bra_bwt_encode(input, 6, &primary_index);
 * // bwt_output contains "NNBAAA", primary_index = 3
 * free(bwt_output);
 * @endcode
 */
uint8_t* bra_bwt_encode(const uint8_t* buf, const size_t buf_size, size_t* primary_index);

/**
 * @brief Decode BWT-transformed data back to original.
 *
 * Reverses the Burrows-Wheeler Transform using the first-last mapping property.
 * Reconstructs the original data from the BWT output and primary index by
 * following the transform chain that maps positions in the last column back
 * to the first column of the sorted rotation matrix.
 *
 * @param buf BWT-transformed data buffer (must not be NULL)
 * @param buf_size Size of transformed data in bytes (must be > 0)
 * @param primary_index Primary index from bra_bwt_encode() (must be < buf_size)
 * @return Allocated output buffer containing original data, or NULL on failure
 *
 * @note Caller is responsible for freeing the returned buffer.
 * @note Output size is always equal to input size.
 * @note Primary index must match the value returned by bra_bwt_encode().
 *
 * @warning Input buffer must not be NULL.
 * @warning buf_size must be greater than 0.
 * @warning primary_index must be less than buf_size.
 *
 * @example
 * @code
 * const uint8_t* bwt_data = (uint8_t*)"NNBAAA";
 * uint8_t* original = bra_bwt_decode(bwt_data, 6, 3);
 * // original contains "BANANA"
 * free(original);
 * @endcode
 */
uint8_t* bra_bwt_decode(const uint8_t* buf, const size_t buf_size, const size_t primary_index);
