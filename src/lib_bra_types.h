#pragma once

#include "lib_bra_defs.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>


typedef uint8_t bra_attr_t;          //!< file attribute type
typedef uint8_t bra_rle_counts_t;    //!< rle counts type

/**
 * @brief Define a file overwrite policy.
 */
typedef enum bra_fs_overwrite_policy_e
{
    BRA_OVERWRITE_ASK        = 0,
    BRA_OVERWRITE_ALWAYS_YES = 1,
    BRA_OVERWRITE_ALWAYS_NO  = 2,
} bra_fs_overwrite_policy_e;

#pragma pack(push, 1)

/**
 * @brief BR-Archive File Header.
 */
typedef struct bra_io_header_t
{
    uint32_t magic;    //!< 'BR-a'

    // uint8_t  version;    //!< Archive format version
    // compression type ? (archive, best, fast, ... ??)
    // crc32 / md5 ?
    uint32_t num_files;

} bra_io_header_t;

/**
 * @brief BR-Sfx File Footer.
 */
typedef struct bra_io_footer_t
{
    uint32_t magic;            //!< 'BR-x'
    int64_t  header_offset;    //!< absolute offset of the header chunk from file start.
} bra_io_footer_t;

#pragma pack(pop)

/**
 * @brief Type used to perform I/O from the disk.
 *        It is just a simple wrapper around @c FILE,
 *        but it carries on the filename @p fn associated with it.
 */
typedef struct bra_io_file_t
{
    FILE* f;     //!< File Pointer representing a file on the disk.
    char* fn;    //!< the filename of the file on disk.
} bra_io_file_t;

/**
 * @brief This is the metadata of each file stored in a BR-archive.
 *        The file data is just after for @p data_size bytes.
 */
typedef struct bra_meta_file_t
{
    // TODO: add CRC ... file permissions, etc... ?
    bra_attr_t attributes;    //!< file attributes: #BRA_ATTR_FILE (regular) or #BRA_ATTR_DIR (directory)
    uint8_t    name_size;     //!< length in bytes excluding the trailing NUL; [1..UINT8_MAX]
    char*      name;          //!< filename/dirname (owned; free via @ref bra_meta_file_free)
    uint64_t   data_size;     //!< file contents size in bytes. Not saved for dir.
} bra_meta_file_t;

/**
 * @brief
 */
typedef struct bra_rle_chunk_t
{
    bra_rle_counts_t        counts;    //!< counts is stored as -1, i.e. 0 means 1 and 255 means 256
    char                    value;     //!< the repeated char.
    struct bra_rle_chunk_t* pNext;     //!< to the next chunk, NULL if it is the last one.
} bra_rle_chunk_t;
