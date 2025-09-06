#pragma once

#ifndef __cplusplus
#error "bra_wildcards.hpp must be included from a C++ translation unit. Use bra_fs.h when compiling as C."
#endif

#include <filesystem>
#include <string>
#include <set>

namespace bra::wildcards
{
/**
 * @brief Check if the given @p path contains a supported wildcard pattern.
 *
 * @todo instead of bool return size_t: std::npos no wildcard, otherwise first wildcard char position.
 *
 * @param path
 * @return true
 * @return false
 */
[[nodiscard]] bool is_wildcard(const std::filesystem::path& path);

/**
 * @brief Extract the directory from a wildcard if it contains any and modify accordingly the @p path_wildcard.
 *        If there is no wildcard, @p path_wildcard will result to be empty.
 *        The @p path_wildcard must have been sanitized with @ref try_sanitize before,
 *        otherwise results are undefined.
 *
 * @note this function doesn't fail if @p path_wildcard doesn't contain any wildcards,
 *       it will just modify to be an empty string.
 *
 * @todo it might be private only.
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

}    // namespace bra::wildcards
