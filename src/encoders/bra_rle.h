#pragma once

#ifdef __cplusplus
extern "C" {
#endif


// #include "../lib_bra_types.h"
#include <lib_bra_types.h>

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief
 *
 * @param buf
 * @param buf_size
 * @param num_rle_chunks this is not really required.. but *2 give the total size in bytes... so know the compression size
 * @param chunk_head
 * @return true
 * @return false
 */
bool bra_encode_rle(const char* buf, const size_t buf_size, uint64_t* num_rle_chunks, bra_rle_chunk_t** chunk_head);

/**
 * @brief
 *
 * @param rle
 * @return true
 * @return false
 */
bool bra_encode_rle_free_list(bra_rle_chunk_t** rle);

/**
 * @brief
 *
 * @param cur
 * @param buf
 * @param buf_size
 * @param buf_i
 * @return true
 * @return false
 */
bool bra_decode_rle(bra_rle_chunk_t** cur, char* buf, const size_t buf_size, size_t* buf_i);

#ifdef __cplusplus
}
#endif
