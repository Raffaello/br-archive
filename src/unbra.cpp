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


namespace fs = std::filesystem;

fs::path g_bra_file;

void help()
{
    using namespace std;

    cout << format("BR-Archive Utility Version: {}", VERSION) << endl;
    cout << endl;
    cout << format("Usage:") << endl;
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

int main(int argc, char* argv[])
{
    using namespace std;

    if (argc < 2)
    {
        help();

        return 1;
    }

    for (int i = 1; i < argc; i++)
    {
        string s = argv[i];
        if (s == "--help")
        {
            help();

            return 0;
        }
        // check if it is file
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

            return 1;
        }
    }

    // header
    g_bra_file.replace_extension(BRA_FILE_EXT);
    FILE* f = fopen(g_bra_file.string().c_str(), "rb");
    if (f == nullptr)
    {
        cout << format("unable to read {} BRa file", g_bra_file.string()) << endl;
        return 1;
    }

    bra_header_t bh{};
    if (fread(&bh, sizeof(bra_header_t), 1, f) != 1)
    {
    BRA_IO_READ_ERR:
        cout << format("unable to read {} BRa file", g_bra_file.string()) << endl;
        fclose(f);
        return 1;
    }

    // check header magic
    if (bh.magic != BRA_MAGIC)
    {
        cerr << "Not a valid .BRa file" << endl;
        goto BRA_IO_READ_ERR;
    }

    cout << format("BRa containing num files: {}", bh.num_files) << endl;

    for (uint32_t i = 0; i < bh.num_files; i++)
    {
        constexpr uint32_t MAX_BUF_SIZE = 1024 * 1024;    // 1M

        char buf[MAX_BUF_SIZE];

        // read bra file
        uint8_t fn_size = 0;
        if (fread(&fn_size, sizeof(uint8_t), 1, f) != 1)
            goto BRA_IO_READ_ERR;

        if (fread(buf, sizeof(uint8_t), fn_size, f) != fn_size)
            goto BRA_IO_READ_ERR;

        buf[fn_size]  = '\0';
        string out_fn = buf;

        uintmax_t ds = 0;
        if (fread(&ds, sizeof(uintmax_t), 1, f) != 1)
            goto BRA_IO_READ_ERR;

        cout << format("Extracting file: {} ...", out_fn);
        FILE* f2 = fopen(out_fn.c_str(), "wb");
        if (f2 == nullptr)
        {
            cerr << format("unable to write file: {}", out_fn.c_str()) << endl;
            goto BRA_IO_READ_ERR;
        }

        for (uintmax_t i = 0; i < ds;)
        {
            uint32_t s = std::min(static_cast<uintmax_t>(MAX_BUF_SIZE), ds - i);

            if (fread(buf, sizeof(char), s, f) != s)
            {
                fclose(f2);
                goto BRA_IO_READ_ERR;
            }

            if (fwrite(buf, sizeof(char), s, f2) != s)
            {
                cerr << format("unable to write file: {}", out_fn.c_str()) << endl;
                fclose(f2);
                fclose(f);
                return 1;
            }

            i += s;
        }

        fclose(f2);
    }

    fclose(f);
}
