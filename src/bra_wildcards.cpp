#include "bra_wildcards.hpp"
#include <bra_log.h>

#include <list>

namespace fs = std::filesystem;

using namespace std;

/////////////////////////////////////////////////////////////////////////////////////////////////

namespace bra::wildcards
{

bool is_wildcard([[maybe_unused]] const std::filesystem::path& path)
{
    if (path.empty())
        return false;

    return path.string().find_first_of("?*") != string::npos;
}

std::filesystem::path wildcard_extract_dir(std::filesystem::path& path_wildcard)
{
    string       dir;
    string       wildcard = path_wildcard.generic_string();
    const size_t pos      = wildcard.find_first_of("?*");
    const size_t dir_pos  = wildcard.find_last_of('/', pos);

    if (dir_pos != string::npos)
        dir = wildcard.substr(0, dir_pos + 1);    // including '/'
    else
        dir = "./";                               // the wildcard is before the directory separator or there is no directory

    switch (pos)
    {
    // case 0:
    // return dir;
    case string::npos:
        bra_log_debug("No wildcard found in here: %s", wildcard.c_str());
        wildcard.clear();
        break;
    }

    if (!wildcard.empty() && dir_pos < pos)
        wildcard = wildcard.substr(dir_pos + 1, wildcard.size());

    path_wildcard = wildcard;
    return fs::path(dir);
}

std::string wildcard_to_regexp(const std::string& wildcard)
{
    std::string regex;
    // regex.reserve(wildcard.size() * 2);

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
        case '+':
        case '\\':
            regex += '\\';    // Escape special regex characters
            [[fallthrough]];
        default:
            regex += c;
        }
    }

    return regex;
}

// bool wildcard_expand([[maybe_unused]] const std::filesystem::path& wildcard_path, [[maybe_unused]] std::set<std::filesystem::path>& out_files)
// {
//     fs::path p = wildcard_path.generic_string();

// if (!is_wildcard(p))
//     return false;

// if (!try_sanitize(p))
//     return false;

// const fs::path dir     = wildcard_extract_dir(p);
// const string   pattern = wildcard_to_regexp(p.string());

// std::list<fs::path> files;
// if (!bra::fs::search(dir, pattern, files))
// {
//     bra_log_error("search failed in %s for wildcard %s", dir.string().c_str(), p.string().c_str());
//     return false;
// }

// while (!files.empty())
// {
//     const auto& f = files.front();
//     if (!out_files.insert(f).second)
//         bra_log_warn("duplicate file given in input: %s", f.string().c_str());

// files.pop_front();
// }
// return true;
// }

}    // namespace bra::wildcards
