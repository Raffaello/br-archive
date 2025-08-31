#pragma once

#ifndef __cplusplus
#error "must be included in a cpp file unit (use bra_fs.h instead)"
#endif

#include <filesystem>
#include <optional>
#include <string>
// #include <generator> // C++23 not supported in Ubuntu24 (due to older GCC version 13)
#include <list>

/**
 * @brief Try to sanitize the @p path.
 *        It must be relative to the current directory.
 *        It can't escape the current directory.
 *        If it doesn't exist it will return false;
 *
 * @param path
 * @return true if it is successful
 * @return false otherwise
 */
bool bra_fs_try_sanitize(std::filesystem::path& path);

/**
 * @brief Create a directory given by @p path.
 *        It creates also the parent directory if necessary.
 *
 * @param path
 * @return true On success
 * @return false On error
 */
bool bra_fs_dir_make(const std::filesystem::path& path);

/**
 * @brief
 *
 * @param path
 * @return std::filesystem::path
 */
[[nodiscard]] std::filesystem::path bra_fs_filename_archive_adjust(const std::filesystem::path& path);

/**
 * @brief Return the given filename ending always with the correct extension.
 *        if @p tmp is true the extension is #BRA_FILE_EXT #BRA_SFX_TMP_FILE_EXT
 *        otherwise                       is #BRA_FILE_EXT #BRA_SFX_FILE_EXT
 *
 * @param path
 * @param tmp
 * @return std::filesystem::path the adjusted path.
 */
[[nodiscard]] std::filesystem::path bra_fs_filename_sfx_adjust(const std::filesystem::path& path, const bool tmp);

/**
 * @brief Check if a regular file exists.
 *
 * @param p
 * @return true
 * @return false
 */
[[nodiscard]] bool bra_fs_file_exists(const std::filesystem::path& p);

/**
 * @brief Check if the file exists and ask the user to overwrite.
 *
 * @note The file is considered a 'regular_file' it won't check if it is a directory.
 *
 * @param p
 * @param always_yes if true, assumes yes without asking.
 * @return std::optional<bool> when has no value the file doesn't exist.
 * @return true overwrite
 * @return false don't overwrite
 */
[[nodiscard]] std::optional<bool> bra_fs_file_exists_ask_overwrite(const std::filesystem::path& p, const bool always_yes);

/**
 * @brief Check if the given @p path contains a wildcard supported pattern.
 *
 * @todo instead of bool return size_t: std::npos no wildcard, otherwise first wildcard char position.
 *
 * @param path
 * @return true
 * @return false
 */
[[nodiscard]] bool bra_fs_isWildcard(const std::filesystem::path& path);

/**
 * @brief Extract the directory from a wildcard if it contains any and modify accordingly the @p path_wildcard.
 *        If there is no wildcard, @p path_wildcard will result to be empty.
 *        The @p path_wildcard must have been sanitized with @ref bra_fs_try_sanitize,
 *        otherwise results are undefined.
 *
 * @note this function doesn't fail if @p path_wildcard doesn't contain any wildcards,
 *       it will just modify to be an empty string.
 *
 * @param path_wildcard this is changed stripping the dir. If it wasn't a wildcard it will be empty
 * @return std::filesystem::path
 */
[[nodiscard]] std::filesystem::path bra_fs_wildcard_extract_dir(std::filesystem::path& path_wildcard);

/**
 * @brief Convert the wildcard to a regular expression for internal use.
 *
 * @todo this should most likely be private. Paired with @ref bra_fs_wildcard_extract_dir
 *
 * @param wildcard
 * @return std::string
 */
[[nodiscard]] std::string bra_fs_wildcard_to_regexp(const std::string& wildcard);

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
bool bra_fs_search(const std::filesystem::path& dir, const std::string& pattern, std::list<std::filesystem::path>& out_files);

/**
 * @brief C++23 generator not supported yet in Ubuntu 24 ... pff!...
 *
 */
// std::generator<std::filesystem::path> bra_fs_co_search(const std::filesystem::path& dir, const std::string& pattern);
