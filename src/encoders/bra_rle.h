#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @brief Encode a buffer using Run-Length Encoding as PackBits.
 *
 * The Control Block is:
 *  -  0 to +127 : Literal block of (n + 1) bytes.
 *  - -1 to -127 : Run Block: repeat next byte (1 - n) times.
 *  - -128       : not used
 *
 * @note Caller must free @p *out_buf on success
 *
 * @param buf           Input buffer to encode.
 * @param buf_size      Size of the input buffer in bytes.
 * @param out_buf       Output buffer encoded.
 * @param out_buf_size  Output buffer size.
 * @retval true         on successful encoding.
 * @retval false        on error.
 */
bool bra_rle_encode(const uint8_t* buf, const size_t buf_size, uint8_t** out_buf, size_t* out_buf_size);

/**
 * @brief Compute the decode RLE buffer size.
 *
 * @param buf RLE encoded buffer
 * @param buf_size RLE encoded buffer size in bytes.
 * @return size_t decoded RLE buffer size
 */
size_t bra_rle_decode_compute_size(const uint8_t* buf, const size_t buf_size);

/**
 * @brief Decode Run-Length Encoded data.
 *
 * @note Caller must free @p *out_buf on success
 *
 * @param buf           Input buffer to decode.
 * @param buf_size      Size of the input buffer in bytes.
 * @param out_buf       Output buffer decoded.
 * @param out_buf_size  Output buffer size.
 * @retval true         on successful decoding.
 * @retval false        on error.
 */
bool bra_rle_decode(const uint8_t* buf, const size_t buf_size, uint8_t** out_buf, size_t* out_buf_size);
