#pragma once


#include <stdbool.h>
#include <stdint.h>

/**
 * @brief
 *
 * @todo implement the SA-IS algorithm to avoid the O(n log n) suffix sorting
 *
 * @param buf
 * @param buf_size
 * @param primary_index
 * @return uint8_t*
 */
uint8_t* bra_bwt_encode(const uint8_t* buf, const size_t buf_size, size_t* primary_index);


uint8_t* bra_bwt_decode(const uint8_t* buf, const size_t buf_size, const size_t primary_index);
