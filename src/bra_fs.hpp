#pragma once

#ifndef __cplusplus
#error "must be included in a cpp file unit"
#endif

#include <filesystem>
#include <optional>

/**
 * @brief
 *
 * @param p
 * @return std::optional<bool> when has no value the file doesn't exist.
 * @return true overwrite
 * @return false don't overwrite
 */
std::optional<bool> bra_fs_file_exists_ask_overwrite(const std::filesystem::path& p);
