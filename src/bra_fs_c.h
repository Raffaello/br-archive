#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "lib_bra_types.h"

#include <stdbool.h>
#include <stdint.h>


// int bra_fs_dir_exists(const char* path);

/**
 * @brief Create a directory given by @p path.
 *        It creates also the parent directory if necessary.
 *
 * @overload bool bra::fs::dir_make(const std::filesystem::path& path)
 *
 * @param path
 * @return true on success
 * @return false on error
 */
bool bra_fs_dir_make(const char* path);

/**
 * @brief Get the file attributes from the given @p path and store them in @p attr.
 *
 * @note Wrapper around @ref bra::fs::file_attributes.
 *
 *
 * @param path
 * @param attr
 * @return true on success.
 * @return false on error (including NULL @p path or @p attr).
 */
bool bra_fs_file_attributes(const char* path, bra_attr_t* attr);

/**
 * @brief Get the file size from the given @p path and store it in @p file_size.
 *
 * @note Wrapper around @ref bra::fs:file_size.
 *
 * @param path
 * @param file_size
 * @return true on success.
 * @return false on error (including NULL @p path or @p file_size).
 */
bool bra_fs_file_size(const char* path, uint64_t* file_size);

#ifdef __cplusplus
}
#endif
