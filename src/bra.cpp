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


std::list<fs::path> g_files;    // it is just 1 for now but...
// std::string         g_out_filename; // TODO

bool g_sfx = false;

void help()
{
    cout << format("BR-Archive Utility Version: {}", VERSION) << endl;
    cout << endl;
    cout << format("Usage:") << endl;
    cout << format("  bra (input_file)") << endl;
    cout << format("The [output_file] will be with a .BRa extension") << endl;
    cout << format("Example:") << endl;
    cout << format("  bra test.txt") << endl;
    cout << endl;
    cout << format("(input_file) : (single file only for now) the output") << endl;
    // cout << format("(output_file): output file name without extension") << endl;
    cout << endl;
    cout << format("Options:") << endl;
    cout << format("--help: display this page.") << endl;
    cout << format("--sfx: generate a self-extracting archive") << endl;
    cout << endl;
}

bool parse_args(int argc, char* argv[])
{
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
        else if (s == "--sfx")
        {
            g_sfx = true;
        }
        // check if it is file
        else if (fs::exists(s))
        {
            if (!fs::is_regular_file(s))
            {
                cout << format("{} is not a file!", s) << endl;
                return false;
            }

            g_files.push_back(s);
        }
        else
        {
            cout << format("unknow argument: {}", s) << endl;
            return false;
        }
    }

    if (g_files.empty())
    {
        cerr << "no input file provided" << endl;
        return false;
    }

    if (g_sfx)
    {
        // locate sfx bin
        if (!fs::exists(BRA_SFX_FILENAME))
        {
            cerr << "unable to find BRa-SFX module" << endl;
            return false;
        }
    }

    return true;
}

bool bra_io_copy_file_chunks(FILE* dst, FILE* src, const uintmax_t data_size, const char* dst_fn, const char* src_fn)
{
    char buf[MAX_BUF_SIZE];

    for (uintmax_t i = 0; i < data_size;)
    {
        uint32_t s = std::min(static_cast<uintmax_t>(MAX_BUF_SIZE), data_size - i);

        // read source chunk
        if (fread(buf, sizeof(char), s, src) != s)
        {
            cout << format("unable to read {} file", src_fn) << endl;
            // fclose(src);
            return false;
        }

        // write source chunk
        if (fwrite(buf, sizeof(char), s, dst) != s)
        {
            // fclose(dst);
            cerr << format("error writing file: {}", dst_fn) << endl;
            return false;
        }

        i += s;
    }

    return true;
}

FILE* bra_file_open_and_write_header(const char* fn)
{
    FILE* f = fopen(fn, "wb");
    if (f == nullptr)
    {
        cout << format("unable to write {} {} file", fn, BRA_NAME) << endl;
        return nullptr;
    }

    bra_header_t header = {
        .magic     = BRA_MAGIC,
        .num_files = static_cast<uint32_t>(g_files.size()),
    };

    if (fwrite(&header, sizeof(bra_header_t), 1, f) != 1)
    {
        cout << format("unable to write {} {} file", fn, BRA_NAME) << endl;
        fclose(f);
        return nullptr;
    }

    return f;
}

bool bra_file_encode_and_write_to_disk(FILE* f, const string& fn)
{
    cout << format("Archiving File: {}...", fn);
    // 1. file name length
    if (fn.size() > std::numeric_limits<uint8_t>::max())
    {
        cerr << std::format("filename too long: {}", fn) << endl;
        return false;
    }

    const uint8_t fn_size = static_cast<uint8_t>(fn.size());
    if (fwrite(&fn_size, sizeof(uint8_t), 1, f) != 1)
    {
    BRA_IO_ENCODE_WRITE_ERR:
        cerr << format("error writing file: {}", fn) << endl;
        return false;
    }

    // 2. file name
    if (fwrite(fn.c_str(), sizeof(char), fn_size, f) != fn_size)
        goto BRA_IO_ENCODE_WRITE_ERR;

    // 3. data size
    const uintmax_t ds = fs::file_size(fn);
    if (fwrite(&ds, sizeof(uintmax_t), 1, f) != 1)
        goto BRA_IO_ENCODE_WRITE_ERR;

    // 4. data
    FILE* f2 = fopen(fn.c_str(), "rb");
    if (f2 == nullptr)
    {
        cerr << format("unable to open file: {}", fn) << endl;
        return false;
    }

    // TODO: dst file name is wrong
    if (!bra_io_copy_file_chunks(f, f2, ds, "", fn.c_str()))
    {
        fclose(f2);
        return false;
    }

    fclose(f2);
    cout << "OK" << endl;
    return true;
}

int main(int argc, char* argv[])
{
    if (!parse_args(argc, argv))
        return 1;

    // header
    fs::path p = g_files.front();


    // adjust input file extension
    if (p.extension() != BRA_FILE_EXT)
        p += BRA_FILE_EXT;

    string out_fn = p.generic_string();    // TODO: add output file without extension
    FILE*  f      = bra_file_open_and_write_header(out_fn.c_str());
    if (f == nullptr)
        return 1;

    for (const auto& fn_ : g_files)
    {
        const string fn = fs::relative(fn_).generic_string();
        if (!bra_file_encode_and_write_to_disk(f, fn))
        {
            fclose(f);
            return 1;
        }
    }

    fclose(f);

    if (g_sfx)
    {
        // file out_fn to be
        fs::path sfx_path  = p;
        sfx_path          += BRA_SFX_FILE_EXT;

        cout << format("creating Self-extracting archive {}...", sfx_path.string());

        // TODO: ask to overwrite instead
        if (!fs::copy_file(BRA_SFX_FILENAME, sfx_path, fs::copy_options::overwrite_existing))
        {
        BRA_SFX_IO_ERROR:
            cerr << "unable to create a BRa-SFX file" << endl;
            return 2;
        }

        FILE* f = fopen(sfx_path.string().c_str(), "rb+");
        if (f == nullptr)
            goto BRA_SFX_IO_ERROR;

        if (fseek(f, 0, SEEK_END) != 0)
        {
        BRA_SFX_IO_F_ERROR:
            fclose(f);
            goto BRA_SFX_IO_ERROR;
        }

        // save the start of the payload for later...
        const long data_offset = ftell(f);
        if (data_offset == -1L)
            goto BRA_SFX_IO_ERROR;

        // append bra file
        FILE* f2 = fopen(out_fn.c_str(), "rb");
        if (f2 == nullptr)
            goto BRA_SFX_IO_F_ERROR;

        bool res = bra_io_copy_file_chunks(f, f2, fs::file_size(out_fn), sfx_path.string().c_str(), out_fn.c_str());
        fclose(f2);
        if (!res)
            goto BRA_SFX_IO_F_ERROR;

        // write footer
        bra_footer_t bf = {
            .magic       = BRA_FOOTER_MAGIC,
            .data_offset = data_offset,
        };

        res = fwrite(&bf, sizeof(bra_footer_t), 1, f) == 1;
        fclose(f);
        if (!res)
            goto BRA_SFX_IO_ERROR;

        cout << "OK" << endl;
    }

    return 0;
}
