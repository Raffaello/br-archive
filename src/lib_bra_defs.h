#pragma once

/**
 * @brief Support only little endian machine at the moment
 *
 * @todo issue with big endian (not supporting it for now)
 * @todo can use a magic string instead so it will be endian independent
 */
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#error "Big-endian is not supported yet; add endian-neutral (LE) serialization."
#endif

#define BRA_MAGIC        0x612D5242    //!< 0x61='a' 0x2D='-' 0x52='R' 0x42='B'
#define BRA_FOOTER_MAGIC 0x782D5242    //!< 0x78='x' 0x2D='-' 0x52='R' 0x42='B'
// #define BRA_ARCHIVE_VERSION  1             //!< the file archive version
#define BRA_FILE_EXT         ".BRa"
#define BRA_NAME             "BRa"
#define BRA_SFX_FILENAME     "bra.sfx"    // @todo: generate it through cmake conf
#define BRA_SFX_TMP_FILE_EXT ".tmp"

#define BRA_ATTR_FILE 0    //!< Regular file.
#define BRA_ATTR_DIR  1    //!< Directory
// #define BRA_ATTR_ERR  0xFF    //!< unknown or not implemented ATTR

#if defined(__APPLE__) || defined(__linux__) || defined(__linux)
#define BRA_SFX_FILE_EXT ".brx"
#elif defined(_WIN32) || defined(_WIN64)
#define BRA_SFX_FILE_EXT ".exe"
#else
#error "unsupported platform" // maybe it could work anyway, but I did't test it.
#endif

#define BRA_MAX_PATH_LENGTH (UINT8_MAX + 1)    //!< capacity including trailing NUL; max on-disk name_size = UINT8_MAX (255)

#define MAX_CHUNK_SIZE (256 * 1024)            //!< Use MAX_CHUNK_SIZE for optimal I/O performance during file transfers (256KB)
