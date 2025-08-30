#include "bra_fs.hpp"

#include <iostream>
#include <format>
#include <string>
#include <cctype>
#include <regex>
#include <algorithm>

namespace fs = std::filesystem;

using namespace std;

bool bra_fs_file_exists(const std::filesystem::path& p)
{
    return fs::exists(p) && fs::is_regular_file(p);
}

std::optional<bool> bra_fs_file_exists_ask_overwrite(const std::filesystem::path& p)
{
    if (!bra_fs_file_exists(p))
        return nullopt;

    char c;

    cout << format("File {} already exists. Overwrite? [Y/N] ", p.string()) << flush;

    do
    {
        cin >> c;
        c = static_cast<char>(tolower(c));
    }
    while (c != 'y' && c != 'n');

    cout << endl;
    return c == 'y';
}

bool bra_fs_isWildcard(const std::string& str)
{
    for (const char& c : str)
    {
        if (c == '*' || c == '?')
            return true;
    }

    return false;
}

std::filesystem::path bra_fs_wildcard_extract_dir(std::string& wildcard)
{
    size_t pos = std::min(wildcard.find_first_of('?'), wildcard.find_first_of('*'));
    if (pos == 0 || pos == string::npos)
        return "./";

    // instead if there is an explicit directory
    string dir = wildcard.substr(0, pos);

    wildcard = wildcard.substr(dir.length(), wildcard.size());
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
