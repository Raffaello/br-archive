#include <format>
#include <iostream>
#include <filesystem>
#include <string>

#include <version.h>

namespace fs = std::filesystem;

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

    if (argc < 3)
    {
        help();
        exit(1);
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
            cout << "SFX NOT IMPLEMENTED YET" << endl;
            exit(0);
        }
        // check if it is file
        else if (fs::exists(s))
        {
            if (!fs::is_regular_file(s))
            {
                cout << format("s is not a file!") << endl;
                exit(2);
            }
        }
        else
        {
            cout << format("unknow argument: {}", s) << endl;
            exit(1);
        }
    }


    return 0;
}
