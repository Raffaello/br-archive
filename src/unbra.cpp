#include <lib_bra.h>
#include <version.h>

#include <format>
#include <iostream>
#include <filesystem>
#include <string>
#include <list>
#include <algorithm>
#include <limits>

#include <cstdint>
#include <cstdio>


using namespace std;

namespace fs = std::filesystem;


fs::path g_bra_file;
bool     g_list = false;

void help()
{
    cout << format("BR-Archive Utility Version: {}", VERSION) << endl;
    cout << endl;
    cout << format("Usage:") << endl;
    cout << endl;
    cout << format("  unbra (input_file).BRa") << endl;
    cout << format("The [output_file(s)] will be as they are stored in the archive") << endl;
    cout << format("Example:") << endl;
    cout << format("  unbra test.BRa") << endl;
    cout << endl;
    cout << format("(input_file) : (single file only for now) the output") << endl;
    // cout << format("(output_file): output file name without extension") << endl;
    cout << endl;
    cout << format("Options:") << endl;
    cout << format("--help | -h : display this page.") << endl;
    cout << format("--list | -l : view archive content.") << endl;
    cout << endl;
}

bool parse_args(int argc, char* argv[])
{
    g_bra_file.clear();

    if (argc < 2)
    {
        help();
        return false;
    }

    g_list = false;
    for (int i = 1; i < argc; i++)
    {
        string s = argv[i];
        if (s == "--help" || s == "-h")
        {
            help();
            exit(0);
        }
        else if (s == "--list" || s == "-l")
        {
            // list content
            g_list = true;
        }
        // check if it is a file
        else if (fs::exists(s))
        {
            if (!fs::is_regular_file(s))
            {
                cout << format("{} is not a file!", s) << endl;
                return 2;
            }

            g_bra_file = s;
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
    if (g_bra_file.empty())
    {
        cerr << "ERROR: no input file provided" << endl;
        return false;
    }

    return true;
}

bool bra_io_list_meta_file(bra_io_file_t& f)
{
    bra_meta_file_t mf;

    if (!bra_io_read_meta_file(&f, &mf))
        return false;

    cout << format("- size: {} bytes | {}", mf.name, mf.data_size) << endl;
    bra_meta_file_free(&mf);

    // skip data content
    const uint64_t ds = mf.data_size;
    if (!bra_io_skip_data(&f, ds))
    {
        bra_io_read_error(&f);
        return false;
    }


    return true;
}

int main(int argc, char* argv[])
{
    if (!parse_args(argc, argv))
        return 1;

    if (!validate_args())
        return 1;

    // adjust input file extension
    if (g_bra_file.extension() != BRA_FILE_EXT)
        g_bra_file += BRA_FILE_EXT;

    // header
    bra_header_t  bh{};
    bra_io_file_t f{};
    if (!bra_io_open(&f, g_bra_file.string().c_str(), "rb"))
        return 1;

    if (!bra_io_read_header(&f, &bh))
        return 1;

    cout << format("{} containing num files: {}", BRA_NAME, bh.num_files) << endl;

    for (uint32_t i = 0; i < bh.num_files; i++)
    {
        if (g_list)
        {
            // bra_meta_file_t mf;

            // if (!bra_io_read_meta_file(&f, &mf))
            //     return 1;

            // const uint64_t ds = mf.data_size;
            // bra_meta_file_free(&mf);

            // // skip data content
            // if (!bra_io_skip_data(&f, ds))
            // {
            //     bra_io_read_error(&f);
            //     return 2;
            // }

            // cout << format("{}.  size: {} bytes | {}", i + 1, mf.name, ds) << endl;

            if (!bra_io_list_meta_file(f))
                return 2;
        }
        else
        {
            if (!bra_io_decode_and_write_to_disk(&f))
                return 1;
        }
    }

    bra_io_close(&f);
    return 0;
}
