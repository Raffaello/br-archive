#include "bra_fs.hpp"

#include <iostream>
#include <format>
#include <string>
#include <cctype>
#include <regex>

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

std::string bra_fs_wildcard_to_regexp(const std::string& wildcard)
{
    std::string regex;

    for (char c : wildcard)
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
    // Define the regex pattern (e.g., files ending with ".txt")
    std::regex r(pattern);

    try
    {
        // Iterate through the directory
        for (const auto& entry : fs::directory_iterator(dir))
        {
            if (entry.is_regular_file())
            {    // Check if it's a file
                const std::string filename = entry.path().filename().string();

                // Match the filename with the regex pattern
                if (std::regex_match(filename, r))
                {
                    std::cout << "Matched file: " << filename << endl;
                }
            }
        }
    }
    catch (const fs::filesystem_error& e)
    {
        cerr << "Filesystem error: " << e.what() << endl;
    }
    catch (const std::regex_error& e)
    {
        cerr << "Regex error: " << e.what() << endl;
    }
}
