#include <lib_bra.h>
#include <bra_fs.hpp>
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
bool     g_listContent = false;

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

    g_listContent = false;
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
            g_listContent = true;
        }
        // check if it is a file
        else
        {
            fs::path p = bra::fs::filename_archive_adjust(s);
            if (bra::fs::file_exists(p))
                g_bra_file = p;
            else
            {
                cout << format("unknown argument: {}", s) << endl;
                return false;
            }
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

char unbra_list_meta_file_attributes(const uint8_t attributes)
{
    switch (attributes)
    {
    case 0:
        return 'f';
    case 1:
        return 'd';
    default:
        return '?';
    }
}

std::string format_bytes(const size_t bytes)
{
    constexpr size_t KB = 1024;
    constexpr size_t MB = KB * 1024;
    constexpr size_t GB = MB * 1024;

    if (bytes >= GB)
        return format("{:2.2f} GB", static_cast<double>(bytes) / GB);
    else if (bytes >= MB)
        return format("{:2.2f} MB", static_cast<double>(bytes) / MB);
    else if (bytes >= KB)
        return format("{:2.2f} KB", static_cast<double>(bytes) / KB);
    else
        return format("{:4} B ", bytes);
}

bool unbra_list_meta_file(bra_io_file_t& f)
{
    bra_meta_file_t mf;

    if (!bra_io_read_meta_file(&f, &mf))
        return false;

    const uint64_t ds = mf.data_size;
    cout << format("- attr: {} | size: {:4} | {}", unbra_list_meta_file_attributes(mf.attributes), format_bytes(mf.data_size), mf.name) << endl;

    bra_meta_file_free(&mf);

    // skip data content
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

    // forcing to work only on BRA_FILE_EXT
    if (g_bra_file.extension() != BRA_FILE_EXT)
    {
        cerr << "CRITICAL: unexpected" << endl;
        return 99;
    }

    // header
    bra_io_header_t bh{};
    bra_io_file_t   f{};
    if (!bra_io_open(&f, g_bra_file.string().c_str(), "rb"))
        return 1;

    if (!bra_io_read_header(&f, &bh))
        return 1;

    cout << format("{} containing num files: {}", BRA_NAME, bh.num_files) << endl;
    for (uint32_t i = 0; i < bh.num_files; i++)
    {
        if (g_listContent)
        {
            if (!unbra_list_meta_file(f))
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
