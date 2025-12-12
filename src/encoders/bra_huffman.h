#pragma once

#include <stdint.h>
#include <lib_bra_defs.h>

/**
 * @brief Huffman encoded chunk
 */
typedef struct bra_huffman_chunk_t
{
    uint8_t lengths[BRA_ALPHABET_SIZE];    //!< huffman symbol frequencies
    // NOTE: not ok, if 2 symbols are encoded make no sense to store 256,
    //       but otherwise need 2 byte for each symbol,
    //       and if more than 128 make no sense in the other way around ...
    //       so... trade-off
    // NOTE: this also implies to require a minimum file size of more than the alphabet size (> 256), otherwise store it directly
    uint32_t orig_size;    //!< orig data size (used for decoding)
    uint32_t size;         //!< size
    uint8_t* data;         //!< data
} bra_huffman_chunk_t;

/**
 * @brief
 *
 * @param buf
 * @param buf_size
 * @return bra_huffman_t*
 */
bra_huffman_chunk_t* bra_huffman_encode(const uint8_t* buf, const uint32_t buf_size);

/**
 * @brief
 *
 * @param chunk
 * @param out_size
 * @return uint8_t*
 */
uint8_t* bra_huffman_decode(const bra_huffman_chunk_t* chunk, uint32_t* out_size);

/**
 * @brief
 *
 * @param chunk
 */
void bra_huffman_free(bra_huffman_chunk_t* chunk);
