#include "bra_test.hpp"

#include <bra_fs.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <cstdio>


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

///////////////////////////////////////////////////////////////////////////////

TEST(test_bra_help_ret_code)
{
    const std::string bra = CMD_PREFIX + "bra";

    ASSERT_EQ(call_system(bra), 1);
    ASSERT_EQ(call_system(bra + " --help"), 0);
    ASSERT_EQ(call_system(bra + " -h"), 0);

    return 0;
}

TEST(test_bra_no_output_file)
{
    const std::string bra     = CMD_PREFIX + "bra";
    const std::string in_file = "./test.txt";

    ASSERT_EQ(call_system(bra + " " + in_file), 1);

    return 0;
}

TEST(test_bra_unbra)
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

    ASSERT_EQ(call_system(bra + " -o " + out_file + " " + in_file), 0);
    ASSERT_TRUE(fs::exists(out_file));

    fs::rename(in_file, exp_file);
    ASSERT_TRUE(!fs::exists(in_file));
    ASSERT_EQ(call_system(unbra + " " + out_file), 0);
    ASSERT_TRUE(fs::exists(in_file));
    ASSERT_TRUE(AreFilesContentEquals(in_file, exp_file));

    return 0;
}

int _test_bra_unbra_list(const fs::path& input)
{
    const std::string bra      = CMD_PREFIX + "bra";
    const std::string unbra    = CMD_PREFIX + "unbra";
    const std::string out_file = "./dir.BRa";

    if (fs::exists(out_file))
        fs::remove(out_file);

    ASSERT_EQ(call_system(bra + " -o " + out_file + " " + input.string()), 0);
    ASSERT_TRUE(fs::exists(out_file));
    ASSERT_EQ(call_system(unbra + " -l " + out_file), 0);

    return 0;
}

TEST(test_bra_wildcard_dir_unbra_list)
{
    ASSERT_EQ(_test_bra_unbra_list("dir1/*"), 0);
    ASSERT_EQ(_test_bra_unbra_list("dir1"), 0);
    ASSERT_EQ(_test_bra_unbra_list("./dir1/*"), 0);
    ASSERT_EQ(_test_bra_unbra_list("./dir1"), 0);

#if defined(__APPLE__) || defined(__linux__) || defined(__linux)
    ASSERT_EQ(_test_bra_unbra_list("./dir\\?"), 1);    // disabling wildcard expansion
    ASSERT_EQ(_test_bra_unbra_list("./dir?"), 0);
#else
    ASSERT_EQ(_test_bra_unbra_list("./dir?"), 1);    // dir1 won't be added as it is not recursive so no input file added
#endif
    return 0;
}

int _test_bra_sfx(const std::string& out_file)
{
    const std::string bra       = CMD_PREFIX + "bra --sfx --yes ";
    const std::string in_file   = "./test.txt";
    const std::string out_file_ = CMD_PREFIX + out_file;
    const std::string exp_file  = "./test.txt.exp";
    std::string       out_file_sfx;

    out_file_sfx = bra::fs::filename_sfx_adjust(out_file_, false).string();

    std::cout << std::format("out_file_sfx: {}", out_file_sfx) << std::endl;
    if (fs::exists(out_file_sfx))
        fs::remove(out_file_sfx);

    if (fs::exists(exp_file))
        fs::remove(exp_file);

    ASSERT_EQ(call_system(bra + " -o " + out_file_ + " " + in_file), 0);
    ASSERT_TRUE(fs::exists(out_file_sfx));

    fs::rename(in_file, exp_file);
    ASSERT_TRUE(!fs::exists(in_file));
    ASSERT_EQ(call_system(out_file_sfx), 0);
    ASSERT_TRUE(fs::exists(in_file));
    ASSERT_TRUE(AreFilesContentEquals(in_file, exp_file));

    return 0;
}

TEST(test_bra_sfx_0)
{
    return _test_bra_sfx("test.txt.BRa");
}

TEST(test_bra_sfx_1)
{
    return _test_bra_sfx("test.txt");
}

TEST(test_bra_sfx_2)
{
    return _test_bra_sfx("test");
}

TEST(test_bra_not_more_than_1_same_file)
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

    ASSERT_EQ(call_system(bra + " -o " + out_file + " " + in_file), 0);
    ASSERT_TRUE(fs::exists(out_file));

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
    const std::map<std::string, std::function<int()>> m = {
        {TEST_FUNC(test_bra_help_ret_code)},
        {TEST_FUNC(test_bra_no_output_file)},
        {TEST_FUNC(test_bra_unbra)},
        {TEST_FUNC(test_bra_wildcard_dir_unbra_list)},
        {TEST_FUNC(test_bra_sfx_0)},
        {TEST_FUNC(test_bra_sfx_1)},
        {TEST_FUNC(test_bra_sfx_2)},
        {TEST_FUNC(test_bra_not_more_than_1_same_file)},
    };

    return test_main(argc, argv, m);
}
