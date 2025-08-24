#include <lib_bra.h>
#include <version.h>

#include <iostream>
#include <filesystem>
#include <format>

#include <cstdio>

/**
 **** @todo THIS FILE IS MOSTLY A DUPLICATION of unbra.cpp
 */


using namespace std;

namespace fs = std::filesystem;

fs::path g_bra_file;

bool isElf(const char* fn)
{
    FILE* f = fopen(fn, "rb");
    if (f == nullptr)
    {
        cerr << format("unable to open file {}", fn) << endl;
        return false;
    }

    // 0x7F,'E','L','F'
    constexpr int MAGIC_SIZE = 4;
    char          magic[MAGIC_SIZE];
    if (fread(magic, sizeof(char), MAGIC_SIZE, f) != MAGIC_SIZE)
    {
        cerr << format("unable to read file {}", fn) << endl;
        return false;
    }

    fclose(f);

    return magic[0] == 0x7F && magic[1] == 'E' && magic[2] == 'L' && magic[3] == 'F';
}

bool isExe(const char* fn)
{
    FILE* f = fopen(fn, "rb");
    if (f == nullptr)
    {
        cerr << format("unable to open file {}", fn) << endl;
        return false;
    }

    // 'M' 'Z'
    constexpr int MAGIC_SIZE = 2;
    char          magic[MAGIC_SIZE];
    if (fread(magic, sizeof(char), MAGIC_SIZE, f) != MAGIC_SIZE)
    {
        cerr << format("unable to read file {}", fn) << endl;
        return false;
    }

    fclose(f);

    return magic[0] = 'M' && magic[1] == 'Z';
}

void help()
{
    cout << format("BR-Archive Utility Version: {}", VERSION) << endl;
    cout << endl;
    // cout << format("Usage:") << endl;
    // cout << endl;
    // cout << format("  unbra (output_directory)") << endl;
    // cout << format("The [output_file(s)] will be as they are stored in the archive") << endl;
    // cout << format("Example:") << endl;
    // cout << format("  unbra test.BRa") << endl;
    // cout << endl;
    // cout << format("(input_file) : (single file only for now) the output") << endl;
    // cout << format("(output_file): output file name without extension") << endl;
    cout << endl;
    cout << format("Options:") << endl;
    cout << format("--help: display this page.") << endl;
    cout << endl;
}

bool parse_args(int argc, char* argv[])
{
    // Supporting only EXE and ELF file type for now
    if (isElf(argv[0]))
    {
        cout << "[debug] ELF file detected" << endl;
    }
    else if (isExe(argv[0]))
    {
        cout << "[debug] EXE file detected" << endl;
    }
    else
    {
        cerr << format("unsupported file detected: {}", argv[0]) << endl;
        return 1;
    }

    for (int i = 1; i < argc; i++)
    {
        string s = argv[i];
        if (s == "--help")
        {
            help();
            exit(0);
        }
        else
        {
            cout << format("unknow argument: {}", s) << endl;
            return false;
        }
    }

    return true;
}

FILE* bra_file_open_and_read_footer_header(const char* fn, bra_header_t* out_bh)
{
    FILE* f = fopen(fn, "rb");
    if (f == nullptr)
    {
        cerr << format("unable to open file {}", fn) << endl;
        return nullptr;
    }

    if (fseek(f, -1L * static_cast<long>(sizeof(bra_footer_t)), SEEK_END) != 0)
    {
    BRA_IO_READ_ERROR:
        cerr << format("unable to read file {}", fn) << endl;
        fclose(f);
        return nullptr;
    }

    bra_footer_t bf{};
    if (fread(&bf, sizeof(bra_footer_t), 1, f) != 1)
        goto BRA_IO_READ_ERROR;

    // check footer magic
    if (bf.magic != BRA_FOOTER_MAGIC)
    {
        cerr << format("corrupted or not valid BRA-SFX file: {}", fn) << endl;
        fclose(f);
        return nullptr;
    }

    // read header and check
    if (fseek(f, bf.data_offset, SEEK_SET) != 0)
    {
    BRA_SFX_IO_READ_ERROR:
        cout << format("unable to read {} {} file", fn, BRA_NAME) << endl;
        fclose(f);
        return nullptr;
    }


    // TODO: refactor in a read header function alone
    // duplicated from unbra.cpp:
    // FILE* bra_file_open_and_read_header(const char* fn, bra_header_t* out_bh)
    if (fread(out_bh, sizeof(bra_header_t), 1, f) != 1)
        goto BRA_SFX_IO_READ_ERROR;

    // check header magic
    if (out_bh->magic != BRA_MAGIC)
    {
        cerr << format("Not a valid {} file", BRA_FILE_EXT) << endl;
        fclose(f);
        return nullptr;
    }

    return f;
}

/**
 * @brief
 * @todo DUPLICATED from unbra.cpp (refactor)
 *
 * @param f
 * @return true
 * @return false
 */
bool bra_file_decode_and_write_to_disk(FILE* f)
{
    char buf[MAX_BUF_SIZE];

    // 1. filename size
    uint8_t fn_size = 0;
    if (fread(&fn_size, sizeof(uint8_t), 1, f) != 1)
    {
    BRA_IO_READ_ERR:
        cout << format("unable to read {} BRa file", g_bra_file.string()) << endl;
        // fclose(f);
        return false;
    }

    // 2. filename
    if (fread(buf, sizeof(uint8_t), fn_size, f) != fn_size)
        goto BRA_IO_READ_ERR;

    buf[fn_size]        = '\0';
    const string out_fn = buf;

    // 3. data size
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

    // 4. read and write in chunk data
    for (uintmax_t i = 0; i < ds;)
    {
        uint32_t s = std::min(static_cast<uintmax_t>(MAX_BUF_SIZE), ds - i);
        if (fread(buf, sizeof(char), s, f) != s)
        {
            cerr << format("unable to decode file: {}", out_fn.c_str()) << endl;
            fclose(f2);
            return false;
        }

        if (fwrite(buf, sizeof(char), s, f2) != s)
        {
            cerr << format("unable to write file: {}", out_fn.c_str()) << endl;
            fclose(f2);
            // fclose(f);
            return false;
        }

        i += s;
    }

    fclose(f2);
    cout << "OK" << endl;
    return true;
}

int main(int argc, char* argv[])
{
    // TODO: add help section
    // TODO: add output directory where to decode
    // TODO: add test archive?
    // TODO: ask to overwrite files, etc..
    // TODO: all these functionalities are common among the utilities

    // TODO: this utility shares functionalties with 'unbra'

    if (!parse_args(argc, argv))
        return 1;


    // The idea of the SFX is to have a footer at the end of the file
    // The footer contain the location where the embedded data is
    // so it can be extracted / dumped into a temporary file
    // and decoded

    bra_header_t bh;
    FILE*        f = bra_file_open_and_read_footer_header(argv[0], &bh);
    if (f == nullptr)
        return 1;

    // extract payload, encoded data
    // TODO: extract payload and copy into a temp file first?
    if (!bra_file_decode_and_write_to_disk(f))
    {
        cerr << "unable to decode" << endl;
        fclose(f);
        return 1;
    }

    fclose(f);
    return 0;
}
