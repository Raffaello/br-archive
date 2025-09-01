#include <lib_bra.h>
#include <version.h>

#include <bra_fs.hpp>

#include <format>
#include <iostream>
#include <filesystem>
#include <string>
#include <set>
#include <algorithm>
#include <limits>
#include <system_error>
// #include <print>   // doesn't work smoothly on MSYS2

#include <cstdint>
#include <cstdio>


using namespace std;

namespace fs = std::filesystem;


std::set<fs::path> g_files;
fs::path           g_out_filename;
bool               g_sfx = false;

//////////////////////////////////////////////////////////////////////////

/**
 * @brief
 *
 * @todo should be moved into bra_fs ?
 *
 * @param path
 * @return true
 * @return false
 */
bool bra_expand_wildcards(const fs::path& path)
{
    fs::path       p       = path.generic_string();
    const fs::path dir     = bra_fs_wildcard_extract_dir(p);
    const string   pattern = bra_fs_wildcard_to_regexp(p.string());

    std::list<fs::path> files;
    if (!bra_fs_search(dir, pattern, files))
    {
        cerr << format("ERROR: not a valid wildcard: {}", p.string()) << endl;
        return false;
    }

    for (const auto& p : files)
    {
        if (!g_files.insert(p).second)
            cout << format("WARNING: duplicate file given in input: {}", p.string()) << endl;
    }

    files.clear();
    return true;
}

/////////////////////////////////////////////////////////////////////////

void help()
{
    cout << format("BR-Archive Utility Version: {}", VERSION) << endl;
    cout << endl;
    cout << format("Usage:") << endl;
    cout << format("  bra [-s] -o <output_file> <input_file1> [<input_file2> ...]") << endl;
    cout << format("The [output_file] will be with a {} extension", BRA_FILE_EXT) << endl;
    cout << format("Example:") << endl;
    cout << format("  bra -o test test.txt") << endl;
    cout << format("  bra -o test *.txt") << endl;
    cout << endl;
    cout << format("(input_file): path to an existing file or a wildcard pattern") << endl;
    cout << endl;
    cout << format("Options:") << endl;
    cout << format("--help | -h : display this page.") << endl;
    cout << format("--sfx  | -s : generate a self-extracting archive") << endl;
    cout << format("--out  | -o : <output_filename> it takes the path of the output file.") << endl;
    cout << format("              If the extension {} is missing it will be automatically added.", BRA_FILE_EXT) << endl;
    cout << endl;
}

bool parse_args(int argc, char* argv[])
{
    if (argc < 2)
    {
        help();
        return false;
    }

    g_files.clear();
    for (int i = 1; i < argc; ++i)
    {
        string s = argv[i];

        if (s == "--help" || s == "-h")
        {
            help();
            exit(0);
        }
        else if (s == "--sfx" || s == "-s")
        {
            g_sfx = true;
        }
        else if (s == "--out" || s == "-o")
        {
            // next arg is output file
            ++i;
            if (i >= argc)
            {
                cout << format("ERROR: {} missing argument <output_filename>", s) << endl;
                return false;
            }

            g_out_filename = argv[i];
        }
        // check if it is file or a dir
        else if (bra_fs_file_exists(s))
        {
            fs::path p = s;

            // check file path
            if (!bra_fs_try_sanitize(p))
            {
                cerr << format("ERROR: path not valid: {}", p.string()) << endl;
                return false;
            }

            if (!g_files.insert(p).second)
                cout << format("WARNING: duplicate file/dir given in input: {}", p.string()) << endl;
        }
        else if (bra_fs_dir_exists(s))
        {
            // This should match exactly the directory.
            // so need to be converted as a wildcard adding a `/*' at the end
            fs::path p = fs::path(s) / "*";
            if (!bra_fs_try_sanitize(p) || !bra_fs_isWildcard(p) || !bra_expand_wildcards(p))
            {
                cerr << format("ERROR: path not valid: {}", p.string()) << endl;
                return false;
            }
        }
        // check if it is a wildcard
        else if (bra_fs_isWildcard(s))
        {
            if (!bra_expand_wildcards(s))
                return false;
        }
        else
        {
            cout << format("unknown argument: {}", s) << endl;
            return false;
        }
    }

    return true;
}

bool validate_args()
{
    if (g_files.empty())
    {
        cerr << "ERROR: no input file provided" << endl;
        return false;
    }

    // Extend the file set with directories
    std::list<fs::path> listFiles(g_files.begin(), g_files.end());
    g_files.clear();
    while (!listFiles.empty())
    {
        // NOTE: as it is a set i can just insert multiple time the directory
        //       and when iterate later it, resolve it resolve on it.
        //       need to keep track of the last directory entry to remove from the file.

        auto f = listFiles.front();
        listFiles.pop_front();

        if (bra_fs_dir_exists(f))
        {
            // TODO: only if recursive is not enabled
            //       recursive will also store empty directories.
            cout << format("DEBUG: removing empty directory") << endl;
        }
        if (!(bra_fs_file_exists(f)))
        {
            cerr << format("ERROR: what is this? {}", f.string()) << endl;
            continue;
        }

        fs::path f_ = f;
        if (!bra_fs_try_sanitize(f_))
        {
            cerr << format("ERROR: what is this? {} - {}", f.string(), f_.string()) << endl;
            return false;
        }

        fs::path p_;
        p_.clear();
        for (const auto& p : f_)
        {
            p_ /= p;
            g_files.insert(p_);
        }
        if (p_ != f_)
        {
            cerr << format("ERROR: expected {} == {}", p_.string(), f_.string()) << endl;
            return false;
        }
    }

    cout << format("files:") << endl;
    for (const auto& f : g_files)
        cout << format("- {}", f.string()) << endl;

    // TODO: Here could also start encoding the filenames
    //       and use them in compressed format to save on disk
    //       only if it less space (but it might be not).
    //       Need to test eventually later on

    if (g_files.size() > numeric_limits<uint32_t>::max())
    {
        cerr << format("ERROR: Too many files, not supported yet: {}/{}", g_files.size(), numeric_limits<uint32_t>::max()) << endl;
        return false;
    }

    if (g_out_filename.empty())
    {
        cerr << "ERROR: no output file provided" << endl;
        return false;
    }

    // adjust input file extension
    if (g_sfx)
    {
        g_out_filename = bra_fs_filename_sfx_adjust(g_out_filename, true);
        // locate sfx bin
        if (!bra_fs_file_exists(BRA_SFX_FILENAME))
        {
            cerr << format("ERROR: unable to find {}-SFX module", BRA_NAME) << endl;
            return false;
        }

        if (bra_fs_file_exists(g_out_filename))
        {
            cerr << format("ERROR: Temporary SFX File {} already exists.", g_out_filename.string()) << endl;
            return false;
        }
    }
    else
    {
        g_out_filename = bra_fs_filename_archive_adjust(g_out_filename);
    }

    fs::path p = g_out_filename;
    if (g_sfx)
        p = p.replace_extension(BRA_SFX_FILE_EXT);

    // TODO: this might not be ok
    if (auto res = bra_fs_file_exists_ask_overwrite(p, false))
    {
        if (!*res)
            return false;

        cout << format("Overwriting file: {}", p.string()) << endl;
    }

    return true;
}

/**
 * @brief
 *
 * @todo move into lib_bra
 *
 * @param f
 * @param fn
 * @return true
 * @return false
 */
bool bra_file_encode_and_write_to_disk(bra_io_file_t* f, const string& fn)
{
    cout << format("Archiving ");

    // 1. attributes
    auto attributes = bra_fs_file_attributes(fn);
    if (!attributes)
    {
        cerr << format("ERROR: {} has unknown attribute", fn) << endl;
    BRA_IO_WRITE_CLOSE_ERROR:
        bra_io_close(f);
        return false;
    }
    switch (*attributes)
    {
    case BRA_ATTR_DIR:
        cout << format("dir: {}...", fn);
        break;
    case BRA_ATTR_FILE:
        cout << format("file: {}...", fn);
        break;
    default:
        goto BRA_IO_WRITE_CLOSE_ERROR;
    }

    // 2. file name length
    if (fn.size() > std::numeric_limits<uint8_t>::max())
    {
        cerr << std::format("ERROR: filename too long: {}", fn) << endl;
        return false;
    }

    const uint8_t fn_size = static_cast<uint8_t>(fn.size());
    const auto    ds      = bra_fs_file_size(fn);
    if (!ds)
        goto BRA_IO_WRITE_CLOSE_ERROR;

    bra_meta_file_t mf{};
    mf.attributes = *attributes;
    mf.name_size  = fn_size;
    mf.data_size  = *ds;
    mf.name       = bra_strdup(fn.c_str());
    if (mf.name == NULL)
        goto BRA_IO_WRITE_CLOSE_ERROR;

    const bool res = bra_io_write_meta_file(f, &mf);
    bra_meta_file_free(&mf);
    if (!res)
        return false;

    // 4. data
    // NOTE: Not saving for directory
    if (attributes == BRA_ATTR_DIR)
    {
        // TODO: store the directory for the next file, to be removed from the name
    }
    else if (attributes == BRA_ATTR_FILE)
    {
        bra_io_file_t f2{};
        if (!bra_io_open(&f2, fn.c_str(), "rb"))
        {
            cerr << format("ERROR: unable to open file: {}", fn) << endl;
            goto BRA_IO_WRITE_CLOSE_ERROR;
        }

        if (!bra_io_copy_file_chunks(f, &f2, *ds))
            return false;

        bra_io_close(&f2);
    }

    cout << "OK" << endl;
    return true;
}

int main(int argc, char* argv[])
{
    if (!parse_args(argc, argv))
        return 1;

    if (!validate_args())
        return 1;

    string        out_fn = g_out_filename.generic_string();
    bra_io_file_t f{};

    // header
    if (!bra_io_open(&f, out_fn.c_str(), "wb"))
        return 1;

    cout << format("Archiving into {}", out_fn) << endl;
    if (!bra_io_write_header(&f, static_cast<uint32_t>(g_files.size())))
        return 1;

    uint32_t written_num_files = 0;
    for (const auto& fn_ : g_files)
    {
        const string fn = fs::relative(fn_).generic_string();
        if (!bra_file_encode_and_write_to_disk(&f, fn))
            return 1;
        else
            ++written_num_files;
        // }
    }

    bra_io_close(&f);

    if (g_sfx)
    {
        fs::path sfx_path = g_out_filename;
        sfx_path.replace_extension(BRA_SFX_FILE_EXT);

        cout << format("creating Self-extracting archive {}...", sfx_path.string());

        if (!fs::copy_file(BRA_SFX_FILENAME, sfx_path, fs::copy_options::overwrite_existing))
        {
        BRA_SFX_IO_ERROR:
            cerr << format("ERROR: unable to create a {}-SFX file", BRA_NAME) << endl;
            return 2;
        }

        bra_io_file_t f{};
        if (!bra_io_open(&f, sfx_path.string().c_str(), "rb+"))
            goto BRA_SFX_IO_ERROR;

        if (!bra_io_seek(&f, 0, SEEK_END))
        {
        BRA_SFX_IO_F_ERROR:
            bra_io_close(&f);
            goto BRA_SFX_IO_ERROR;
        }

        // save the start of the payload for later...
        const int64_t header_offset = bra_io_tell(&f);
        if (header_offset < 0L)
            goto BRA_SFX_IO_F_ERROR;

        // append bra file
        bra_io_file_t f2{};
        if (!bra_io_open(&f2, out_fn.c_str(), "rb"))
            goto BRA_SFX_IO_F_ERROR;

        error_code ec;
        auto       file_size = fs::file_size(out_fn, ec);
        if (ec || !bra_io_copy_file_chunks(&f, &f2, file_size))
            goto BRA_SFX_IO_F_ERROR;

        bra_io_close(&f2);
        // write footer
        if (!bra_io_write_footer(&f, header_offset))
            goto BRA_SFX_IO_ERROR;

        bra_io_close(&f);

// add executable permission on UNIX
#if defined(__unix__) || defined(__APPLE__)
        fs::permissions(sfx_path,
                        fs::perms::owner_exec | fs::perms::owner_read | fs::perms::owner_write |
                            fs::perms::group_exec | fs::perms::group_read |
                            fs::perms::others_exec | fs::perms::others_read,
                        fs::perm_options::add);
#endif

        // remove TMP SFX FILE
        // TODO: better starting with the SFX file then append the file
        //       it will save disk space as in this way requires twice the archive size
        //       to do an SFX
        if (!fs::remove(g_out_filename))
            cout << format("WARN: unable to remove temporary file {}", g_out_filename.string()) << endl;

        cout << "OK" << endl;
    }

    return 0;
}
