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
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#error "Big-endian is not supported yet; add endian-neutral (LE) serialization."
#endif


#define BRA_MAGIC            0x612D5242    //!< 0x61='a' 0x2D='-' 0x52='R' 0x42='B'
#define BRA_FOOTER_MAGIC     0x782D5242    //!< 0x78='x' 0x2D='-' 0x52='R' 0x42='B'
#define BRA_FILE_EXT         ".BRa"
#define BRA_NAME             "BRa"
#define BRA_SFX_FILENAME     "bra.sfx"    // @todo: generate it through cmake conf
#define BRA_SFX_TMP_FILE_EXT ".tmp"

#if defined(__APPLE__) || defined(__linux__) || defined(__linux)
#define BRA_SFX_FILE_EXT ".brx"
#elif defined(_WIN32) || defined(_WIN64)
#define BRA_SFX_FILE_EXT ".exe"
#else
#error "unsupported platform" // maybe it could work anyway, but I did't test it.
#endif


#define MAX_CHUNK_SIZE (256 * 1024)    //!< Use MAX_CHUNK_SIZE for optimal I/O performance during file transfers (256KB)


#pragma pack(push, 1)

/**
 * @brief BR-Archive File Header.
 */
typedef struct bra_header_t
{
    uint32_t magic;    //!< 'BR-a'
    // version
    // compression type ? (archive, best, fast, ... ??)
    // crc32 / md5 ?
    uint32_t num_files;    // just 1 for now

    // TODO: put num_directories. 1 must be always present and then
    //       num_files inside the bra_meta_directory_t
    //       it means that is an internal directory containing files.
    //
    //       this leads to save chars on the filename string,
    //       for multiple files in the same directory
    //       also it will mostly save the slash.
    // TODO: ISSUE: how about nested sub-directories instead?
    //       so the directory should contain a num_directory.
    //       basically the header is just follow of a bra_directory_t instead.
} bra_header_t;

/**
 * @brief BR-Sfx File Footer.
 */
typedef struct bra_footer_t
{
    uint32_t magic;          //!< 'BR-x'
    int64_t  data_offset;    //!< where the data chunk start from the beginning of the file
} bra_footer_t;

/**
 * @brief This is the metadata of each file stored in a BR-archive.
 *        The file data is just after for @p data_size bytes.
 */
typedef struct bra_meta_file_t
{
    // TODO: add CRC ... file permissions, file attributes, etc... ?
    uint8_t  name_size;
    char*    name;
    uint64_t data_size;
} bra_meta_file_t;

#pragma pack(pop)

typedef struct bra_io_file_t
{
    FILE* f;
    char* fn;
} bra_io_file_t;

/**
 * @brief print error message and close file.
 *
 * @param bf
 */
void bra_io_read_error(bra_io_file_t* bf);

/**
 * @brief open the file @p fn in the @p mode
 *        the @p will be overwritten.
 *        On failure there is no need to call @ref bra_io_close
 *
 * @param bf
 * @param fn
 * @param mode
 * @return true on success
 * @return false on error
 */
bool bra_io_open(bra_io_file_t* bf, const char* fn, const char* mode);

/**
 * @brief close file, free internal memory and set fields to NULL.
 *
 * @param bf
 */
void bra_io_close(bra_io_file_t* bf);

/**
 * @brief seek file at position @p offs.
 *  *
 * @param f
 * @param offs
 * @param origin SEEK_SET, SEEK_CUR, SEEK_END
 * @return true
 * @return false
 */
bool bra_io_seek(bra_io_file_t* f, const int64_t offs, const int origin);

/**
 * @brief tell the file position.
 *        On error returns -1
 *
 * @param f
 * @return int64_t
 */
int64_t bra_io_tell(bra_io_file_t* f);

/**
 * @brief Read the header from the give @p bf file.
 *        The file must be positioned at the beginning of the header.
 *        On error returns false and closes the file via @ref bra_io_close.
 *
 * @param bf
 * @param out_bh
 * @return true on success
 * @return false on error
 */
bool bra_io_read_header(bra_io_file_t* bf, bra_header_t* out_bh);

/**
 * @brief Write the bra header into @p bf with @p num_files.
 *        On error closes @p f via @ref bra_io_close.
 *
 * @param f
 * @param num_files
 * @return true
 * @return false
 */
bool bra_io_write_header(bra_io_file_t* f, const uint32_t num_files);

/**
 * @brief Read thr bra footer into @p bf_out.
 *        On error closes @p f via @ref bra_io_close.
 *
 * @param f
 * @param bf_out
 * @return true
 * @return false
 */
bool bra_io_read_footer(bra_io_file_t* f, bra_footer_t* bf_out);

/**
 * @brief Write the footer into the file @p f.
 *        On error closes @p f via @ref bra_io_close.
 *
 *
 * @param f
 * @param data_offset
 * @return true
 * @return false
 */
bool bra_io_write_footer(bra_io_file_t* f, const int64_t data_offset);

/**
 * @brief Read the filename meta data information that is pointing in @p f and store it on @p mf.
 *        On error closes @p f via @ref bra_io_close.
 *        On success @mf must be explicitly free via @ref bra_meta_file_free.
 *
 * @param f
 * @param mf
 * @return true On success
 * @return false On error
 */
bool bra_io_read_meta_file(bra_io_file_t* f, bra_meta_file_t* mf);

/**
 * @brief Free any eventual content on @p mf.
 *
 * @param mf
 */
void bra_meta_file_free(bra_meta_file_t* mf);

/**
 * @brief Copy from @p src to @p dst in chunks size of #MAX_CHUNK_SIZE for @p data_size bytes
 *        the files must be positioned at the correct read/write offsets.
 *        On failure closes both @p dst and @p src via @ref bra_io_close
 *
 * @param dst
 * @param src
 * @param data_size
 * @return true
 * @return false
 */
bool bra_io_copy_file_chunks(bra_io_file_t* dst, bra_io_file_t* src, const uint64_t data_size);

/**
 * @brief Move @p f forward of @p data_size bytes.
 *
 * @param f
 * @param data_size
 * @return true
 * @return false
 */
bool bra_io_skip_data(bra_io_file_t* f, const uint64_t data_size);

/**
 * @brief Decode the current pointed internal file contained in @p f and write it to its relative path on disk.
 *        On error calls @ref bra_io_close with param @p f closing it.
 *
 *
 * @param f
 * @return true on success
 * @return false on error
 */
bool bra_io_decode_and_write_to_disk(bra_io_file_t* f);

#ifdef __cplusplus
}
#endif
