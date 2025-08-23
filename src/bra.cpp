#include <lib_bra.h>

#include <version.h>

#include <format>
#include <iostream>
#include <filesystem>
#include <string>
#include <list>

namespace fs = std::filesystem;


std::list<fs::path> g_files;    // it is just 1 for now but...

bool g_sfx = false;

void help()
{
    using namespace std;

    cout << format("BR-Archive Utility Version: {}", VERSION) << endl;
    cout << endl;
    cout << format("Usage:") << endl;
    cout << format("  bra (input_file)") << endl;
    cout << format("The [output_file] generate will be with a .BRa extension") << endl;
    cout << format("Example:") << endl;
    cout << format("  bra test.txt") << endl;
    cout << endl;
    cout << format("(input_file): (single file only for now) the output") << endl;
    cout << endl;
    cout << format("Options:") << endl;
    cout << format("--help: display this page.") << endl;
    cout << format("--sfx: generate a self-extracting archive") << endl;
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
        else if (s == "--sfx")
        {
            g_sfx = true;
        }
        // check if it is file
        else if (fs::exists(s))
        {
            if (!fs::is_regular_file(s))
            {
                cout << format("s is not a file!") << endl;

                return 2;
            }

            g_files.push_back(s);
        }
        else
        {
            cout << format("unknow argument: {}", s) << endl;

            return 1;
        }
    }

    if (g_files.size() == 0)
    {
        cerr << "[debug] (it shouldn't be here...)" << endl;
        help();

        return 1;
    }


    // header
    string out_fn = "test.bra";    // TODO: add output file without extension
    FILE*  f      = fopen(out_fn.c_str(), "wb");
    if (f == NULL)
    {
        cout << format("unable to write {} BRa file", out_fn) << endl;
        return 1;
    }

    bra_header_t header = {
        .magic     = BRA_MAGIC,
        .num_files = (uint32_t) g_files.size(),
    };

    if (fwrite(&header, sizeof(bra_header_t), 1, f) != 1)
    {
    BRA_IO_WRITE_ERR:
        cout << format("unable to write {} BRa file", out_fn) << endl;
        fclose(f);
        return 1;
    }

    for (const auto& fn_ : g_files)
    {
        const string fn = fn_.generic_string();

        cout << format("Archiving File: {}...", fn);
        // 1. file name length
        const uint8_t fn_size = fn.size();
        if (fwrite(&fn_size, sizeof(uint8_t), 1, f) != 1)
            goto BRA_IO_WRITE_ERR;
        // 2. file name
        if (fwrite(fn.c_str(), sizeof(char) * fn_size, 1, f) != 1)
            goto BRA_IO_WRITE_ERR;
        // 3. data size
        const uintmax_t ds = fs::file_size(fn);
        if (fwrite(&ds, sizeof(uintmax_t), 1, f) != 1)
            goto BRA_IO_WRITE_ERR;
        // 4. data
        FILE* f2 = fopen(fn.c_str(), "rb");
        if (f2 == NULL)
            goto BRA_IO_WRITE_ERR;

        constexpr uint32_t MAX_BUF_SIZE = 1024 * 1024;
        uint8_t            buf[MAX_BUF_SIZE];    // 1M
        for (uintmax_t i = 0; i < ds;)
        {
            uint32_t s = std::min(static_cast<uintmax_t>(MAX_BUF_SIZE), ds - i);

            // read source chunk
            if (fread(buf, sizeof(uint8_t) * s, 1, f2) != 1)
            {
            BRA_IO_WRITE_ERR_READ:
                cout << format("unable to read {} file", fn) << endl;
                fclose(f2);
                fclose(f);
                return 1;
            }

            // write source chunk
            if (fwrite(buf, sizeof(uint8_t) * s, 1, f) != 1)
            {
                fclose(f2);
                goto BRA_IO_WRITE_ERR;
            }

            i += s;
        }

        fclose(f2);
        cout << "OK" << endl;
    }

    fclose(f);

    if (g_sfx)
    {
        cout << "SFX NOT IMPLEMENTED YET" << endl;

        return 3;
    }

    return 0;
}
