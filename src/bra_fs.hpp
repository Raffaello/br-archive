#pragma once

#ifndef __cplusplus
#error "must be included in a cpp file unit"
#endif

#include <filesystem>
#include <optional>

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
std::optional<bool> bra_fs_file_exists_ask_overwrite(const std::filesystem::path& p);


bool bra_fs_isWildcard(const std::string& str);

/**
 * @brief
 *
 * @param wildcard
 * @return std::string
 */
std::string bra_fs_wildcard_to_regexp(const std::string& wildcard);

bool bra_fs_search(const std::filesystem::path& dir, const std::string& pattern);
