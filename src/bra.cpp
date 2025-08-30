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
size_t             g_num_files;
fs::path           g_out_filename;

bool g_sfx = false;

void help()
{
    cout << format("BR-Archive Utility Version: {}", VERSION) << endl;
    cout << endl;
    cout << format("Usage:") << endl;
    cout << format("  bra [-s] -o <output_file> <input_file 1> [<input_file 2> ...]") << endl;
    cout << format("The [output_file] will be with a .BRa extension") << endl;
    cout << format("Example:") << endl;
    cout << format("  bra -o test test.txt") << endl;
    cout << format("  bra -o text *.txt") << endl;
    cout << endl;
    cout << format("(input_file) : path to an existing file or a wildcard pattern") << endl;
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
    g_num_files = 0;
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
        // check if it is file
        else if (bra_fs_file_exists(s))
        {
            fs::path p = s;

            // check file path
            if (!bra_fs_try_sanitize(p))
            {
                cerr << format("ERROR: path not valid: {}", p.string()) << endl;
                return false;
            }

            if (g_files.insert(p).second)
                ++g_num_files;
            else
                cout << format("WARNING: duplicate file given in input: {}", p.string()) << endl;
        }
        // check if it is a wildcard
        else if (bra_fs_isWildcard(s))
        {
            fs::path p             = s;
            p                      = p.generic_string();
            const fs::path dir     = bra_fs_wildcard_extract_dir(p);
            const string   pattern = bra_fs_wildcard_to_regexp(p.string());

            size_t num_files = 0;
            for ([[maybe_unused]] auto const& fn : bra_fs_co_search(dir, pattern))
                ++num_files;

            if (num_files > 0)
            {
                g_num_files += num_files;
                if (!g_files.insert(p).second)
                    cout << format("WARNING: duplicate file given in input: {}", p.string()) << endl;
            }
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

    if (g_files.size() > numeric_limits<uint32_t>::max())
    {
        cerr << format("ERROR: Too many files, not supported yet: {}/{}", g_files.size(), numeric_limits<uint32_t>::max());
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
            cerr << "ERROR: unable to find BRa-SFX module" << endl;
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
    cout << format("Archiving File: {}...", fn);
    // 1. file name length
    if (fn.size() > std::numeric_limits<uint8_t>::max())
    {
        cerr << std::format("ERROR: filename too long: {}", fn) << endl;
        return false;
    }

    const uint8_t fn_size = static_cast<uint8_t>(fn.size());
    if (fwrite(&fn_size, sizeof(uint8_t), 1, f->f) != 1)
    {
    BRA_IO_ENCODE_WRITE_ERR:
        cerr << format("ERROR: writing file: {}", fn) << endl;
        bra_io_close(f);
        return false;
    }

    // 2. file name
    if (fwrite(fn.c_str(), sizeof(char), fn_size, f->f) != fn_size)
        goto BRA_IO_ENCODE_WRITE_ERR;

    // 3. data size
    std::error_code ec;
    const uint64_t  ds = fs::file_size(fn, ec);
    if (ec || fwrite(&ds, sizeof(uint64_t), 1, f->f) != 1)
        goto BRA_IO_ENCODE_WRITE_ERR;

    // 4. data
    bra_io_file_t f2{};
    if (!bra_io_open(&f2, fn.c_str(), "rb"))
    {
        bra_io_close(f);
        return false;
    }

    if (!bra_io_copy_file_chunks(f, &f2, ds))
        return false;

    bra_io_close(&f2);
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
    if (!bra_io_write_header(&f, static_cast<uint32_t>(g_num_files)))
        return 1;

    uint32_t written_num_files = 0;
    for (const auto& fn_ : g_files)
    {
        if (bra_fs_isWildcard(fn_))
        {
            fs::path       p       = fn_.generic_string();
            const fs::path dir     = bra_fs_wildcard_extract_dir(p);
            const string   pattern = bra_fs_wildcard_to_regexp(p.string());
            for (auto const& fn2 : bra_fs_co_search(dir, pattern))
            {
                p = fn2;
                if (!bra_fs_try_sanitize(p))
                {
                    cerr << format("ERROR: file not found or not valid: {}", fn2.string());
                    return 1;
                }

                if (p == g_out_filename)
                    continue;

                const string fn = fs::relative(p).generic_string();
                if (!bra_file_encode_and_write_to_disk(&f, fn))
                    return 1;
                else
                    ++written_num_files;
            }
        }
        else
        {
            const string fn = fs::relative(fn_).generic_string();

            if (!bra_file_encode_and_write_to_disk(&f, fn))
                return 1;
            else
                ++written_num_files;
        }
    }

    bra_io_close(&f);

    // check num files
    if (written_num_files != g_num_files)
    {
        cerr << format("ERROR: written files are not equal to expected ones: {}/{}", written_num_files, g_num_files) << endl;
        return 2;
    }

    if (g_sfx)
    {
        fs::path sfx_path = g_out_filename;
        sfx_path.replace_extension(BRA_SFX_FILE_EXT);

        cout << format("creating Self-extracting archive {}...", sfx_path.string());

        if (!fs::copy_file(BRA_SFX_FILENAME, sfx_path, fs::copy_options::overwrite_existing))
        {
        BRA_SFX_IO_ERROR:
            cerr << "ERROR: unable to create a BRa-SFX file" << endl;
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
        const int64_t data_offset = bra_io_tell(&f);
        if (data_offset < 0L)
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
        if (!bra_io_write_footer(&f, data_offset))
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
        if (!fs::remove(g_out_filename))
            cout << format("WARN: unable to remove temporary file {}", g_out_filename.string()) << endl;

        cout << "OK" << endl;
    }

    return 0;
}
