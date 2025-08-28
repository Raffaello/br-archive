#include <cstdlib>
#include <filesystem>
#include <string>
#include <fstream>
#include <format>
#include <iostream>
#include <cstdio>

#if defined(__unix__) || defined(__APPLE__)
#include <sys/wait.h>
#else
#define WEXITSTATUS(ret) ret
#endif

#include <lib_bra.h>

namespace fs = std::filesystem;

#if defined(__unix__) || defined(__APPLE__)
constexpr const std::string CMD_PREFIX = "./";
#else
constexpr const std::string CMD_PREFIX = "";

#endif

bool AreFilesContentEquals(const std::filesystem::path& file1, const std::filesystem::path& file2)
{
    // Open both files in binary mode
    std::ifstream f1(file1, std::ios::binary);
    std::ifstream f2(file2, std::ios::binary);

    // Check if both files are open
    if (!f1.is_open() || !f2.is_open())
        return false;

    // Compare file sizes first
    const size_t file_size1 = fs::file_size(file1);
    const size_t file_size2 = fs::file_size(file2);
    if (file_size1 != file_size2)
    {
        std::cout << std::format("files are different in size: {} != {}", file_size1, file_size2) << std::endl;
        return false;    // Files are different
    }

    // Compare contents byte by byte
    char ch1, ch2;
    while (f1.get(ch1) && f2.get(ch2))
    {
        if (ch1 != ch2)
        {
            std::cout << std::format("'{}' != '{}'", ch1, ch2) << std::endl;
            return false;    // Files are different
        }
    }

    return true;    // Files are identical
}

/////////////////////////////////////////////////////./////////////////////////

int test_bra_no_output_file()
{
    const std::string bra     = CMD_PREFIX + "bra";
    const std::string in_file = "./test.txt";

    const int ret = system((bra + " " + in_file).c_str());
    if (WEXITSTATUS(ret) != 1)
        return 1;

    return 0;
}

int test_bra_unbra()
{
    const std::string bra      = CMD_PREFIX + "bra";
    const std::string unbra    = CMD_PREFIX + "unbra";
    const std::string in_file  = "./test.txt";
    const std::string out_file = "./test.txt.BRa";
    const std::string exp_file = "./test.txt.exp";

    if (fs::exists(out_file))
        fs::remove(out_file);

    if (fs::exists(exp_file))
        fs::remove(exp_file);

    if (system((bra + " -o " + out_file + " " + in_file).c_str()) != 0)
        return 1;

    if (!fs::exists(out_file))
    {
        std::cerr << std::format("TEST FAILED: missing out_file {}", out_file) << std::endl;
        return 2;
    }

    fs::rename(in_file, exp_file);
    if (fs::exists(in_file))
        return 3;
    if (system((unbra + " " + out_file).c_str()) != 0)
        return 4;

    if (!fs::exists(in_file))
        return 5;

    if (!AreFilesContentEquals(in_file, exp_file))
        return 6;

    return 0;
}

int test_bra_sfx()
{
    const std::string bra          = CMD_PREFIX + "bra --sfx";
    const std::string in_file      = "./test.txt";
    const std::string out_file     = CMD_PREFIX + "test.txt.BRa";
    const std::string out_file_sfx = out_file + BRA_SFX_FILE_EXT;
    const std::string exp_file     = "./test.txt.exp";

    if (fs::exists(out_file_sfx))
        fs::remove(out_file_sfx);

    if (fs::exists(exp_file))
        fs::remove(exp_file);

    if (system((bra + " -o " + out_file + " " + in_file).c_str()) != 0)
        return 1;

    if (!fs::exists(out_file_sfx))
        return 2;

    fs::rename(in_file, exp_file);
    if (fs::exists(in_file))
        return 3;

    if (system((out_file_sfx).c_str()) != 0)
        return 4;

    if (!fs::exists(in_file))
        return 5;

    if (!AreFilesContentEquals(in_file, exp_file))
        return 6;

    return 0;
}

int test_bra_not_more_than_1_same_file()
{
    const std::string bra      = CMD_PREFIX + "bra";
    const std::string unbra    = CMD_PREFIX + "unbra";
    const std::string in_file  = "./test.txt ./test.txt test.txt test.txt";
    const std::string out_file = "./test.txt.BRa";
    const std::string exp_file = "./test.txt.exp";

    if (fs::exists(out_file))
        fs::remove(out_file);

    if (fs::exists(exp_file))
        fs::remove(exp_file);

    if (system((bra + " -o " + out_file + " " + in_file).c_str()) != 0)
        return 1;

    if (!fs::exists(out_file))
        return 2;

    FILE* output = popen((unbra + " -l " + out_file).c_str(), "r");
    if (output == nullptr)
        return 4;

    char        buf[1024];
    char*       line = fgets(buf, sizeof(buf), output);
    std::string str  = line;
    if (!str.ends_with("num files: 1\n"))
    {
        std::cerr << std::format("expected ends with num files: 1\n actual: {}", str) << std::endl;
        return 5;
    }

    pclose(output);

    return 0;
}

int main(int argc, char* argv[])
{
    int ret = 0;

    if (argc >= 2)
    {
        if (std::string(argv[1]) == std::string("test_bra_no_output_file"))
            ret += test_bra_no_output_file();
        else if (std::string(argv[1]) == std::string("test_bra_unbra"))
            ret += test_bra_unbra();
        else if (std::string(argv[1]) == std::string("test_bra_sfx"))
            ret += test_bra_sfx();
        else if (std::string(argv[1]) == std::string("test_bra_not_more_than_1_same_file"))
            ret += test_bra_not_more_than_1_same_file();
    }
    else
    {
        std::cout << "Executing all tests..." << std::endl;
        ret += test_bra_no_output_file();
        ret += test_bra_unbra();
        ret += test_bra_sfx();
        ret += test_bra_not_more_than_1_same_file();
    }

    return ret;
}
