#pragma once

#ifndef __cplusplus
#error "bra_wildcards.hpp must be included from a C++ translation unit."
#endif

#if __cplusplus < 201703L
#error "requires C++17 or later (std::filesystem)."
#endif


#include <filesystem>
#include <string_view>
#include <string>

namespace bra::wildcards
{
/**
 * @brief Check if the given @p path contains a supported wildcard pattern.
 *
 * @todo instead of bool return size_t std::npos no wildcard, otherwise first wildcard char position.
 *
 * @param path
 * @retval true
 * @retval false
 */
[[nodiscard]] bool is_wildcard(const std::filesystem::path& path) noexcept;

/**
 * @brief Extract the directory from a wildcard if it contains any and modify accordingly the @p path_wildcard.
 *        If there is no wildcard, @p path_wildcard will result to be empty.
 *        The @p path_wildcard must have been sanitized with @ref bra::fs::try_sanitize before,
 *        otherwise results are undefined.
 *
 * @note this function doesn't fail if @p path_wildcard doesn't contain any wildcards,
 *       it will just modify to be empty.
 *
 * @todo it might be private only.
 *
 * @param path_wildcard this is changed stripping the dir. If it wasn't a wildcard it will be empty
 * @return std::filesystem::path
 */
[[nodiscard]] std::filesystem::path wildcard_extract_dir(std::filesystem::path& path_wildcard) noexcept;

/**
 * @brief Convert the wildcard to a regular expression for internal use.
 *
 * @todo this should most likely be private. Paired with @ref wildcard_extract_dir
 *
 * @param wildcard
 * @return std::string
 */
[[nodiscard]] std::string wildcard_to_regexp(std::string_view wildcard) noexcept;

}    // namespace bra::wildcards
