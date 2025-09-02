#include "bra_fs_c.h"
#include "bra_fs.hpp"

namespace fs = std::filesystem;

extern "C" bool bra_fs_dir_make(const char* path)
{
    const fs::path p(path);

    return bra::fs::dir_make(p);
}
