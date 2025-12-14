#pragma once

#include <lib_bra_types.h>

#include <stdbool.h>

/**
 * @brief Validates a chunk header for correctness
 *
 * @param chunk_header
 *
 * @retval true if the chunk header is valid (non-NULL, sizes within bounds and non-zero)
 * @retval false if validation fails
 */
bool bra_io_chunk_header_validate(const bra_io_chunk_header_t* chunk_header);
