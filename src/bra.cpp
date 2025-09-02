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
bool               g_sfx        = false;
bool               g_always_yes = false;

//////////////////////////////////////////////////////////////////////////

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
    cout << format("--yes  | -y : this will force a 'yes' response to all the user questions.") << endl;
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
    g_out_filename.clear();
    g_sfx        = false;
    g_always_yes = false;
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
        else if (s == "--yes" || s == "-y")
        {
            g_always_yes = true;
        }
        // check if it is file or a dir
        else if (bra::fs::file_exists(s))
        {
            fs::path p = s;

            // check file path
            if (!bra::fs::try_sanitize(p))
            {
                cerr << format("ERROR: path not valid: {}", p.string()) << endl;
                return false;
            }

            if (!g_files.insert(p).second)
                cout << format("WARNING: duplicate file/dir given in input: {}", p.string()) << endl;
        }
        else if (bra::fs::dir_exists(s))
        {
            // This should match exactly the directory.
            // so need to be converted as a wildcard adding a `/*' at the end
            fs::path p = fs::path(s) / "*";
            if (!bra::fs::wildcard_expand(p, g_files))
            {
                cerr << format("ERROR: path not valid: {}", p.string()) << endl;
                return false;
            }
        }
        // check if it is a wildcard
        else if (bra::fs::isWildcard(s))
        {
            if (!bra::fs::wildcard_expand(s, g_files))
                return false;
        }
        else
        {
            cerr << format("ERROR: unknown argument/file doesn't exist: {}", s) << endl;
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
    // TODO: move into bra_fs as bra::fs::file_set_add_dir
    std::list<fs::path> listFiles(g_files.begin(), g_files.end());
    g_files.clear();
    while (!listFiles.empty())
    {
        // NOTE: as it is a set i can just insert multiple time the directory
        //       and when iterate later it, resolve it resolve on it.
        //       need to keep track of the last directory entry to remove from the file.

        auto f = listFiles.front();
        listFiles.pop_front();

        if (bra::fs::dir_exists(f))
        {
            // TODO: only if recursive is not enabled
            //       recursive will also store empty directories.
            cout << format("DEBUG: removing empty directory") << endl;
        }
        else if (!(bra::fs::file_exists(f)))
        {
            cerr << format("ERROR: {} is neither a regular file nor a directory", f.string()) << endl;
            continue;
        }

        fs::path f_ = f;
        if (!bra::fs::try_sanitize(f_))
        {
            cerr << format("ERROR: path not valid: {} - ({})", f.string(), f_.string()) << endl;
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
        g_out_filename = bra::fs::filename_sfx_adjust(g_out_filename, true);
        // locate sfx bin
        if (!bra::fs::file_exists(BRA_SFX_FILENAME))
        {
            cerr << format("ERROR: unable to find {}-SFX module", BRA_NAME) << endl;
            return false;
        }

        if (bra::fs::file_exists(g_out_filename))
        {
            cerr << format("ERROR: Temporary SFX File {} already exists.", g_out_filename.string()) << endl;
            return false;
        }
    }
    else
    {
        g_out_filename = bra::fs::filename_archive_adjust(g_out_filename);
    }

    fs::path p = g_out_filename;
    if (g_sfx)
        p = p.replace_extension(BRA_SFX_FILE_EXT);

    // TODO: this might not be ok
    if (auto res = bra::fs::file_exists_ask_overwrite(p, g_always_yes))
    {
        if (!*res)
            return false;

        cout << format("Overwriting file: {}", p.string()) << endl;
    }

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
        if (!bra_file_encode_and_write_to_disk(&f, fn.c_str()))
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
