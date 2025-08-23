#include <format>
#include <iostream>
#include <filesystem>
#include <string>
#include <list>

#include <version.h>

namespace fs = std::filesystem;

std::list<std::string> g_files;    // it is just 1 for now but...

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

    for (const auto& f : g_files)
    {
        cout << format("Archiving File: {}...", f) << endl;
    }

    if (g_sfx)
    {
        cout << "SFX NOT IMPLEMENTED YET" << endl;

        return 3;
    }

    return 0;
}
