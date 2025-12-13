#pragma once

#include <stdint.h>
#include <lib_bra_defs.h>
#include <lib_bra_types.h>

/**
 * @brief Huffman encoded data
 */
typedef struct bra_huffman_chunk_t
{
    bra_huffman_t meta;    //!< huffman meta-data
    uint8_t*      data;    //!< data
} bra_huffman_chunk_t;

/**
 * @brief Encode @p buf into Huffman canonical codes.
 *
 * @see bra_huffman_chunk_free
 *
 * @param buf      the buffer to encode
 * @param buf_size the buffer size in bytes.
 * @return bra_huffman_t* Huffman encoded data and metadata. The caller must free it with @ref bra_huffman_chunk_free
 */
bra_huffman_chunk_t* bra_huffman_encode(const uint8_t* buf, const uint32_t buf_size);

/**
 * @brief Decode Huffman canonical codes.
 *
 * @param meta      Huffman metadata
 * @param data      Huffman encoded data
 * @param out_size  Decoded data size
 * @return uint8_t* Heap allocated decoded data, must be free by the caller
 */
uint8_t* bra_huffman_decode(const bra_huffman_t* meta, const uint8_t* data, uint32_t* out_size);

/**
 * @brief Free the Huffman encoded data struct. It is safe to pass @p chunk as @c NULL.
 *
 * @param chunk the Huffman encoded data struct to be freed.
 */
void bra_huffman_chunk_free(bra_huffman_chunk_t* chunk);
