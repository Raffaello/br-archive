#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <lib_bra_types.h>

#include <stdbool.h>
#include <stdint.h>

// TODO:  later on as exercise discard the c++ filesystem and do it in native OS APIs with C (bra_fs.c)
//        no wrapper in the c++ filesystem. [not important for now]

/**
 * @brief Return if the given @p path is a directory.
 *
 * @overload bra::fs::dir_exists(const std::filesystem::path& path)
 *
 * @param path
 * @retval true exists
 * @retval false not exists or error.
 */
bool bra_fs_dir_exists(const char* path);

/**
 * @brief Create a directory given by @p path.
 *        It creates also the parent directory if necessary.
 *
 * @overload bool bra::fs::dir_make(const std::filesystem::path& path)
 *
 * @param path
 * @retval true on success
 * @retval false on error
 */
bool bra_fs_dir_make(const char* path);

/**
 * @brief Check if the @p dst is a sub-dir of @p src relative to the current directory.
 *
 * @overload bool bra::fs::dir_isSubDir(const std::filesystem::path& src, const std::filesystem::path dst)
 *
 * @param src
 * @param dst
 * @retval true
 * @retval false
 */
bool bra_fs_dir_is_sub_dir(const char* src, const char* dst);

/**
 * @brief Return if the given @p path is a file.
 *
 * @overload bra::fs::file_exists(const std::filesystem::path& path)
 *
 * @param path
 * @retval true exists
 * @retval false if it does not exist or on error (I/O errors are not distinguished).
 */
bool bra_fs_file_exists(const char* path);

/**
 * @brief Get the file attributes from the given @p path and store them in @p attr.
 *
 * @note Wrapper around @ref bra::fs::file_attributes.
 *
 *
 * @param path
 * @param attr
 * @retval true on success.
 * @retval false on error (including NULL @p path or @p attr).
 */
bool bra_fs_file_attributes(const char* path, bra_attr_t* attr);

/**
 * @brief Get the file size from the given @p path and store it in @p file_size.
 *
 * @note Wrapper around @ref bra::fs::file_size.
 *
 * @param path
 * @param file_size
 * @retval true on success.
 * @retval false on error (including NULL @p path or @p file_size).
 */
bool bra_fs_file_size(const char* path, uint64_t* file_size);

/**
 * @brief If a file @p path exists ask to overwrite it.
 *
 * @overload std::optional<bool> file_exists_ask_overwrite(const std::filesystem::path& path, const bra_fs_overwrite_policy_e overwrite_policy)
 *
 * @param path
 * @param overwrite_policy
 * @param single_overwrite if true All and None extra choice are not enabled.
 * @retval true  if the file does not exist or overwrite is approved.
 * @retval false if the file exists and overwrite is declined, or on error (e.g., NULL @p path).
 */
bool bra_fs_file_exists_ask_overwrite(const char* path, bra_fs_overwrite_policy_e* overwrite_policy, const bool single_overwrite);

#ifdef __cplusplus
}
#endif
