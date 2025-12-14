#pragma once

#include <lib_bra_types.h>
#include <io/lib_bra_io_file.h>

#include <stdbool.h>

/**
 * @brief write the common meta entry data.
 *
 * @param f the target file
 * @param me the meta entry to write to @p f
 * @retval true on success
 * @retval false on error
 */
bool bra_io_file_meta_entry_write_common_header(bra_io_file_t* f, const bra_meta_entry_t* me);

/**
 * @brief read the @ref bra_meta_entry_file_t into @p me
 *
 * @param f the source file to read from.
 * @param me where to store the read data.
 * @retval true on success
 * @retval false on error
 */
bool bra_io_file_meta_entry_read_file_entry(bra_io_file_t* f, bra_meta_entry_t* me);

/**
 * @brief Write the @ref bra_meta_entry_file_t from @p me
 *
 * @param f the destination file to write from.
 * @param me the data to be written.
 * @retval true on success
 * @retval false on error
 */
bool bra_io_file_meta_entry_write_file_entry(bra_io_file_t* f, const bra_meta_entry_t* me);

/**
 * @brief Read the meta entry subdir data
 *
 * @param f the file to read from
 * @param me the meta entry to store the meta subdir data.
 * @retval true on success
 * @retval false on error
 */
bool bra_io_file_meta_entry_read_subdir_entry(bra_io_file_t* f, bra_meta_entry_t* me);

/**
 * @brief Write the meta entry subdir data.
 *
 * @param f the file to write to.
 * @param me the meta entry to read the meta subdir data from.
 * @retval true on success
 * @retval false on error
 */
bool bra_io_file_meta_entry_write_subdir_entry(bra_io_file_t* f, const bra_meta_entry_t* me);

/**
 * @brief Flush the whole meta entry file.
 *
 * @see bra_io_file_meta_entry_write_common_header
 * @see bra_io_file_meta_entry_write_file_entry
 *
 * @param f the file to write to
 * @param me the meta entry file.
 * @param filename the original filename with its path to be archived.
 * @param filename_len the length of @p filename (to avoid to recompute it internally)
 * @retval true on success
 * @retval false on failure
 */
bool bra_io_file_meta_entry_flush_entry_file(bra_io_file_t* f, bra_meta_entry_t* me, const char* filename, const size_t filename_len);

/**
 * @brief Flush the whole meta entry directory.
 *
 * @param f the file to write to
 * @param me the meta entry directory
 * @retval true on success
 * @retval false on failure
 */
bool bra_io_file_meta_entry_flush_entry_dir(bra_io_file_t* f, const bra_meta_entry_t* me);

/**
 * @brief Flush the whole meta entry subdirectory
 *
 * @param f the file to write to
 * @param me the meta entry subdirectory.
 * @retval true on success
 * @retval false on failure
 */
bool bra_io_file_meta_entry_flush_entry_subdir(bra_io_file_t* f, const bra_meta_entry_t* me);

/**
 * @brief Computes the CRC32 of the whole meta entry
 *
 * @param f the file to be read from
 * @param filename_len the length of @p filename
 * @param filename the reconstructed full path of the meta entry name
 * @param me the meta entry
 * @retval true on success
 * @retval false on failures
 */
bool bra_io_file_meta_entry_compute_crc32(bra_io_file_t* f, const size_t filename_len, const char* filename, bra_meta_entry_t* me);
