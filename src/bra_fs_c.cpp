#include "bra_fs_c.h"
#include "bra_fs.hpp"

namespace fs = std::filesystem;

extern "C" bool bra_fs_dir_exists(const char* path)
{
    if (path == nullptr)
        return false;

    const fs::path p(path);
    return bra::fs::dir_exists(p);
}

extern "C" bool bra_fs_dir_make(const char* path)
{
    if (path == nullptr)
        return false;

    const fs::path p(path);
    return bra::fs::dir_make(p);
}

extern "C" bool bra_fs_file_exists(const char* path)
{
    if (path == nullptr)
        return false;

    const fs::path p(path);
    return bra::fs::file_exists(p);
}

extern "C" bool bra_fs_file_attributes(const char* path, bra_attr_t* attr)
{
    if (attr == nullptr || path == nullptr)
        return false;

    const fs::path p(path);
    auto           a = bra::fs::file_attributes(p);
    if (!a)
        return false;

    *attr = *a;
    return true;
}

bool bra_fs_file_size(const char* path, uint64_t* file_size)
{
    if (file_size == nullptr || path == nullptr)
        return false;

    const fs::path p(path);
    auto           s = bra::fs::file_size(p);
    if (!s)
        return false;

    *file_size = *s;
    return true;
}
