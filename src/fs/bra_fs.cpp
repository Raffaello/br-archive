#include <fs/bra_fs.hpp>

#include <log/bra_log.h>
#include <fs/bra_wildcards.hpp>

#include <iostream>
#include <string>
#include <cctype>
#include <regex>
#include <algorithm>
#include <limits>

namespace bra::fs
{

namespace fs = std::filesystem;

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////


bool try_sanitize(std::filesystem::path& path) noexcept
{
    error_code ec;
    auto       err = [&path, &ec]() {
        bra_log_error("unable to sanitize path %s: %s", path.string().c_str(), ec.message().c_str());
        return false;
    };

    auto abs = fs::absolute(path, ec);
    if (ec)
        return err();

    fs::path p = abs.lexically_relative(fs::current_path(ec));
    if (ec)
        return err();
    if (p.empty())
        return false;

    p = fs::relative(p, fs::current_path(), ec);
    if (ec)
        return err();

    if (p.empty())
    {
        if (path.is_absolute() || path.has_root_name())
            return false;
    }
    else
        path = p;

    path = path.lexically_normal().generic_string();
    for (const auto& p_ : path)
    {
        if (p_ == "..")
            return false;
    }

    return !path.empty();
}

bool dir_exists(const std::filesystem::path& path) noexcept
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

bool dir_make(const std::filesystem::path& path) noexcept
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

bool dir_isSubDir(const std::filesystem::path& base, const std::filesystem::path& path) noexcept
{
    error_code ec;

    fs::path b = base;
    fs::path p = path;

    if (!try_sanitize(b) || !try_sanitize(p))
        return false;

    if (b.empty() || p.empty() || b == p)
        return false;

    p = fs::relative(p, b, ec);
    if (ec)
    {
        bra_log_error("unable to detect %s subdir of %s: %s", path.string().c_str(), base.string().c_str(), ec.message().c_str());
        return false;
    }

    // Required: ensure p is a relative subdir of base (not "." or starting with ".")
    if (!try_sanitize(p))
        return false;

    return !p.empty() && !p.native().starts_with('.');
}

std::filesystem::path filename_archive_adjust(const std::filesystem::path& path) noexcept
{
    fs::path p = path;

    if (p.extension() != BRA_FILE_EXT)
        p += BRA_FILE_EXT;

    return p;
}

std::filesystem::path filename_sfx_adjust(const std::filesystem::path& path, const bool tmp) noexcept
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
    {
        p += BRA_FILE_EXT;
        p += sfx_ext;
    }

    return p;
}

bool file_exists(const std::filesystem::path& path) noexcept
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

std::optional<bool> file_exists_ask_overwrite(const std::filesystem::path& path, bra_fs_overwrite_policy_e& overwrite_policy, const bool single_overwrite) noexcept
{
    if (!file_exists(path))
        return nullopt;

    char c;

    switch (overwrite_policy)
    {
    case BRA_OVERWRITE_ALWAYS_YES:
        return true;
    case BRA_OVERWRITE_ALWAYS_NO:
        return false;
    default:
        break;
    }

    if (single_overwrite)
        bra_log_printf("File %s already exists. Overwrite? ([y]es/[n]o) ", path.string().c_str());
    else
        bra_log_printf("File %s already exists. Overwrite? ([y]es/[n]o/[A]ll/N[o]ne) ", path.string().c_str());

    bra_log_flush();
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
            if (c == 'a' || c == 'o')
                break;
        }
    }
    while (c != 'y' && c != 'n');
    bra_log_printf("\n");

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

std::optional<bra_attr_t> file_attributes(const std::filesystem::path& path) noexcept
{
    std::error_code ec;
    auto            err = [&path, &ec]() {
        bra_log_error("unable to read file attributes of %s: %s ", path.string().c_str(), ec.message().c_str());
        return nullopt;
    };

    const fs::file_status fileStatus = fs::status(path, ec);
    if (ec)
        return err();
    const fs::file_type fileType = fileStatus.type();
    switch (fileType)
    {
        using enum fs::file_type;

    default:
        [[fallthrough]];
    case none:
        [[fallthrough]];
    case not_found:
        [[fallthrough]];
    case block:
        [[fallthrough]];
    case character:
        [[fallthrough]];
    case fifo:
        [[fallthrough]];
    case socket:
        [[fallthrough]];
    case unknown:
        return nullopt;
        break;

    case regular:
        return BRA_ATTR_TYPE_FILE;
    case directory:
        return BRA_ATTR_TYPE_SUBDIR;
    case symlink:
        return BRA_ATTR_TYPE_SYM;
    }

    return nullopt;
}

std::optional<uint64_t> file_size(const std::filesystem::path& path) noexcept
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

    return nullopt;
}

bool file_remove(const std::filesystem::path& path) noexcept
{
    if (!file_exists(path))
        return true;

    error_code ec;

    fs::remove(path, ec);
    if (ec)
    {
        bra_log_error("unable to remove file %s: %s", path.string().c_str(), ec.message().c_str());
        return false;
    }

    return true;
}

bool file_rename(const std::filesystem::path& from, const std::filesystem::path& to) noexcept
{
    error_code ec;

    // No-op when source and destination are the same file/path
    if (from == to || fs::equivalent(from, to, ec))
        return true;

    ec.clear();
    // if erroring is most likely not present,
    // if it is for something else, will fail again later.
    // so no need to check ec.
    if (file_exists(from))
        fs::remove(to, ec);

    fs::rename(from, to, ec);
    if (ec)
    {
        // Fallback: copy then remove source
        ec.clear();
        fs::copy_file(from, to, fs::copy_options::overwrite_existing, ec);
        if (ec)
        {
            bra_log_error("unable to rename %s to %s (%s)", from.string().c_str(), to.string().c_str(), ec.message().c_str());
            return false;
        }

        fs::remove(from, ec);    // best-effort cleanup
        if (ec)
            bra_log_warn("unable to clean-up %s  (%s)", from.string().c_str(), ec.message().c_str());
    }

    return true;
}

bool file_permissions(const std::filesystem::path& path, const std::filesystem::perms permissions, const std::filesystem::perm_options perm_options) noexcept
{
    if (!file_exists(path))
        return false;

    error_code ec;

    fs::permissions(path, permissions, perm_options, ec);
    if (ec)
    {
        bra_log_error("unable to set permissions on %s: %s", path.string().c_str(), ec.message().c_str());
        return false;
    }

    return true;
}

bool file_set_add_dirs(std::set<std::filesystem::path>& files) noexcept
{
    std::list<fs::path> listFiles(files.begin(), files.end());
    files.clear();
    while (!listFiles.empty())
    {
        // NOTE: as it is a set i can just insert multiple time the directory
        //       and when iterate later it, resolve it on it.
        //       need to keep track of the last directory entry to remove from the file.

        const auto f = listFiles.front();
        listFiles.pop_front();

        // skip current dir "." as no valuable information.
        // it is by default starting from there.
        if (f == ".")
            continue;

        if (!dir_exists(f) && !file_exists(f))
        {
            bra_log_error("%s is neither a regular file nor a directory", f.string().c_str());

            return false;
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

bool search(const std::filesystem::path& dir, const std::string& pattern, std::list<std::filesystem::path>& out_files, const bool recursive) noexcept
{
    try
    {
        const std::regex r(pattern);

        // TODO: add a cli flag for fs::directory_options::skip_permission_denied
        // TODO: add a cli flag for fs::directory_options::follow_directory_symlink (to replace symlink with the real file)
        for (const auto& entry : fs::directory_iterator(dir))
        {
            fs::path   ep     = entry.path();
            const bool is_dir = dir_exists(ep);
            if (!(file_exists(ep) || is_dir))
                continue;

            const std::string filename = ep.filename().string();

            if (is_dir)
            {
                // TODO: it might be better to use recursive_directory_iterator instead
                //       of bra::fs::search being recursive itself.
                if (!recursive)
                {
                    bra_log_info("Skip directory: %s", filename.c_str());
                    continue;
                }

                bra_log_debug("Matched dir: %s", ep.string().c_str());
                // out_files.push_back(ep);    // but if it is not a wildcard match it shouldn't be added.
                if (!search(ep, pattern, out_files, recursive))
                    return false;
            }

            // here is either a file or a dir
            if (!std::regex_match(filename, r))
                continue;

            // ep is a file
            if (!try_sanitize(ep))
            {
                bra_log_error("not a valid file: %s", ep.string().c_str());
                return false;
            }

            out_files.push_back(ep);
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

bool search_wildcard(const std::filesystem::path& wildcard_path, std::set<std::filesystem::path>& out_files, const bool recursive) noexcept
{
    fs::path p = wildcard_path.generic_string();

    if (!bra::wildcards::is_wildcard(p))
        return false;

    if (!try_sanitize(p))
        return false;

    const fs::path dir     = bra::wildcards::wildcard_extract_dir(p);
    const string   pattern = bra::wildcards::wildcard_to_regexp(p.string());

    std::list<fs::path> files;
    if (!bra::fs::search(dir, pattern, files, recursive))
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

bool make_tree(const std::set<std::filesystem::path>& set_files, std::map<std::filesystem::path, std::set<std::filesystem::path>>& tree, size_t& out_tree_size) noexcept
{
    out_tree_size = 0;
    for (const auto& f : set_files)
    {
        if (dir_exists(f))
        {
            tree.insert({f, std::set<fs::path>()});
            ++out_tree_size;
        }
        else if (file_exists(f))
        {
            auto d = f.parent_path();
            tree[d].insert(f.filename());
            ++out_tree_size;
        }
        else
            return false;
    }

    return true;
}

}    // namespace bra::fs
