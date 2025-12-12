#pragma once

#include <stdint.h>
#include <lib_bra_defs.h>
#include <lib_bra_types.h>

/**
 * @brief Huffman encoded chunk
 */
typedef struct bra_huffman_chunk_t
{
    bra_huffman_t meta;    //!< huffman meta-data
    // uint32_t      size;    //!< size
    uint8_t* data;    //!< data
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
 * @brief s
 *
 * @param meta
 * @param data_size
 * @param data
 * @param out_size
 * @return uint8_t*
 */
uint8_t* bra_huffman_decode(const bra_huffman_t* meta, const uint32_t data_size, const uint8_t* data, uint32_t* out_size);

/**
 * @brief
 *
 * @param chunk
 */
void bra_huffman_free(bra_huffman_chunk_t* chunk);
