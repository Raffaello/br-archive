#pragma once


#include <stdbool.h>
#include <stdint.h>


uint8_t* bra_bwt_encode(const uint8_t* buf, const size_t buf_size, size_t* primary_index);


uint8_t* bra_bwt_decode(const uint8_t* buf, const size_t buf_size, const size_t primary_index);
