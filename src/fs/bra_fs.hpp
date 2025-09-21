#pragma once

#ifndef __cplusplus
#error "bra_fs.hpp must be included from a C++ translation unit. Use bra_fs_c.h when compiling as C."
#endif

#if __cplusplus < 201703L && !(defined(_MSC_VER) && (_MSVC_LANG >= 201703L))
#error "requires C++17 or later (std::filesystem)."
#endif


#include <lib_bra_types.h>

#include <filesystem>
#include <optional>
#include <string>
#include <list>
#include <set>
#include <map>

namespace bra::fs
{

/**
 * @brief Try to sanitize the @p path.
 *        It must be relative to the current directory.
 *        It can't escape the current directory.
 *        Existence is not checked; it only rewrites to a safe, relative path.
 *
 * @param path
 * @retval true if it is successful
 * @retval false otherwise
 */
[[nodiscard]] bool try_sanitize(std::filesystem::path& path) noexcept;

/**
 * @brief Check if the given @p path exists and is a directory.
 *
 * @see file_exists
 *
 * @param path
 * @retval true if the path exists and is a directory.
 * @retval false otherwise.
 */
[[nodiscard]] bool dir_exists(const std::filesystem::path& path) noexcept;

/**
 * @brief Create the directory at @p path.
 *        Also creates parent directories as needed.
 *
 * @note Idempotent: returns true if the directory already exists.
 *
 * @see dir_exists
 *
 * @param path
 * @retval true if the directory was created or already existed.
 * @retval false on error
 */
[[nodiscard]] bool dir_make(const std::filesystem::path& path) noexcept;

/**
 * @brief Check if the @p path is a sub-dir of @p base relative to the current directory.
 *        Path outside the current directory return false.
 *
 * @param base
 * @param path
 * @retval true if @p path is a subdir of @p base;
 * @retval false otherwise or on error.
 */
[[nodiscard]] bool dir_isSubDir(const std::filesystem::path& base, const std::filesystem::path& path) noexcept;

/**
 * @brief Make sure the file extension is #BRA_FILE_EXT
 *
 * @param path
 * @return std::filesystem::path
 */
[[nodiscard]] std::filesystem::path filename_archive_adjust(const std::filesystem::path& path) noexcept;

/**
 * @brief Return the given filename ending always with the correct extension.
 *        if @p tmp is true, adjust the extension to be #BRA_FILE_EXT #BRA_SFX_TMP_FILE_EXT (e.g., .BRa.tmp)
 *        otherwise adjust to                           #BRA_FILE_EXT #BRA_SFX_FILE_EXT     (e.g., .BRa.exe or BRa.BRx)
 *
 * @param path
 * @param tmp
 * @return std::filesystem::path the adjusted path.
 */
[[nodiscard]] std::filesystem::path filename_sfx_adjust(const std::filesystem::path& path, const bool tmp) noexcept;

/**
 * @brief Check if the given @p path is a regular file and exists.
 *
 * @see dir_exists
 *
 * @param path
 * @retval true
 * @retval false
 */
[[nodiscard]] bool file_exists(const std::filesystem::path& path) noexcept;

/**
 * @brief Check if the file in @p path exists and ask the user to overwrite.
 *
 * @note The file is considered a 'regular_file' it won't check if it is a directory.
 *
 * @param path
 * @param overwrite_policy in/out overwrite policy; may be updated by user’s global choice.
 * @param single_overwrite if true, global choices (“Yes to all” / “No to all”) are disabled.
 * @return std::optional<bool> when has no value the file doesn't exist.
 * @retval true overwrite
 * @retval false don't overwrite
 */
[[nodiscard]] std::optional<bool> file_exists_ask_overwrite(const std::filesystem::path& path, bra_fs_overwrite_policy_e& overwrite_policy, const bool single_overwrite) noexcept;

/**
 * @brief Get the file attributes for the given @p path.
 *
 * @param base used to determine if it is a directory or subdirectory.
 * @param path
 * @return std::optional<bra_attr_t> #BRA_ATTR_TYPE_FILE    for regular files,
 *                                   #BRA_ATTR_TYPE_DIR     for directory,
 *                                   #BRA_ATTR_TYPE_SYM     for symlink;
 *                                   #BRA_ATTR_TYPE_SUBDIR  for subdirectory of @p base;
 *                                   @c nullopt             for errors or unknown types.
 */
[[nodiscard]] std::optional<bra_attr_t> file_attributes(const std::filesystem::path& path) noexcept;

/**
 * @brief Get the size of a file or directory.
 *
 * @param path
 * @return std::optional<uint64_t> File size in bytes for regular files, 0 for directories, @c nullopt on error
 */
[[nodiscard]] std::optional<uint64_t> file_size(const std::filesystem::path& path) noexcept;

/**
 * @brief Remove the file at the @p path.
 *
 * @param path
 * @retval true on success (also when the file does not exist)
 * @retval false on error
 */
[[nodiscard]] bool file_remove(const std::filesystem::path& path) noexcept;

/**
 * @brief  Rename/move a regular file from @p from to @p to, overwriting an existing regular file at @p to.
 *
 * @note No-op if @p from and @p to refer to the same file (returns true).
 *       Only regular files are supported; if @p to is a directory, the operation fails.
 *       May fall back to copy-then-remove across devices; this is not atomic and, on cleanup failure,
 *       the source may remain (a warning is logged).
 *
 * @param from Source file path.
 * @param to Destination file path.
 * @retval true on success.
 * @retval false on error
 */
[[nodiscard]] bool file_rename(const std::filesystem::path& from, const std::filesystem::path& to) noexcept;

/**
 * @brief set the @p permissions with options @p perm_options on the file @p path.
 *
 * @param path
 * @param permissions
 * @param perm_options
 * @retval true on success
 * @retval false on error
 */
[[nodiscard]] bool file_permissions(const std::filesystem::path& path, const std::filesystem::perms permissions, const std::filesystem::perm_options perm_options) noexcept;


/**
 * @brief Extend the @p files set with their directories.
 *
 * @details the parent directories from the files present in @p files are added into @p files.
 *
 * @param files
 * @retval true on success.
 * @retval false on error. The @p files set may be left in an invalid state.
 */
[[nodiscard]] bool file_set_add_dirs(std::set<std::filesystem::path>& files) noexcept;

/**
 * @brief Search for files in the given @p dir matching the regular expression @p pattern.
 *        It stores the results in @p out_files.
 *
 * @note  The existing contents of @p out_files are preserved (results are appended).
 *
 * @param dir
 * @param pattern Regular expression pattern (not a wildcard pattern)
 * @param out_files List to append matching file paths to
 * @param recursive If false performs a non-recursive search in the immediate directory only; recursive otherwise.
 * @retval true if successful
 * @retval false otherwise
 */
[[nodiscard]] bool search(const std::filesystem::path& dir, const std::string& pattern, std::list<std::filesystem::path>& out_files, const bool recursive) noexcept;

/**
 * @brief Expand the given @p wildcard_path and store the resulting paths into @p out_files.
 *        It doesn't clear @p out_files, but it adds on it.
 *
 * @param wildcard_path
 * @param out_files
 * @param recursive
 * @retval true on success and add the results to @p out_files
 * @retval false on error (unsupported wildcard, sanitization failure, or search failure).
 */
[[nodiscard]] bool search_wildcard(const std::filesystem::path& wildcard_path, std::set<std::filesystem::path>& out_files, const bool recursive) noexcept;

/**
 * @brief Make a tree from @p set_files into @p tree. Counting also all the elements into @p out_tree.
 *
 * @param set_files
 * @param tree
 * @param out_tree_size
 * @retval true
 * @retval false
 */
[[nodiscard]] bool make_tree(const std::set<std::filesystem::path>& set_files, std::map<std::filesystem::path, std::set<std::filesystem::path>>& tree, size_t& out_tree_size) noexcept;

}    // namespace bra::fs
