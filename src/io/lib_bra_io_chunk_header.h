#pragma once

#include <lib_bra_types.h>

#include <stdbool.h>

/**
 * @brief
 *
 * @param chunk_header
 * @return true
 * @return false
 */
bool bra_io_chunk_header_validate(const bra_io_chunk_header_t* chunk_header);
