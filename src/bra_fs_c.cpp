#include "bra_fs.h"
#include "bra_fs.hpp"

namespace fs = std::filesystem;

bool bra_fs_dir_make(const char* path)
{
    const fs::path p(path);

    return bra_fs_dir_make(p);
}
