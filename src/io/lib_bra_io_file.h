#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <lib_bra_types.h>


/**
 * @brief Print an error message and close the file.
 *
 * @param f
 * @param verb a string to complete the error message: "unable to %s".
 */
void bra_io_file_error(bra_io_file_t* f, const char* verb);

/**
 * @brief Print an error message and eventually close the file
 *
 * @see bra_io_file_error
 *
 * @param f
 */
void bra_io_file_open_error(bra_io_file_t* f);

/**
 * @brief Print an error message and close the file.
 *
 * @see bra_io_file_error
 *
 * @param f
 */
void bra_io_file_read_error(bra_io_file_t* f);

/**
 * @brief Print an error message and close the file.
 *
 * @see bra_io_file_error
 *
 * @param f
 */
void bra_io_file_seek_error(bra_io_file_t* f);

/**
 * @brief Print an error message and close the file.
 *
 * @see bra_io_file_error
 *
 * @param f
 */
void bra_io_file_write_error(bra_io_file_t* f);

/**
 * @brief Detect if the given filename @p fn is an ELF.
 *
 * @todo merge with PE_EXE to check at once if it is one or the other.
 *       or just open the file with bra_io_file_open and then do the checks instead.
 *       but once read it can do both checks at the same time.
 *
 * @see bra_io_file_is_pe_exe
 * @see bra_io_file_is_sfx
 *
 * @param fn
 * @return true if ELF magic is detected.
 * @return false otherwise.
 */
bool bra_io_file_is_elf(const char* fn);

/**
 * @brief Detect if the given filename @p fn is a PE/EXE.
 *
 * @see bra_io_file_is_elf
 * @see bra_io_file_is_sfx
 *
 * @param fn
 * @return true if a valid PE signature is detected.
 * @return false otherwise.
 */
bool bra_io_file_is_pe_exe(const char* fn);

/**
 * @brief Detect if the given filename @p fn is a possible SFX archive.
 *
 * @todo consider to open the file once instead of 2 times.
 *
 * @see bra_io_file_is_elf
 * @see bra_io_file_is_pe_exe
 *
 * @param fn
 * @return true if a file appear to be an ELF or PE/EXE (SFX wrapper).
 * @return false otherwise.
 */
bool bra_io_file_is_sfx(const char* fn);

/**
 * @brief Open the file @p fn in the @p mode.
 *        On failure there is no need to call @ref bra_io_file_close
 *
 * @param f[out]
 * @param fn
 * @param mode @c fopen modes
 * @return true on success
 * @return false on error and close @p f via @ref bra_io_file_close.
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
 * @return true
 * @return false
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
 * @return true
 * @return false
 */
bool bra_io_file_read_footer(bra_io_file_t* f, bra_io_footer_t* bf_out);

/**
 * @brief Write the footer into the file @p f.
 *        On error closes @p f via @ref bra_io_file_close.
 *
 * @param f
 * @param header_offset
 * @return true
 * @return false
 */
bool bra_io_file_write_footer(bra_io_file_t* f, const int64_t header_offset);

/**
 * @brief Copy from @p src to @p dst in chunks size of #BRA_MAX_CHUNK_SIZE for @p data_size bytes
 *        the files must be positioned at the correct read/write offsets.
 *        On failure closes both @p dst and @p src via @ref bra_io_file_close
 *
 * @param dst
 * @param src
 * @param data_size
 * @return true
 * @return false
 */
bool bra_io_file_copy_file_chunks(bra_io_file_t* dst, bra_io_file_t* src, const uint64_t data_size);

/**
 * @brief Move @p f forward of @p data_size bytes.
 *
 * @param f
 * @param data_size
 * @return true
 * @return false
 */
bool bra_io_file_skip_data(bra_io_file_t* f, const uint64_t data_size);


#ifdef __cplusplus
}
#endif
