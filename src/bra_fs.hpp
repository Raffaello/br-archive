#pragma once

#ifndef __cplusplus
#error "must be included in a cpp file unit"
#endif

#include <filesystem>
#include <optional>
#include <string>
#include <functional>


bool bra_fs_file_exists(const std::filesystem::path& p);

/**
 * @brief Check if the file exists and ask the user to overwrite.
 *
 * @note The file is considered a 'regular_file' it won't check if it is a directory.
 *
 * @param p
 * @return std::optional<bool> when has no value the file doesn't exist.
 * @return true overwrite
 * @return false don't overwrite
 */
[[nodiscard]] std::optional<bool> bra_fs_file_exists_ask_overwrite(const std::filesystem::path& p);

/**
 * @brief
 *
 * @todo instead of bool return size_t: std::npos no wildcard, otherwise first wildcard char position.
 *
 * @param str
 * @return true
 * @return false
 */
bool bra_fs_isWildcard(const std::string& str);

std::filesystem::path bra_fs_wildcard_extract_dir(std::string& wildcard);

/**
 * @brief
 *
 * @param wildcard
 * @return std::string
 */
std::string bra_fs_wildcard_to_regexp(const std::string& wildcard);

bool bra_fs_search(const std::filesystem::path& dir, const std::string& pattern);
