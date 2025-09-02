#pragma once

#ifndef __cplusplus
#error "bra_fs.hpp must be included from a C++ translation unit. Use bra_fs.h when compiling as C."
#endif

#include "lib_bra_types.h"


#include <filesystem>
#include <optional>
#include <string>
#include <list>
#include <set>

// #include <generator> // C++23 not supported in Ubuntu24 (due to older GCC version 13)

namespace bra::fs
{

/**
 * @brief Try to sanitize the @p path.
 *        It must be relative to the current directory.
 *        It can't escape the current directory.
 *        Existence is not checked; it only rewrites to a safe, relative path.
 *
 * @param path
 * @return true if it is successful
 * @return false otherwise
 */
[[nodiscard]] bool try_sanitize(std::filesystem::path& path);

/**
 * @brief Check if the given @p path contains a wildcard supported pattern.
 *
 * @todo instead of bool return size_t: std::npos no wildcard, otherwise first wildcard char position.
 *
 * @param path
 * @return true
 * @return false
 */
[[nodiscard]] bool isWildcard(const std::filesystem::path& path);

/**
 * @brief Check if the given @p path exists and is a directory.
 *
 * @see file_exists
 *
 * @param path
 * @return true if the path exists and is a directory
 * @return false otherwise
 */
[[nodiscard]] bool dir_exists(const std::filesystem::path& path);

/**
 * @brief Create the directory at @p path.
 *        Also creates parent directories as needed.
 *
 * @note Idempotent: returns true if the directory already exists.
 *
 * @see dir_exists
 *
 * @param path
 * @return true if the directory was created or already existed.
 * @return false on error
 */
[[nodiscard]] bool dir_make(const std::filesystem::path& path);

/**
 * @brief
 *
 * @param path
 * @return std::filesystem::path
 */
[[nodiscard]] std::filesystem::path filename_archive_adjust(const std::filesystem::path& path);

/**
 * @brief Return the given filename ending always with the correct extension.
 *        if @p tmp is true the extension is #BRA_FILE_EXT #BRA_SFX_TMP_FILE_EXT
 *        otherwise                       is #BRA_FILE_EXT #BRA_SFX_FILE_EXT
 *
 * @param path
 * @param tmp
 * @return std::filesystem::path the adjusted path.
 */
[[nodiscard]] std::filesystem::path filename_sfx_adjust(const std::filesystem::path& path, const bool tmp);

/**
 * @brief Check if the given @p path is a regular file and exists.
 *
 * @see dir_exists
 *
 * @param path
 * @return true
 * @return false
 */
[[nodiscard]] bool file_exists(const std::filesystem::path& path);

/**
 * @brief Check if the file in @p path exists and ask the user to overwrite.
 *
 * @note The file is considered a 'regular_file' it won't check if it is a directory.
 *
 * @param path
 * @param always_yes if true, assumes yes without asking.
 * @return std::optional<bool> when has no value the file doesn't exist.
 * @return true overwrite
 * @return false don't overwrite
 */
[[nodiscard]] std::optional<bool> file_exists_ask_overwrite(const std::filesystem::path& path, const bool always_yes);

/**
 * @brief Get the file attributes for the given @p path.
 *
 * @param path
 * @return std::optional<bra_attr_t> #BRA_ATTR_FILE for regular files, #BRA_ATTR_DIR for directories, @c nullopt for errors or unknown types.
 */
[[nodiscard]] std::optional<bra_attr_t> file_attributes(const std::filesystem::path& path);

/**
 * @brief Get the size of a file or directory.
 *
 * @param path
 * @return std::optional<uint64_t> File size in bytes for regular files, 0 for directories, @c nullopt on error
 */
[[nodiscard]] std::optional<uint64_t> file_size(const std::filesystem::path& path);

/**
 * @brief Extract the directory from a wildcard if it contains any and modify accordingly the @p path_wildcard.
 *        If there is no wildcard, @p path_wildcard will result to be empty.
 *        The @p path_wildcard must have been sanitized with @ref try_sanitize before,
 *        otherwise results are undefined.
 *
 * @note this function doesn't fail if @p path_wildcard doesn't contain any wildcards,
 *       it will just modify to be an empty string.
 *
 * @param path_wildcard this is changed stripping the dir. If it wasn't a wildcard it will be empty
 * @return std::filesystem::path
 */
[[nodiscard]] std::filesystem::path wildcard_extract_dir(std::filesystem::path& path_wildcard);

/**
 * @brief Convert the wildcard to a regular expression for internal use.
 *
 * @todo this should most likely be private. Paired with @ref wildcard_extract_dir
 *
 * @param wildcard
 * @return std::string
 */
[[nodiscard]] std::string wildcard_to_regexp(const std::string& wildcard);

/**
 * @brief Expand the given @p wildcard_path and store the resulting paths into @p out_files.
 *        It doesn't clear @p out_files, but it adds on it.
 *
 * @param wildcard_path
 * @param out_files
 * @return true
 * @return false
 */
[[nodiscard]] bool wildcard_expand(const std::filesystem::path& wildcard_path, std::set<std::filesystem::path>& out_files);

/**
 * @brief Search all the files in the given @p dir matching the regular expression @p pattern.
 *        It stores the results in @p out_files.
 *
 * @param dir
 * @param pattern
 * @param out_files
 * @return true if successful
 * @return false otherwise
 */
[[nodiscard]] bool search(const std::filesystem::path& dir, const std::string& pattern, std::list<std::filesystem::path>& out_files);

/**
 * @brief C++23 generator not supported yet in Ubuntu 24 ... pff!...
 *
 */
// std::generator<std::filesystem::path> bra_fs_co_search(const std::filesystem::path& dir, const std::string& pattern);

}    // namespace bra::fs
