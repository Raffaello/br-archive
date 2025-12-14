#include "lib_bra_io_chunk_header.h"
#include <lib_bra_defs.h>

bool bra_io_chunk_header_validate(const bra_io_chunk_header_t* chunk_header)
{
    if (chunk_header == NULL)
        return false;

    if (chunk_header->huffman.encoded_size > BRA_MAX_CHUNK_SIZE)
        return false;
    if (chunk_header->huffman.orig_size > BRA_MAX_CHUNK_SIZE)
        return false;
    if (chunk_header->huffman.encoded_size == 0)
        return false;
    if (chunk_header->huffman.orig_size == 0)
        return false;

    return true;
}
