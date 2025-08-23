#pragma once


#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

// #include <stdio.h>


/**
 * @brief
 *
 * @todo issue with big endian (not supporting it for now)
 */
#define BRA_MAGIC    0x612D5242    // 0x61='a' 0x2D='-' 0x52='R' 0x42='B'
#define BRA_FILE_EXT ".BRa"

typedef struct bra_file_name_t
{
    // TODO: add CRC ... file permissions, etc...
    uint8_t name_size;
    // char*   name;
} bra_file_name_t;

typedef struct bra_data_t
{
    uintmax_t data_size;
    // uint8_t* data;
} bra_data_t;

typedef struct bra_header_t
{
    uint32_t magic;        // !< 'BR-a' => 0x41522D61
    uint32_t num_files;    // just 1 for now
    // bra_file_t* files;
} bra_header_t;

// bool bra_encode(const char* file, const uint8_t buf);


#ifdef __cplusplus
}
#endif
