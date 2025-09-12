#pragma once

#ifdef __cplusplus
extern "C" {
#endif


// #include "../lib_bra_types.h"
#include <lib_bra_types.h>

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Encode a buffer using Run-Length Encoding.
 *
 * @see bra_encode_rle
 * @see bra_encode_rle_free_list
 *
 * @param buf Input buffer to encode.
 * @param buf_size Size of the input buffer in bytes.
 * @param num_rle_chunks[in, out] On input: existing chunks count; on output: total chunks after encoding. @note this is not really required, but *2 give the total size in bytes.
 * @param rle_head[in, out]
 * @return true on successful encoding.
 * @return false on error.
 */
bool bra_encode_rle(const char* buf, const size_t buf_size, uint64_t* num_rle_chunks, bra_rle_chunk_t** rle_head);

/**
 * @brief Decode Run-Length Encoded data to output buffer @p buf.
 *
 * @note Do not pass the @c rle_head as a @p cur_rle, but create a copy of the head to be passed on; the @p rle_head need to be used later on with @ref bra_encode_rle_free_list
 *
 * @see bra_encode_rle
 * @see bra_encode_rle_free_list
 *
 * @param cur_rle[in, out]
 * @param buf[out] buf Output buffer to write decoded data.
 * @param buf_size Size of the output buffer in bytes.
 * @param buf_i[out]
 * @return true on successful encoding and there is still to decode.
 * @return false on error or nothing to decode. @todo this might be not clear to detect errors.
 */
bool bra_decode_rle(bra_rle_chunk_t** cur_rle, char* buf, const size_t buf_size, size_t* buf_i);

/**
 * @brief Free the allocate @p rle_head from @ref bra_encode_rle
 *
 * @see bra_encode_rle
 * @see bra_decode_rle
 *
 * @param rle_head
 * @return true
 * @return false
 */
bool bra_encode_rle_free_list(bra_rle_chunk_t** rle_head);

#ifdef __cplusplus
}
#endif
