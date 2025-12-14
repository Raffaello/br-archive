#pragma once

#include <lib_bra_types.h>
#include <io/lib_bra_io_file.h>

#include <stdbool.h>

/**
 * @brief
 *
 * @param f
 * @param attr
 * @param filename
 * @param filename_size
 * @return true
 * @return false
 */
bool bra_io_file_meta_entry_write_common_header(bra_io_file_t* f, const bra_attr_t attr, const char* filename, const uint8_t filename_size);

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
 * @brief
 *
 * @param f
 * @param me
 * @return true
 * @return false
 */
bool bra_io_file_meta_entry_read_subdir_entry(bra_io_file_t* f, bra_meta_entry_t* me);
// bool bra_io_file_meta_entry_write_subdir_entry(bra_io_file_t* f, const bra_meta_entry_t* me);

/**
 * @brief
 *
 * @param f
 * @param me
 * @param filename
 * @param filename_len
 * @return true
 * @return false
 */
bool bra_io_file_meta_entry_flush_entry_file(bra_io_file_t* f, bra_meta_entry_t* me, const char* filename, const size_t filename_len);

/**
 * @brief
 *
 * @param f
 * @param me
 * @return true
 * @return false
 */
bool bra_io_file_meta_entry_flush_entry_dir(bra_io_file_t* f, const bra_meta_entry_t* me);

/**
 * @brief
 *
 * @param f
 * @param me
 * @param parent_index
 * @return true
 * @return false
 */
bool bra_io_file_meta_entry_flush_entry_subdir(bra_io_file_t* f, const bra_meta_entry_t* me);
