#pragma once


#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

// #include <stdio.h>


/**
 * @brief Support only little endian machine at the moment
 *
 * @todo issue with big endian (not supporting it for now)
 * @todo can use a magic string instead so it will be endian independent
 */
#define BRA_MAGIC        0x612D5242    // 0x61='a' 0x2D='-' 0x52='R' 0x42='B'
#define BRA_FOOTER_MAGIC 0x782D5242    // 0x78='x' 0x2D='-' 0x52='R' 0x42='B'
#define BRA_FILE_EXT     ".BRa"
#define BRA_NAME         "BRa"
#define BRA_SFX_FILENAME "bra.sfx"    // @todo: generate it through cmake conf

#ifdef _linux_
#define BRA_SFX_FILE_EXT ".brx"
#elif __WIN32__
#define BRA_SFX_FILE_EXT ".exe"
#else
#error "unsupported platform" // maybe it could work anyway, but did't test it.
#endif
// #define BRA_SFX_FILE_TMP ".tmp"

#define MAX_BUF_SIZE (1024 * 1024)    // 1M

typedef struct bra_file_name_t
{
    // TODO: add CRC ... file permissions, file attributes, etc...
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
    uint32_t magic;        //!< 'BR-a'
    uint32_t num_files;    // just 1 for now
    // bra_file_t* files;
} bra_header_t;

typedef struct bra_footer_t
{
    uint32_t magic;          //!< 'BR-x'
    long     data_offset;    //!< where the data chunk start from the beginning of the file
} bra_footer_t;

// bool bra_encode(const char* file, const uint8_t buf);


#ifdef __cplusplus
}
#endif
