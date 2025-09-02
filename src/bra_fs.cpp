#include "bra_fs.hpp"
#include "lib_bra_defs.h"

#include <iostream>
#include <format>
#include <string>
#include <cctype>
#include <regex>
#include <algorithm>

// #include <coroutine>

namespace bra::fs
{

namespace fs = std::filesystem;

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////


bool try_sanitize(std::filesystem::path& path)
{
    error_code ec;

    path = fs::relative(path, fs::current_path(), ec);
    if (ec)
        return false;

    if (path.is_absolute() || path.has_root_name())
        return false;

    for (const auto& p : path)
    {
        if (p == "..")
            return false;
    }

    path = path.lexically_normal().generic_string();

    return !path.empty();
}

bool isWildcard(const std::filesystem::path& path)
{
    if (path.empty())
        return false;

    return path.string().find_first_of("?*") != string::npos;
}

bool dir_exists(const std::filesystem::path& path)
{
    error_code ec;
    const bool isDir = fs::is_directory(path, ec);

    // TODO: can't know if it was an error, maybe use an optional instead?
    if (ec)
        return false;

    return isDir;
}

bool dir_make(const std::filesystem::path& path)
{
    error_code ec;

    if (dir_exists(path))
        return true;

    const bool created = fs::create_directories(path, ec);
    if (ec)
    {
        cerr << format("ERROR: can't mkdir {}: {}", path.string(), ec.message()) << endl;
        return false;
    }

    // TOCTOU-Safe: Handle race condition: directory may have been created by another process.
    //              Commented-out as it is pointless for this application.
    // return created || dir_exists(path);
    return created;
}

std::filesystem::path filename_archive_adjust(const std::filesystem::path& path)
{
    fs::path p = path;

    if (p.extension() != BRA_FILE_EXT)
        p += BRA_FILE_EXT;

    return p;
}

std::filesystem::path filename_sfx_adjust(const std::filesystem::path& path, const bool tmp)
{
    fs::path       p;
    const fs::path sfx_ext = tmp ? BRA_SFX_TMP_FILE_EXT : BRA_SFX_FILE_EXT;

    if (path.extension() == BRA_SFX_FILE_EXT)
        p = path.stem();
    else
        p = path;


    if (p.extension() == BRA_FILE_EXT)
        p += sfx_ext;
    else
        p += string(BRA_FILE_EXT) + sfx_ext.string();

    return p;
}

bool file_exists(const std::filesystem::path& path)
{
    error_code ec;
    const bool isRegFile = fs::is_regular_file(path, ec);

    if (ec)
        return false;

    return isRegFile;
}

std::optional<bool> file_exists_ask_overwrite(const std::filesystem::path& path, const bool always_yes)
{
    if (!file_exists(path))
        return nullopt;

    char c;

    cout << format("File {} already exists. Overwrite? [Y/N] ", path.string()) << flush;
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
            c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    }
    while (c != 'y' && c != 'n');

    cout << endl;
    return c == 'y';
}

std::optional<bra_attr_t> file_attributes(const std::filesystem::path& path)
{
    std::error_code ec;
    auto            err = [&path, &ec]() {
        cerr << format("ERROR: unable to read file attributes of {}: {} ", path.string(), ec.message()) << endl;
        return nullopt;
    };

    if (fs::is_regular_file(path, ec))
        return BRA_ATTR_FILE;
    else if (ec)
        return err();
    else if (fs::is_directory(path, ec))
        return BRA_ATTR_DIR;

    return err();
}

std::optional<uint64_t> file_size(const std::filesystem::path& path)
{
    std::error_code ec;
    auto            err = [&path, &ec]() {
        cerr << format("ERROR: unable to read file size of {}: {} ", path.string(), ec.message()) << endl;
        return nullopt;
    };

    if (fs::is_directory(path, ec))
        return 0;
    else if (ec)
        return err();
    else if (fs::is_regular_file(path, ec))
    {
        const auto size = fs::file_size(path, ec);
        if (ec)
            return err();

        return size;
    }

    return err();
}

std::filesystem::path wildcard_extract_dir(std::filesystem::path& path_wildcard)
{
    string       dir;
    string       wildcard = path_wildcard.string();
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
        cout << format("[DEBUG] No wildcard found in here: {}", wildcard) << endl;
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

bool wildcard_expand(const std::filesystem::path& wildcard_path, std::set<std::filesystem::path>& out_files)
{
    fs::path       p       = wildcard_path.generic_string();
    const fs::path dir     = bra::fs::wildcard_extract_dir(p);
    const string   pattern = bra::fs::wildcard_to_regexp(p.string());

    std::list<fs::path> files;
    if (!bra::fs::search(dir, pattern, files))
    {
        cerr << format("ERROR: not a valid wildcard: {}", p.string()) << endl;
        return false;
    }

    for (const auto& p : files)
    {
        if (!out_files.insert(p).second)
            cout << format("WARNING: duplicate file given in input: {}", p.string()) << endl;
    }

    files.clear();
    return true;
}

bool search(const std::filesystem::path& dir, const std::string& pattern, std::list<std::filesystem::path>& out_files)
{
    const std::regex r(pattern);
    bool             res = true;

    try
    {
        for (const auto& entry : fs::directory_iterator(dir))
        {
            // TODO: dir to search only if it is recursive (-r)

            // const bool is_dir = entry.is_directory();
            const bool is_dir = false;    // TODO: must be done later, requires to struct into dirs the archive too first.
            if (!(entry.is_regular_file() || is_dir))
                continue;

            fs::path          ep       = entry.path();
            const std::string filename = ep.filename().string();
            if (!std::regex_match(filename, r))
                continue;

            // if (is_dir)
            // {
            //     std::cout << "Matched dir: " << filename << endl;
            //     const std::string p  = pattern.substr(ep.string().size());
            //     res                 &= search(ep, p);
            // }
            // else
            // std::cout << "[DEBUG] Expected file: " << filename << endl;

            if (!try_sanitize(ep))
            {
                cerr << format("[ERROR] not a valid file: {}", ep.string()) << endl;
                return false;
            }

            out_files.push_back(ep);
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

    return res;
}

/*/
std::generator<std::filesystem::path> bra_fs_co_search(const std::filesystem::path& dir, [[maybe_unused]] const std::string& pattern)
{
    const std::regex r(pattern);

    try
    {
        for (const auto& entry : fs::directory_iterator(dir))
        {
            // const bool is_dir = entry.is_directory();
            const bool is_dir = false;    // TODO: must be done later, requires to struct into dir the archive too first.
            if (!(entry.is_regular_file() || is_dir))
                continue;

            // const fs::path    ep       = entry.path();
            // const std::string filename = ep.filename().string();
            if (!std::regex_match(entry.path().filename().string(), r))
                continue;

            // if (is_dir)
            // {
            //     std::cout << "Matched dir: " << filename << endl;
            //     const std::string p  = pattern.substr(ep.string().size());
            //     res                 &= search(ep, p);
            // }
            //  else
            // std::cout << "Matched file: " << filename << endl;
            co_yield entry.path();
        }
    }
    catch (const fs::filesystem_error& e)
    {
        cerr << "ERROR: Filesystem error: " << e.what() << endl;
        co_return;
    }
    catch (const std::regex_error& e)
    {
        cerr << "ERROR: Regex error: " << e.what() << endl;
        co_return;
    }
}
*/

}    // namespace bra::fs
