#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/**
 * @brief Create a directory given by @p path.
 *        It creates also the parent directory if necessary.
 *
 * @overload bool bra_fs_dir_make(const std::filesystem::path& path)
 *
 * @param path
 * @return true On success
 * @return false On error
 */
bool bra_fs_dir_make(const char* path);

#ifdef __cplusplus
}
#endif
