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
    cout << format("--help: display this page.") << endl;
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

    for (int i = 1; i < argc; i++)
    {
        string s = argv[i];
        if (s == "--help")
        {
            help();
            exit(0);
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
            cout << format("unknow argument: {}", s) << endl;
            return false;
        }
    }

    if (g_bra_file.empty())
    {
        cerr << "no input file provided" << endl;
        return false;
    }

    return true;
}

FILE* bra_file_open_and_read_header(const char* fn, bra_header_t* out_bh)
{
    FILE* f = fopen(fn, "rb");
    if (f == nullptr)
    {
        cout << format("unable to read {} {} file", g_bra_file.string(), BRA_NAME) << endl;
        return nullptr;
    }

    if (fread(out_bh, sizeof(bra_header_t), 1, f) != 1)
    {
        cout << format("unable to read {} {} file", fn, BRA_NAME) << endl;
        fclose(f);
        return nullptr;
    }

    // check header magic
    if (out_bh->magic != BRA_MAGIC)
    {
        cerr << format("Not a valid {} file", BRA_FILE_EXT) << endl;
        fclose(f);
        return nullptr;
    }

    return f;
}

// bool bra_file_decode_and_write_to_disk(bra_file_t* f)
// {
//     char buf[MAX_BUF_SIZE];

// // 1. filename size
// uint8_t fn_size = 0;
// if (fread(&fn_size, sizeof(uint8_t), 1, f->f) != 1)
// {
// BRA_IO_READ_ERR:
//     cout << format("unable to read {} {} file", f->fn, BRA_NAME) << endl;
//     // fclose(f);
//     return false;
// }

// // 2. filename
// if (fread(buf, sizeof(uint8_t), fn_size, f->f) != fn_size)
//     goto BRA_IO_READ_ERR;

// buf[fn_size]        = '\0';
// const string out_fn = buf;

// // 3. data size
// uintmax_t ds = 0;
// if (fread(&ds, sizeof(uintmax_t), 1, f->f) != 1)
//     goto BRA_IO_READ_ERR;

// // 4. read and write in chunk data
// cout << format("Extracting file: {} ...", out_fn);
// bra_file_t f2{};
// if (!bra_io_open(&f2, out_fn.c_str(), "wb"))
// {
//     cerr << format("unable to write file: {}", out_fn.c_str()) << endl;
//     goto BRA_IO_READ_ERR;
// }

// if (!bra_io_copy_file_chunks(&f2, f, ds))
// {
//     bra_io_close(&f2);
//     return false;
// }

// bra_io_close(&f2);
// cout << "OK" << endl;
// return true;
// }

int main(int argc, char* argv[])
{
    if (!parse_args(argc, argv))
        return 1;

    // adjust input file extension
    if (g_bra_file.extension() != BRA_FILE_EXT)
        g_bra_file.append(BRA_FILE_EXT);

    // header
    bra_header_t bh{};
    bra_file_t   f{};
    if (!bra_io_open(&f, g_bra_file.string().c_str(), "rb"))
        return 1;

    cout << format("{} containing num files: {}", BRA_NAME, bh.num_files) << endl;
    for (uint32_t i = 0; i < bh.num_files; i++)
    {
        if (!bra_io_decode_and_write_to_disk(&f))
            return 1;
    }

    bra_io_close(&f);
    return 0;
}
