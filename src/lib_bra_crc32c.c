#include <lib_bra_crc32c.h>

#include <log/bra_log.h>

uint32_t bra_crc32c(const void* data, const uint64_t length, const uint32_t previous_crc)
{
    uint32_t crc = ~previous_crc;    // Invert initial CRC value
    for (uint64_t i = 0; i < length; ++i)
    {
        const uint8_t byte  = ((uint8_t*) data)[i];
        crc                ^= byte;    // XOR byte into the least significant byte of CRC
        for (int j = 0; j < 8; ++j)    // Process each bit
        {
            if (crc & 1)
                crc = (crc >> 1) ^ BRA_CRC32C_POLY;    // Apply polynomial if LSB is 1
            else
                crc >>= 1;                             // Just shift if LSB is 0
        }
    }

    return ~crc;    // Invert final CRC value
}
