#pragma once

#include <stdint.h>    // for UINT8_MAX

/**
 * @brief Support only little endian machine at the moment
 *
 * @todo issue with big endian (not supporting it for now)
 * @todo can use a magic string instead so it will be endian independent
 */
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#error "Big-endian is not supported yet; add endian-neutral (LE) serialization."
#endif

#if defined(__GNUC__) || defined(__clang__)
#define BRA_FUNC_ATTR_CONSTRUCTOR __attribute__((constructor))
#define BRA_ATTR_FALLTHROUGH      __attribute__((fallthrough))
#elif defined(_WIN32) || defined(_WIN64)
#define BRA_FUNC_ATTR_CONSTRUCTOR
#define BRA_ATTR_FALLTHROUGH
#else
#define BRA_FUNC_ATTR_CONSTRUCTOR
#define BRA_ATTR_FALLTHROUGH
#endif

// Enable printf-like format checking where supported
#if defined(__clang__)
#define BRA_FUNC_ATTR_FMT_PRINTF(fmt_idx, va_idx) __attribute__((format(printf, fmt_idx, va_idx)))
#elif defined(__GNUC__)
#define BRA_FUNC_ATTR_FMT_PRINTF(fmt_idx, va_idx) __attribute__((format(gnu_printf, fmt_idx, va_idx)))
#else
#define BRA_FUNC_ATTR_FMT_PRINTF(fmt_idx, va_idx)
#endif


#define BRA_MAGIC        0x612D5242    //!< 0x61='a' 0x2D='-' 0x52='R' 0x42='B'
#define BRA_FOOTER_MAGIC 0x782D5242    //!< 0x78='x' 0x2D='-' 0x52='R' 0x42='B'
// #define BRA_ARCHIVE_VERSION  1             //!< the file archive version
#define BRA_FILE_EXT         ".BRa"
#define BRA_NAME             "BRa"
#define BRA_SFX_FILENAME     "bra.sfx"    // @todo: generate it through cmake conf
#define BRA_SFX_TMP_FILE_EXT ".tmp"

#define BRA_DIR_DELIM "/"


#define BRA_ATTR_TYPE_MASK         ((bra_attr_t) 0x03)                                                 //!< lower 2 bits encode the type
#define BRA_ATTR_TYPE(x)           ((bra_attr_t) (x) & BRA_ATTR_TYPE_MASK)                             //!< first 2 bits
#define BRA_ATTR_SET_TYPE(x, type) ((bra_attr_t) ((x) & ~BRA_ATTR_TYPE_MASK) | BRA_ATTR_TYPE(type))    //!< set first 2 bits.
#define BRA_ATTR_TYPE_FILE         0                                                                   //!< Regular file.
#define BRA_ATTR_TYPE_DIR          1                                                                   //!< Directory
#define BRA_ATTR_TYPE_SYM          2                                                                   //!< Symlink
#define BRA_ATTR_TYPE_SUBDIR       3                                                                   //!< sub-dir, its base is an index of another dir/sub_dir previously encountered.

#define BRA_ATTR_STORED (0 << 2)
// #define BRA_ATTR_BWT_MTF_RLE      (1 << 2)
// #define BRA_ATTR_BWT_MTD_RLE_LZ78 (2 << 2)
#define BRA_ATTR_COMPRESSED (1 << 2)


// #define BRA_ATTR_ERR  0xFF    //!< unknown or not implemented ATTR

#define BRA_PRINTF_FMT_BYTES_BUF_SIZE 12                 //!< buffer size to convert meta file sizes into char*.

#define BRA_PRINTF_FMT_FILENAME_MAX_LENGTH 50            //!< max filename length before being truncated
#define BRA_PRINTF_FMT_FILENAME            "%-50.50s"    //!< printf-like format to print a filename.

#define BRA_SFX_FILE_EXT_LIN ".brx"
#define BRA_SFX_FILE_EXT_WIN ".exe"

#if defined(__APPLE__) || defined(__linux__) || defined(__unix__)
#define BRA_SFX_FILE_EXT BRA_SFX_FILE_EXT_LIN
#elif defined(_WIN32) || defined(_WIN64)
#define BRA_SFX_FILE_EXT BRA_SFX_FILE_EXT_WIN
#else
#error "unsupported platform" // maybe it could work anyway, but I did't test it.
#endif

#define BRA_MAX_PATH_LENGTH (UINT8_MAX + 1)    //!< capacity including trailing NUL; max on-disk name_size = UINT8_MAX (255).
#define BRA_MAX_CHUNK_SIZE  (256 * 1024)       //!< Use #BRA_MAX_CHUNK_SIZE for optimal I/O performance during file transfers (256KB).
#define BRA_MAX_RLE_COUNTS  UINT8_MAX          //!< Maximum encoded count value (255) representing runs up to 256 bytes (count = run_length - 1).
