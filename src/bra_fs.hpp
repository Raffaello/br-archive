#pragma once

#ifndef __cplusplus
#error "must be included in a cpp file unit"
#endif

#include <filesystem>
#include <optional>
#include <string>
#include <generator>


/**
 * @brief Try to sanitize the @p path.
 *        It must be relative to the current directory.
 *        It can't escape the current directory.
 *        If it doesn't exists it will return false;
 *
 * @param path
 * @return true if it is successful
 * @return false otherwise
 */
bool bra_fs_try_sanitize(std::filesystem::path& path);

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
 * @brief
 *
 * @todo instead of bool return size_t: std::npos no wildcard, otherwise first wildcard char position.
 *
 * @param path
 * @return true
 * @return false
 */
[[nodiscard]] bool bra_fs_isWildcard(const std::filesystem::path& path);

/**
 * @brief Extract the directory from a wildcard if it contains any.
 *        The @p path_wildcard must have been sanitized with @ref bra_fs_try_sanitize,
 *        otherwise results are undefined.
 *
 * @param path_wildcard
 * @return std::filesystem::path
 */
[[nodiscard]] std::filesystem::path bra_fs_wildcard_extract_dir(std::filesystem::path& path_wildcard);

/**
 * @brief
 *
 * @param wildcard
 * @return std::string
 */
[[nodiscard]] std::string bra_fs_wildcard_to_regexp(const std::string& wildcard);

bool bra_fs_search(const std::filesystem::path& dir, const std::string& pattern);

std::generator<std::filesystem::path> bra_fs_co_search(const std::filesystem::path& dir, const std::string& pattern);
