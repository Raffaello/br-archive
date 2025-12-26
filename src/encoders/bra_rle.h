#pragma once

#include <lib_bra_types.h>

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief RLE chunk representing a run of repeated characters.
 */
typedef struct bra_rle_chunk_t
{
    bra_rle_counts_t        counts;    //!< counts is stored as -1, i.e. 0 means 1 and 255 means 256
    uint8_t                 value;     //!< the repeated char.
    struct bra_rle_chunk_t* pNext;     //!< pointer to next chunk; NULL if last.
} bra_rle_chunk_t;

/**
 * @brief Encode a buffer using Run-Length Encoding.
 *
 * @todo pass in/out rle_tail param to avoid iterating through the rle_head each time after the first buffer.
 *
 * @see bra_encode_rle
 * @see bra_encode_rle_free_list
 *
 * @param buf                       Input buffer to encode.
 * @param buf_size                  Size of the input buffer in bytes.
 * @param num_rle_chunks[in, out]   On input: existing chunks count; on output: total chunks after encoding. @note this is not really required, but *2 give the total size in bytes.
 * @param rle_head[in, out]
 * @retval true                     on successful encoding.
 * @retval false                    on error.
 */
bool bra_encode_rle(const uint8_t* buf, const size_t buf_size, uint64_t* num_rle_chunks, bra_rle_chunk_t** rle_head);

/**
 * @brief Decode Run-Length Encoded data to output buffer @p buf.
 *
 * @note Do not pass @p rle_head as @p cur_rle. Pass a copy of the head instead; @p rle_head is needed later with @ref bra_encode_rle_free_list.
 *
 * @see bra_encode_rle
 * @see bra_encode_rle_free_list
 *
 * @param cur_rle[in, out]
 * @param buf[out]          buf Output buffer to write decoded data.
 * @param buf_size          Size of the output buffer in bytes.
 * @param buf_i[out]
 * @retval true             on success ( @p buf_i advanced; @p *cur_rle updated; becomes @c NULL when decoding completes).
 * @retval false            on error or nothing to decode.
 */
bool bra_decode_rle(bra_rle_chunk_t** cur_rle, uint8_t* buf, const size_t buf_size, size_t* buf_i);

/**
 * @brief Free the allocated RLE chunk list @p rle_head created by @ref bra_encode_rle.
 *
 * @see bra_encode_rle
 * @see bra_decode_rle
 *
 * @param rle_head
 * @retval true on success
 * @retval false if rle_head is NULL
 */
bool bra_encode_rle_free_list(bra_rle_chunk_t** rle_head);

/**
 * @brief Encode a buffer using Run-Length Encoding as PackBits.
 *
 * The Control Block is:
 *  -  0 to +127 : Literal block of (n + 1) bytes.
 *  - -1 to -127 : Run Block: repeat next byte (1 - n) times.
 *  - -128       : not used
 *
 * @param buf           Input buffer to encode.
 * @param buf_size      Size of the input buffer in bytes.
 * @param out_buf       Output buffer encoded.
 * @param out_buf_size  Output buffer size.
 * @retval true         on successful encoding.
 * @retval false        on error.
 */
bool bra_rle_encode(const uint8_t* buf, const size_t buf_size, uint8_t** out_buf, size_t* out_buf_size);
