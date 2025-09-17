#include <fs/bra_fs_c.h>
#include <fs/bra_fs.hpp>

namespace fs = std::filesystem;

bool bra_fs_dir_exists(const char* path)
{
    if (path == nullptr)
        return false;

    const fs::path p(path);
    return bra::fs::dir_exists(p);
}

bool bra_fs_dir_make(const char* path)
{
    if (path == nullptr)
        return false;

    const fs::path p(path);
    return bra::fs::dir_make(p);
}

bool bra_fs_dir_is_sub_dir(const char* src, const char* dst)
{
    if (src == nullptr || dst == nullptr)
        return false;

    return bra::fs::dir_isSubDir(src, dst);
}

bool bra_fs_file_exists(const char* path)
{
    if (path == nullptr)
        return false;

    const fs::path p(path);
    return bra::fs::file_exists(p);
}

bool bra_fs_file_attributes(const char* path, const char* base, bra_attr_t* attr)
{
    if (attr == nullptr || path == nullptr || base == nullptr)
        return false;

    const fs::path p(path);
    const fs::path b(base);
    const auto     a = bra::fs::file_attributes(p, b);
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
    const auto     s = bra::fs::file_size(p);
    if (!s)
        return false;

    *file_size = *s;
    return true;
}

bool bra_fs_file_exists_ask_overwrite(const char* path, bra_fs_overwrite_policy_e* overwrite_policy, const bool single_overwrite)
{
    if (path == nullptr || overwrite_policy == nullptr)
        return false;

    const fs::path p(path);
    const auto     res = bra::fs::file_exists_ask_overwrite(p, *overwrite_policy, single_overwrite);
    if (!res)
        return true;    // doesn't exists, equivalent to overwrite

    return *res;
}
