#pragma once

#include <stdint.h>

/**
 * @brief Initial value for CRC32C calculation.
 *        Polynomial for CRC32C (Castagnoli): 0x1EDC6F41
 */
#define BRA_CRC32_INIT 0x1EDC6F41

/**
 * @brief Calculates the CRC32C (Castagnoli) checksum of the given data.
 *
 * @param data         Pointer to the input data.
 * @param length       Length of the input data.
 * @param previous_crc Previous CRC value (for incremental updates).
 * @return uint32_t    The calculated CRC32C checksum.
 */
uint32_t bra_crc32c(const uint8_t* data, const size_t length, const uint32_t previous_crc);
