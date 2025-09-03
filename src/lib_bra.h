#pragma once


#ifdef __cplusplus
extern "C" {
#endif

#include <lib_bra_defs.h>
#include <lib_bra_types.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>

/**
 * @brief Function Prototype for the message callback function.
 */
typedef int bra_message_callback_f(const char* fmt, va_list args) BRA_FUNC_ATTR_FMT_PRINTF(1, 0);


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
    char*      name;          //!< filename (owned; free via @ref bra_meta_file_free)
    uint64_t   data_size;     //!< file contents size in bytes
} bra_meta_file_t;

/////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief strdup()
 *
 * @details Returns @c NULL if @p str is @c NULL or on allocation failure.
 *          The caller owns the returned buffer and must free() it.
 *
 * @todo remove when switching to C23
 *
 * @param str
 * @return char*
 */
char* bra_strdup(const char* str);

/**
 * @brief Set the message callback to call to write messages to the user.
 *        The default one is @c vprintf.
 *        If passing @c NULL restores the default one.
 *
 * @param msg_cb
 */
void bra_set_message_callback(bra_message_callback_f* msg_cb);

/**
 * @brief This call the message callback.
 *
 * @todo should be instead implemented in bra_log.{h,c} ?
 *       And use the bra_log_info on stdout instead of stderr?
 *
 * @param fmt
 * @param ...
 */
int bra_printf_msg(const char* fmt, ...) BRA_FUNC_ATTR_FMT_PRINTF(1, 2);

/**
 * @brief vprintf-style variant that forwards an existing va_list to the callback.
 */
int bra_vprintf_msg(const char* fmt, va_list args) BRA_FUNC_ATTR_FMT_PRINTF(1, 0);

/**
 * @brief Print an error message and close the file.
 *
 * @param bf
 * @param verb a string to complete the error message: "unable to %s".
 */
void bra_io_file_error(bra_io_file_t* bf, const char* verb);

/**
 * @brief Print an error message and eventually close the file
 *
 * @see bra_io_file_error
 *
 * @param bf
 */
void bra_io_file_open_error(bra_io_file_t* bf);

/**
 * @brief Print an error message and close the file.
 *
 * @see bra_io_file_error
 *
 * @param bf
 */
void bra_io_file_read_error(bra_io_file_t* bf);

/**
 * @brief Print an error message and close the file.
 *
 * @see bra_io_file_error
 *
 * @param bf
 */
void bra_io_file_seek_error(bra_io_file_t* bf);

/**
 * @brief Print an error message and close the file.
 *
 * @see bra_io_file_error
 *
 * @param bf
 */
void bra_io_file_write_error(bra_io_file_t* bf);

/**
 * @brief open the file @p fn in the @p mode.
 *        The file @p bf will be overwritten if it exists.
 *        On failure there is no need to call @ref bra_io_close
 *
 * @param bf
 * @param fn
 * @param mode
 * @return true on success
 * @return false on error and close @p bf via @ref bra_io_close.
 */
bool bra_io_open(bra_io_file_t* bf, const char* fn, const char* mode);

/**
 * @brief Close the file, free the internal memory and set the fields to NULL.
 *
 * @param bf
 */
void bra_io_close(bra_io_file_t* bf);

/**
 * @brief Seek file at position @p offs.
 *
 * @param f
 * @param offs
 * @param origin SEEK_SET, SEEK_CUR, SEEK_END
 * @return true
 * @return false
 */
bool bra_io_seek(bra_io_file_t* f, const int64_t offs, const int origin);

/**
 * @brief Tell the file position.
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
bool bra_io_read_header(bra_io_file_t* bf, bra_io_header_t* out_bh);

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
 * @brief Read the bra footer into @p bf_out.
 *        On error closes @p f via @ref bra_io_close.
 *
 * @param f
 * @param bf_out
 * @return true
 * @return false
 */
bool bra_io_read_footer(bra_io_file_t* f, bra_io_footer_t* bf_out);

/**
 * @brief Write the footer into the file @p f.
 *        On error closes @p f via @ref bra_io_close.
 *
 * @param f
 * @param header_offset
 * @return true
 * @return false
 */
bool bra_io_write_footer(bra_io_file_t* f, const int64_t header_offset);

/**
 * @brief Read the filename meta data information that is pointing in @p f and store it on @p mf.
 *
 * @param f
 * @param mf
 * @return true on success @p mf must be explicitly free via @ref bra_meta_file_free.
 * @return false on error closes @p f via @ref bra_io_close.
 */
bool bra_io_read_meta_file(bra_io_file_t* f, bra_meta_file_t* mf);

/**
 * @brief Write the filename meta data information given from @p mf in @p f.
 *
 * @param f
 * @param mf
 * @return true on success
 * @return false on error closes @p f via @ref bra_io_close.
 */
bool bra_io_write_meta_file(bra_io_file_t* f, const bra_meta_file_t* mf);

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
 * @brief Encode a file  or directory @p fn and append it to the open archive @p f.
 *        On error closes @p f via @ref bra_io_close.
 *
 * @param f Open archive handle.
 * @param fn NULL-terminated path to file or directory.
 * @return true on success
 * @return false on error (archive handle is closed)
 */
bool bra_io_encode_and_write_to_disk(bra_io_file_t* f, const char* fn);

/**
 * @brief Decode the current pointed internal file contained in @p f and write it to its relative path on disk.
 *        On error closes @p f via @ref bra_io_close.
 *
 * @param f
 * @return true on success
 * @return false on error
 */
bool bra_io_decode_and_write_to_disk(bra_io_file_t* f);

#ifdef __cplusplus
}
#endif
