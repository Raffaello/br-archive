#pragma once

#include <lib_bra_types.h>

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
 * @param buf Input data buffer to transform (must not be @c NULL)
 * @param buf_size Size of input data in bytes (must be > 0)
 * @param primary_index Pointer to store primary index for decoding (must not be @c NULL)
 * @return Allocated output buffer containing BWT-transformed data, or @c NULL on failure
 *
 * @note Caller is responsible for freeing the returned buffer.
 * @note Output size is always equal to input size.
 * @note Primary index is required for reversible decoding with @ref bra_bwt_decode().
 * @note Currently uses O(n² log n) comparison-based sorting.
 *
 * @todo Implement SA-IS algorithm to achieve O(n) time complexity.
 *
 * @warning Input buffer and primary_index must not be @c NULL.
 * @warning buf_size must be greater than 0.
 *
 * @code
 * const uint8_t* input = (uint8_t*)"BANANA";
 * bra_bwt_index_t primary_index;
 * uint8_t* bwt_output = bra_bwt_encode(input, 6, &primary_index);
 * // bwt_output contains "NNBAAA", primary_index = 3
 * free(bwt_output);
 * @endcode
 */
uint8_t* bra_bwt_encode(const uint8_t* buf, const bra_bwt_index_t buf_size, bra_bwt_index_t* primary_index);

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
 * @param buf Input data buffer to transform (must not be @c NULL)
 * @param buf_size Size of input data in bytes (must be > 0)
 * @param primary_index Pointer to store primary index for decoding (must not be @c NULL)
 * @param out_buf Output buffer to store BWT-transformed data (must not be @c NULL)
 * @retval true  on success
 * @retval false on failure
 */
bool bra_bwt_encode2(const uint8_t* buf, const bra_bwt_index_t buf_size, bra_bwt_index_t* primary_index, uint8_t* out_buf);

/**
 * @brief Decode BWT-transformed data back to original.
 *
 * Reverses the Burrows-Wheeler Transform using the first-last mapping property.
 * Reconstructs the original data from the BWT output and primary index by
 * following the transform chain that maps positions in the last column back
 * to the first column of the sorted rotation matrix.
 *
 * @param buf BWT-transformed data buffer (must not be @c NULL)
 * @param buf_size Size of transformed data in bytes (must be > 0)
 * @param primary_index Primary index from  @ref bra_bwt_encode() (must be < @p buf_size)
 * @return Allocated output buffer containing original data, or @c NULL on failure
 *
 * @note Caller is responsible for freeing the returned buffer.
 * @note Output size is always equal to input size.
 * @note Primary index must match the value returned by @ref bra_bwt_encode().
 *
 * @warning Input buffer must not be @c NULL.
 * @warning buf_size must be greater than 0.
 * @warning primary_index must be less than @p buf_size.
 *
 * @code
 * const uint8_t* bwt_data = (uint8_t*)"NNBAAA";
 * uint8_t* original = bra_bwt_decode(bwt_data, 6, 3);
 * // original contains "BANANA"
 * free(original);
 * @endcode
 */
uint8_t* bra_bwt_decode(const uint8_t* buf, const bra_bwt_index_t buf_size, const bra_bwt_index_t primary_index);

/**
 * @brief Decode BWT-transformed data back to original.
 *
 * Reverses the Burrows-Wheeler Transform using the first-last mapping property.
 * Reconstructs the original data from the BWT output and primary index by
 * following the transform chain that maps positions in the last column back
 * to the first column of the sorted rotation matrix.
 *
 * @param buf BWT-transformed data buffer (must not be @c NULL)
 * @param buf_size Size of transformed data in bytes (must be > 0)
 * @param primary_index Primary index from  @ref bra_bwt_encode() (must be < @p buf_size)
 * @param transform transform buffer to be passed as at least having @p buf_size num elements.
 * @param out_buf Allocated output buffer containing original data of at least @p buf_size  size.
 *
 * @note Caller is responsible for freeing the returned buffer.
 * @note Output size is always equal to input size.
 * @note Primary index must match the value returned by @ref bra_bwt_encode().
 *
 * @warning Input buffer must not be @c NULL.
 * @warning buf_size must be greater than 0.
 * @warning primary_index must be less than @p buf_size.
 *
 * @code
 * const uint8_t* bwt_data = (uint8_t*)"NNBAAA";
 * uint8_t* original = bra_bwt_decode(bwt_data, 6, 3);
 * // original contains "BANANA"
 * free(original);
 * @endcode
 */

/**
 * @brief
 *
 * @param buf
 * @param buf_size
 * @param primary_index

 */
void bra_bwt_decode2(const uint8_t* buf, const bra_bwt_index_t buf_size, const bra_bwt_index_t primary_index, bra_bwt_index_t* transform, uint8_t* out_buf);
