#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <lib_bra_types.h>
#include <stdbool.h>


/**
 * @brief Log an error message and close the file.
 *
 * @param f
 * @param verb a string to complete the error message: "unable to %s".
 */
void bra_io_file_error(bra_io_file_t* f, const char* verb);

/**
 * @brief Log an error message and close the file.
 *
 * @see bra_io_file_error
 *
 * @param f
 */
void bra_io_file_open_error(bra_io_file_t* f);

/**
 * @brief Log an error message and close the file.
 *
 * @see bra_io_file_error
 *
 * @param f
 */
void bra_io_file_read_error(bra_io_file_t* f);

/**
 * @brief Log an error message and close the file.
 *
 * @see bra_io_file_error
 *
 * @param f
 */
void bra_io_file_seek_error(bra_io_file_t* f);

/**
 * @brief Log an error message and close the file.
 *
 * @see bra_io_file_error
 *
 * @param f
 */
void bra_io_file_write_error(bra_io_file_t* f);

/**
 * @brief Detect if the given filename @p fn is an @c ELF file.
 *
 * @todo merge with PE_EXE to check at once if it is one or the other.
 *       or just open the file with bra_io_file_open and then do the checks instead.
 *       but once read it can do both checks at the same time.
 *
 * @see bra_io_file_is_pe_exe
 * @see bra_io_file_can_be_sfx
 *
 * @param fn
 * @retval true if ELF magic is detected.
 * @retval false otherwise.
 */
bool bra_io_file_is_elf(const char* fn);

/**
 * @brief Detect if the given filename @p fn is a PE/EXE.
 *
 * @see bra_io_file_is_elf
 * @see bra_io_file_can_be_sfx
 *
 * @param fn
 * @retval true if a valid PE signature is detected.
 * @retval false otherwise.
 */
bool bra_io_file_is_pe_exe(const char* fn);

/**
 * @brief Detect if the given filename @p fn is a possible SFX archive.
 *
 * @note Implementation opens once and checks both ELF and PE signatures.
 *
 * @see bra_io_file_is_elf
 * @see bra_io_file_is_pe_exe
 *
 * @param fn
 * @retval true if the file appears to be an ELF or PE/EXE (SFX wrapper).
 * @retval false otherwise.
 */
bool bra_io_file_can_be_sfx(const char* fn);

/**
 * @brief Open the file @p fn in the @p mode.
 *        On failure there is no need to call @ref bra_io_file_close
 *
 * @param f[out]   file wrapper to initialize.
 * @param fn[in]   filename to open.
 * @param mode[in] @c fopen modes
 * @retval true    on success
 * @retval false   on error and close @p f via @ref bra_io_file_close.
 */
bool bra_io_file_open(bra_io_file_t* f, const char* fn, const char* mode);

/**
 * @brief Close the file, free the internal memory and set the fields to NULL.
 *
 * @param f
 */
void bra_io_file_close(bra_io_file_t* f);

/**
 * @brief Seek file at position @p offs.
 *
 * @param f
 * @param offs
 * @param origin SEEK_SET, SEEK_CUR, SEEK_END
 * @retval true
 * @retval false
 */
bool bra_io_file_seek(bra_io_file_t* f, const int64_t offs, const int origin);

/**
 * @brief Tell the file position.
 *        On error returns -1
 *
 * @param f
 * @return int64_t
 */
int64_t bra_io_file_tell(bra_io_file_t* f);

/**
 * @brief Read the bra footer into @p bf_out.
 *        On error closes @p f via @ref bra_io_file_close.
 *
 * @param f
 * @param bf_out
 * @retval true
 * @retval false
 */
bool bra_io_file_read_footer(bra_io_file_t* f, bra_io_footer_t* bf_out);

/**
 * @brief Write the footer into the file @p f.
 *        On error closes @p f via @ref bra_io_file_close.
 *
 * @param f
 * @param header_offset
 * @retval true
 * @retval false
 */
bool bra_io_file_write_footer(bra_io_file_t* f, const int64_t header_offset);

/**
 * @brief Read a chunk of data from the source file.
 *
 * @param src      Source file wrapper.
 * @param buf      Buffer to read data into.
 * @param buf_size Size of the buffer.
 * @param me       Metadata entry whose crc32 field is updated (must not be @c NULL).
 * @retval true    On success.
 * @retval false   On error. ( @p src is closed)
 */
bool bra_io_file_read_chunk(bra_io_file_t* src, void* buf, const size_t buf_size, bra_meta_entry_t* me);

/**
 * @brief Read data_size bytes from src in #BRA_MAX_CHUNK_SIZE chunks and update @p me->crc32.
 *        The file must be positioned at the start of the entry's data. On success,
 *        the stream is advanced by data_size bytes (positioned at the entry's trailing CRC32).
 *        On error, logs and closes @p src via @ref bra_io_file_read_error().
 *
 * @param src[in,out]    input file wrapper (advanced by data_size on success; closed on error).
 * @param data_size[in]  number of bytes to read from the current position.
 * @param me[in,out]     metadata entry whose crc32 field is updated (must not be @c NULL).
 * @retval true          on success
 * @retval false         on error ( @p src is closed)
 */
bool bra_io_file_read_file_chunks(bra_io_file_t* src, const uint64_t data_size, bra_meta_entry_t* me);

/**
 * @brief Copy from @p src to @p dst in chunks size of #BRA_MAX_CHUNK_SIZE for @p data_size bytes
 *        the files must be positioned at the correct read/write offsets.
 *        On failure closes both @p dst and @p src via @ref bra_io_file_close
 *
 * @param dst
 * @param src
 * @param data_size
 * @param me        passed to compute the CRC32C of the file content, not @c NULL and @p me->crc32 is updated.
 * @retval true
 * @retval false
 */
bool bra_io_file_copy_file_chunks(bra_io_file_t* dst, bra_io_file_t* src, const uint64_t data_size, bra_meta_entry_t* me);

/**
 * @brief Move @p f forward of @p data_size bytes.
 *
 * @param f
 * @param data_size
 * @retval true
 * @retval false
 */
bool bra_io_file_skip_data(bra_io_file_t* f, const uint64_t data_size);


#ifdef __cplusplus
}
#endif
