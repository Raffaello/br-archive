#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/**
 * @brief Create a directory given by @p path.
 *        It creates also the parent directory if necessary.
 *
 * @overload bool bra::fs::dir_make(const std::filesystem::path& path)
 *
 * @param path
 * @return true on success
 * @return false on error
 */
bool bra_fs_dir_make(const char* path);

#ifdef __cplusplus
}
#endif
