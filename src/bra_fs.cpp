#include "bra_fs.hpp"
#include "lib_bra_defs.h"
#include "bra_log.h"

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
    auto       err = [&path, &ec]() {
        bra_log_error("unable to sanitize path %s: %s", path.string().c_str(), ec.message().c_str());
        return false;
    };

    const bool rel = fs::absolute(path, ec).string().starts_with(fs::current_path().string());
    if (ec)
        return err();
    if (!rel)
        return false;

    fs::path p = fs::relative(path, fs::current_path(), ec);
    if (ec)
        return err();

    if (p.empty())
    {
        if (path.is_absolute() || path.has_root_name())
            // in this case it should have ec.clear()
            return err();

        // try adding current directory as it might be a wildcard...
        path = "./" / path;
        path = fs::relative(path, fs::current_path(), ec);
        if (ec)
            return false;
    }
    else
        path = p;

    for (const auto& p : path)
    {
        if (p == "..")
            return false;
    }

    path = path.lexically_normal().generic_string();
    return !path.empty();
}

bool is_wildcard(const std::filesystem::path& path)
{
    if (path.empty())
        return false;

    return path.string().find_first_of("?*") != string::npos;
}

bool dir_exists(const std::filesystem::path& path)
{
    error_code ec;
    const auto err = [&path, &ec]() {
        bra_log_error("can't check dir %s: %s", path.string().c_str(), ec.message().c_str());
        return false;
    };

    const bool exists = fs::exists(path, ec);
    if (ec)
        return err();

    if (!exists)
        return false;

    const bool isDir = fs::is_directory(path, ec);
    if (ec)
        return err();

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
        bra_log_error("can't mkdir %s: %s", path.string().c_str(), ec.message().c_str());
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
    const auto err = [&path, &ec]() {
        bra_log_error("can't check file %s: %s", path.string().c_str(), ec.message().c_str());
        return false;
    };

    const bool exists = fs::exists(path, ec);
    if (ec)
        return err();

    if (!exists)
        return false;

    const bool isRegFile = fs::is_regular_file(path, ec);
    if (ec)
        return err();

    return isRegFile;
}

std::optional<bool> file_exists_ask_overwrite(const std::filesystem::path& path, bra_fs_overwrite_policy_e& overwrite_policy, const bool single_overwrite)
{
    if (!file_exists(path))
        return nullopt;

    char c;

    // TODO: this should use bra_message, bra_message must be moved into bra_log.h
    if (single_overwrite)
        cout << format("File {} already exists. Overwrite? ([y]es/[n]o) ", path.string()) << flush;
    else
        cout << format("File {} already exists. Overwrite? ([y]es/[n]o/[A]ll/N[o]ne) ", path.string()) << flush;

    switch (overwrite_policy)
    {
    case BRA_OVERWRITE_ALWAYS_YES:
        cout << 'y' << endl;
        return true;
    case BRA_OVERWRITE_ALWAYS_NO:
        cout << 'n' << endl;
        return false;
    default:
        break;
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

        if (!single_overwrite)
        {
            if (c == 'a' && c == 'o')
                break;
        }
    }
    while (c != 'y' && c != 'n');
    cout << endl;

    if (!single_overwrite)
    {
        if (c == 'a')
        {
            overwrite_policy = BRA_OVERWRITE_ALWAYS_YES;
            c                = 'y';
        }
        else if (c == 'o')
        {
            overwrite_policy = BRA_OVERWRITE_ALWAYS_NO;
            c                = 'n';
        }
    }

    return c == 'y';
}

std::optional<bra_attr_t> file_attributes(const std::filesystem::path& path)
{
    std::error_code ec;
    auto            err = [&path, &ec]() {
        bra_log_error("unable to read file attributes of %s: %s ", path.string().c_str(), ec.message().c_str());
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
        bra_log_error("unable to read file size of %s: %s", path.string().c_str(), ec.message().c_str());
        return nullopt;
    };

    if (fs::is_directory(path, ec))
        return static_cast<uint64_t>(0);
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

bool file_set_add_dir(std::set<std::filesystem::path>& files)
{
    std::list<fs::path> listFiles(files.begin(), files.end());
    files.clear();
    while (!listFiles.empty())
    {
        // NOTE: as it is a set i can just insert multiple time the directory
        //       and when iterate later it, resolve it resolve on it.
        //       need to keep track of the last directory entry to remove from the file.

        auto f = listFiles.front();
        listFiles.pop_front();

        if (dir_exists(f))
        {
            // TODO: only if recursive is not enabled
            //       recursive will also store empty directories.
            bra_log_debug("ignoring directory (non-recursive mode): %s", f.string().c_str());
        }
        else if (!file_exists(f))
        {
            bra_log_error("%s is neither a regular file nor a directory", f.string().c_str());
            continue;
        }

        fs::path f_ = f;
        if (!bra::fs::try_sanitize(f_))
        {
            bra_log_error("path not valid: %s - (%s)", f.string().c_str(), f_.string().c_str());
            return false;
        }

        fs::path p_;
        p_.clear();
        for (const auto& p : f_)
        {
            p_ /= p;
            files.insert(p_);
        }
        if (p_ != f_)
        {
            bra_log_error("expected %s == %s", p_.string().c_str(), f_.string().c_str());
            return false;
        }
    }

    if (files.size() > numeric_limits<uint32_t>::max())
    {
        bra_log_error("Too many files, not supported yet: %zu/%u", files.size(), numeric_limits<uint32_t>::max());
        return false;
    }

    return true;
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

bool wildcard_expand(const std::filesystem::path& wildcard_path, std::set<std::filesystem::path>& out_files)
{
    fs::path p = wildcard_path.generic_string();

    if (!is_wildcard(p))
        return false;

    if (!try_sanitize(p))
        return false;

    const fs::path dir     = bra::fs::wildcard_extract_dir(p);
    const string   pattern = bra::fs::wildcard_to_regexp(p.string());

    std::list<fs::path> files;
    if (!search(dir, pattern, files))
    {
        bra_log_error("search failed in %s for wildcard %s", dir.string().c_str(), p.string().c_str());
        return false;
    }

    while (!files.empty())
    {
        const auto& f = files.front();
        if (!out_files.insert(f).second)
            bra_log_warn("duplicate file given in input: %s", f.string().c_str());

        files.pop_front();
    }

    return true;
}

bool search(const std::filesystem::path& dir, const std::string& pattern, std::list<std::filesystem::path>& out_files)
{
    try
    {
        const std::regex r(pattern);

        // TODO: add a cli flag for fs::directory_options::skip_permission_denied
        // TODO: add a cli flag for fs::directory_options::follow_directory_symlink
        for (const auto& entry : fs::directory_iterator(dir))
        {
            // TODO: dir to search only if it is recursive (-r)
            const bool is_dir = entry.is_directory();
            if (!(entry.is_regular_file() || entry.is_directory()))
                continue;

            fs::path          ep       = entry.path();
            const std::string filename = ep.filename().string();
            if (!std::regex_match(filename, r))
                continue;

            if (is_dir)
            {
                // bra_log_debug("Matched dir: %s", filename.c_str());

                // TODO: if recursive...
                // if(recursive)
                // const std::string p = pattern.size() > ep.string().size() ? pattern.substr(ep.string().size()) : pattern;
                // res                 &= search(ep, p);
                continue;
            }
            else
            {
                // ep is a file
                if (!try_sanitize(ep))
                {
                    bra_log_error("not a valid file: %s", ep.string().c_str());
                    return false;
                }

                out_files.push_back(ep);
            }
        }

        return true;
    }
    catch (const fs::filesystem_error& e)
    {
        bra_log_error("Filesystem: %s", e.what());
        return false;
    }
    catch (const std::regex_error& e)
    {
        bra_log_error("Regex: %s", e.what());
        return false;
    }
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
