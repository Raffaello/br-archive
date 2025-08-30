#include "bra_fs.hpp"
#include "lib_bra_defs.h"

#include <iostream>
#include <format>
#include <string>
#include <cctype>
#include <regex>
#include <algorithm>

namespace fs = std::filesystem;

using namespace std;

std::filesystem::path bra_fs_filename_sfx_adjust(const std::filesystem::path& out_file, const bool tmp)
{
    fs::path p;
    fs::path sfx_ext = tmp ? BRA_SFX_TMP_FILE_EXT : BRA_SFX_FILE_EXT;

    if (out_file.extension() == BRA_SFX_FILE_EXT)
        p = out_file.stem();
    else
        p = out_file;


    if (p.extension() == BRA_FILE_EXT)
        p += sfx_ext;

    else
        p += string(BRA_FILE_EXT) + sfx_ext.string();

    return p;
}

bool bra_fs_file_exists(const std::filesystem::path& p)
{
    return fs::exists(p) && fs::is_regular_file(p);
}

std::optional<bool> bra_fs_file_exists_ask_overwrite(const std::filesystem::path& p, const bool always_yes)
{
    if (!bra_fs_file_exists(p))
        return nullopt;

    char c;

    cout << format("File {} already exists. Overwrite? [Y/N] ", p.string()) << flush;
    if (always_yes)
    {
        cout << 'y' << endl;
        return true;
    }

    do
    {
        cin >> c;
        if (cin.fail())
        {
            cin.clear();
            c = 'n';
        }
        else
            c = static_cast<char>(tolower(c));
    }
    while (c != 'y' && c != 'n');

    cout << endl;
    return c == 'y';
}

bool bra_fs_isWildcard(const std::string& str)
{
    return str.find_first_of("?*") != string::npos;
}

std::filesystem::path bra_fs_wildcard_extract_dir(std::string& wildcard)
{
    replace(wildcard.begin(), wildcard.end(), '\\', '/');

    string       dir;
    const size_t pos     = wildcard.find_first_of("?*");
    const size_t dir_pos = wildcard.find_last_of('/', pos);

    if (dir_pos != string::npos)
        dir = wildcard.substr(0, dir_pos + 1);    // including '/'
    else
        dir = "./";

    switch (pos)
    {
    // case 0:
    // return dir;
    case string::npos:
        cout << format("[DEBUG] No wildcard found in here: {}", wildcard) << endl;
        wildcard.clear();
        break;
    }

    if (!wildcard.empty())
        wildcard = wildcard.substr(pos, wildcard.size());

    return fs::path(dir);
}

std::string bra_fs_wildcard_to_regexp(const std::string& wildcard)
{
    std::string regex;

    for (const char& c : wildcard)
    {
        switch (c)
        {
        case '*':
            regex += ".*";
            break;    // '*' -> '.*'
        case '?':
            regex += '.';
            break;    // '?' -> '.'
        case '.':
        case '^':
        case '$':
        case '(':
        case ')':
        case '[':
        case ']':
        case '{':
        case '}':
        case '|':
        case '\\':
            regex += '\\';    // Escape special regex characters
            [[fallthrough]];
        default:
            regex += c;
        }
    }

    return regex;
}

bool bra_fs_search(const std::filesystem::path& dir, const std::string& pattern)
{
    std::regex r(pattern);

    try
    {
        for (const auto& entry : fs::directory_iterator(dir))
        {
            if (entry.is_regular_file())
            {
                const std::string filename = entry.path().filename().string();
                if (std::regex_match(filename, r))
                {
                    std::cout << "Matched file: " << filename << endl;
                }
            }
        }
    }
    catch (const fs::filesystem_error& e)
    {
        cerr << "ERROR: Filesystem error: " << e.what() << endl;
        return false;
    }
    catch (const std::regex_error& e)
    {
        cerr << "ERROR: Regex error: " << e.what() << endl;
        return false;
    }

    return true;
}
