#include <lib_bra.h>
#include <bra_log.h>
#include <bra_fs.hpp>
#include <version.h>

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


static std::set<fs::path> g_files;
static fs::path           g_out_filename;
static bool               g_sfx        = false;
static bool               g_always_yes = false;

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
    cout << format("              Directories expand to <dir/*> (non-recursive); empty directories are ignored.") << endl;
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
        // first part is about arguments sections
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
                bra_log_error("%s missing argument <output_filename>", s.c_str());
                return false;
            }

            g_out_filename = argv[i];
        }
        else if (s == "--yes" || s == "-y")
        {
            g_always_yes = true;
        }
        else
        {
            // FS sub-section
            fs::path p = s;
            // check if it is file or a dir
            if (bra::fs::file_exists(p))
            {
                // check file path
                if (!bra::fs::try_sanitize(p))
                {
                    bra_log_error("path not valid: %s", p.string().c_str());
                    return false;
                }

                if (!g_files.insert(p).second)
                    bra_log_warn("duplicate file/dir given in input: %s", p.string().c_str());
            }
            else if (bra::fs::dir_exists(p))
            {
                // This should match exactly the directory.
                // so need to be converted as a wildcard adding a `/*' at the end
                if (!bra::fs::wildcard_expand(p, g_files))
                {
                    bra_log_error("path not valid: %s", p.string().c_str());
                    return false;
                }
            }
            // check if it is a wildcard
            else if (bra::fs::is_wildcard(p))
            {
                if (!bra::fs::wildcard_expand(p, g_files))
                {
                    bra_log_error("unable to expand wildcard: %s", s.c_str());
                    return false;
                }
            }
            else
            {
                bra_log_error("unknown argument/file doesn't exist: %s", s.c_str());
                return false;
            }
        }
    }

    return true;
}

bool validate_args()
{
    if (g_files.empty())
    {
        bra_log_error("no input file provided");
        return false;
    }

    if (!bra::fs::file_set_add_dir(g_files))
        return false;

#ifndef NDEBUG
    bra_log_debug("Detected files:");
    for (const auto& f : g_files)
        bra_log_debug("- %s", f.string().c_str());
#endif

    // TODO: Here could also start encoding the filenames
    //       and use them in compressed format to save on disk
    //       only if it less space (but it might be not).
    //       Need to test eventually later on

    if (g_out_filename.empty())
    {
        bra_log_error("no output file provided");
        return false;
    }

    // adjust input file extension
    if (g_sfx)
    {
        g_out_filename = bra::fs::filename_sfx_adjust(g_out_filename, true);
        // locate sfx bin
        if (!bra::fs::file_exists(BRA_SFX_FILENAME))
        {
            bra_log_error("unable to find %s-SFX module", BRA_NAME);
            return false;
        }

        // check if SFX_TMP exists...
        const auto overwrite = bra::fs::file_exists_ask_overwrite(g_out_filename, g_always_yes);
        if (overwrite)
        {
            if (!*overwrite)
                return false;

            cout << format("Overwriting file: {}", g_out_filename.string()) << endl;
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
    else    // the output directory might not exists...
    {
        // create the parent directory if needed.
        const fs::path pof = g_out_filename.parent_path();
        if (!pof.empty())
        {
            if (!bra::fs::dir_make(pof))
                return false;
        }
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
        fs::path p = fn_;
        // no need to sanitize it again, but it won't hurt neither
        if (!bra::fs::try_sanitize(p))
        {
            bra_log_error("invalid path: %s", fn_.string().c_str());
            return 1;
        }

        const string fn = p.generic_string();
        if (!bra_io_encode_and_write_to_disk(&f, fn.c_str()))
            return 1;
        else
            ++written_num_files;
        // }
    }

    bra_io_close(&f);

    // TODO: do an SFX should be in the lib_bra
    if (g_sfx)
    {
        fs::path sfx_path = g_out_filename;
        sfx_path.replace_extension(BRA_SFX_FILE_EXT);

        cout << format("creating Self-extracting archive {}...", sfx_path.string());

        if (!fs::copy_file(BRA_SFX_FILENAME, sfx_path, fs::copy_options::overwrite_existing))
        {
        BRA_SFX_IO_ERROR:
            bra_log_error("unable to create a %s-SFX file", BRA_NAME);
            return 2;
        }

        bra_io_file_t f{};
        if (!bra_io_open(&f, sfx_path.string().c_str(), "rb+"))
            goto BRA_SFX_IO_ERROR;

        if (!bra_io_seek(&f, 0, SEEK_END))
        {
            bra_io_file_seek_error(&f);
            return 2;
        }

        // save the start of the payload for later...
        const int64_t header_offset = bra_io_tell(&f);
        if (header_offset < 0L)
        {
            bra_io_file_error(&f, "tell");
            return 2;
        }

        // append bra file
        bra_io_file_t f2{};
        if (!bra_io_open(&f2, out_fn.c_str(), "rb"))
        {
            bra_io_close(&f);
            return 2;
        }

        auto file_size = bra::fs::file_size(out_fn);
        if (!file_size)
        {
            bra_io_close(&f);
            bra_io_close(&f2);
            return 2;
        }

        if (!bra_io_copy_file_chunks(&f, &f2, *file_size))
            return 2;

        bra_io_close(&f2);
        // write footer
        if (!bra_io_write_footer(&f, header_offset))
        {
            bra_io_file_write_error(&f);
            return 2;
        }

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
            bra_log_warn("unable to remove temporary file %s", g_out_filename.string().c_str());

        cout << "OK" << endl;
    }

    return 0;
}
