#pragma once

#include <stdint.h>

// #define BRA_CRC32_INIT      0xFFFFFFFF     // 0x1EDC6F41
#define BRA_CRC32C_INIT 0u    // reflected initial value

/**
 * @brief Calculates the CRC32C (Castagnoli) checksum of the given data.
 *
 * @details This implementation uses a pre-computed 256-entry lookup table for efficient
 *          calculation of the CRC32C checksum using the reflected polynomial 0x82F63B78.
 *
 * @param data         Pointer to the input data.
 * @param length       Length of the input data in bytes.
 * @param previous_crc Previous CRC value (for incremental updates).
 * @return uint32_t    The calculated CRC32C checksum.
 */
uint32_t bra_crc32c(const void* data, const uint64_t length, const uint32_t previous_crc);
