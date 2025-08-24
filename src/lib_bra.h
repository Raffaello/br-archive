#pragma once


#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>


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
    uint32_t      magic;          //!< 'BR-x'
    unsigned long data_offset;    //!< where the data chunk start from the beginning of the file
} bra_footer_t;

typedef struct bra_file_t
{
    FILE* f;
    char* fn;
} bra_file_t;

// bool bra_encode(const char* file, const uint8_t buf);

/**
 * @brief open the file @p fn in the @p mode
 *        the @p will be overwritten.
 *        if it fails there is no need to call @ref bra_io_close
 *
 * @param bf
 * @param fn
 * @param mode
 * @return true
 * @return false
 */
bool bra_io_open(bra_file_t* bf, const char* fn, const char* mode);

/**
 * @brief close file, free internal memory and set internal values to NULL.
 *
 * @param bf
 */
void bra_io_close(bra_file_t* bf);

/**
 * @brief read the header from the give @p bf file.
 *        the file must be position at the beginning of the header.
 *        In case of error return false and close the file.
 *
 * @param bf
 * @param out_bh
 * @return true when successful
 * @return false on error
 */
bool bra_io_read_header(bra_file_t* bf, bra_header_t* out_bh);

/**
 * @brief write the bra header into @p bf with @p num_files.
 *        In case of error will close @p bf calling @ref bra_io_close
 *
 * @param bf
 * @param num_files
 * @return true
 * @return false
 */
bool bra_io_write_header(bra_file_t* bf, const uint32_t num_files);

/**
 * @brief read thr bra footer into @p bf_out.
 *        in case of error call @ref bra_io_close with @p f
 *
 * @param f
 * @param bf_out
 * @return true
 * @return false
 */
bool bra_io_read_footer(bra_file_t* f, bra_footer_t* bf_out);

/**
 * @brief write the footer into the file @p f.
 *
 *
 * @param f
 * @param data_offset
 * @return true
 * @return false
 */
bool bra_io_write_footer(bra_file_t* f, const unsigned long data_offset);

/**
 * @brief copy the file @src into @dst in a size of chunks of #MAX_BUF_SIZE for @p data_size
 *        the files must be positioned from where to be read for @p src
 *        and where to be written for @p dst
 *
 * @param dst
 * @param src
 * @param data_size
 * @return true
 * @return false
 */
bool bra_io_copy_file_chunks(bra_file_t* dst, bra_file_t* src, const uintmax_t data_size);

#ifdef __cplusplus
}
#endif
