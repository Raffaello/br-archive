#include "bra_fs.hpp"

#include <iostream>
#include <format>
#include <string>
#include <cctype>

namespace fs = std::filesystem;

using namespace std;

std::optional<bool> bra_fs_file_exists_ask_overwrite(const std::filesystem::path& p)
{
    if (fs::exists(p) && fs::is_regular_file(p))
    {
        char c;

        cout << format("File {} already exists. Overwrite? [Y/N] ", p.string());
        do
        {
            cin >> c;
            c = tolower(c);
        }
        while (c != 'y' && c != 'n');

        cout << endl;
        return c == 'y';
    }

    return nullopt;
}
