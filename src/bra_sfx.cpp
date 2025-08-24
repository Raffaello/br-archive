#include <lib_bra.h>
#include <version.h>

#include <iostream>
#include <filesystem>
#include <format>

#include <cstdio>


using namespace std;

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

int help()
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
        cerr << "unsupported file detected" << endl;
        return 1;
    }

    // The idea of the SFX is to have a footer at the end of the file
    // The footer contain the location where the embedded data is
    // so it can be extracted / dumped into a temporary file
    // and decoded


    return 0;
}
