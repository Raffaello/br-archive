#pragma once

#include <stdint.h>

// #define BRA_CRC32_INIT      0xFFFFFFFF     // 0x1EDC6F41
#define BRA_CRC32C_INIT 0u    // // public initial value; function inverts in/out (std CRC-32C init/xor=0xFFFFFFFF)

/**
 * @brief Calculates the CRC32C (Castagnoli) checksum of the given data using a look-up table.
 *        Use @ref bra_crc32c instead.
 *
 * @details This implementation uses a pre-computed 256-entry lookup table for efficient
 *          calculation of the CRC32C checksum using the reflected polynomial 0x82F63B78.
 *
 * @see bra_crc32c
 *
 * @param data         Pointer to the input data.
 * @param length       Length of the input data in bytes.
 * @param previous_crc Previous CRC value (for incremental updates).
 * @return uint32_t    The calculated CRC32C checksum.
 */
uint32_t bra_crc32c_table(const void* data, const uint64_t length, const uint32_t previous_crc);

/**
 * @brief Calculates the CRC32C (Castagnoli) checksum using SSE4.2 intrinsics.
 *        Use @ref bra_crc32c instead.
 *
 * @details This implementation uses the hardware CRC32C instruction available in
 *          processors with SSE4.2 support.
 *
 * @see bra_crc32c
 *
 * @param data         Pointer to the input data.
 * @param length       Length of the input data in bytes.
 * @param previous_crc Previous CRC value (for incremental updates).
 * @return uint32_t    The calculated CRC32C checksum.
 */
uint32_t bra_crc32c_sse42(const void* data, const uint64_t length, const uint32_t previous_crc);

/**
 * @brief Calculates the CRC32/C (Castagnoli) checksum using SSE4.2 intrinsics or look-up table,
 *        depending on hardware support.
 *
 * @param data         Pointer to the input data.
 * @param length       Length of the input data in bytes.
 * @param previous_crc Previous CRC value (for incremental updates).
 * @return uint32_t    The calculated CRC32/C checksum.
 */
uint32_t bra_crc32c(const void* data, const uint64_t length, const uint32_t previous_crc);
