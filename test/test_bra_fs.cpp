#include "bra_test.hpp"
#include "lib_bra_defs.h"

#include <bra_fs.hpp>

#include <regex>

using namespace std;


namespace fs = std::filesystem;

int test_bra_fs_is_wildcards()
{
    PRINT_TEST_NAME;

    ASSERT_TRUE(bra::fs::is_wildcard("*"));
    ASSERT_TRUE(bra::fs::is_wildcard(".*"));
    ASSERT_TRUE(bra::fs::is_wildcard("AbcD?"));
    // ASSERT_TRUE(is_wildcard("t[1-9]")); // TODO

    ASSERT_TRUE(!bra::fs::is_wildcard(""));
    ASSERT_TRUE(!bra::fs::is_wildcard("1234"));
    // ASSERT_TRUE(!is_wildcard("t]1-9["));

    return 0;
}

int test_bra_fs_wildcard_expand()
{
    PRINT_TEST_NAME;

    std::set<fs::path> files;

#if defined(_WIN32)
    constexpr size_t exp_files = 2;    // bra.exe, bra.sfx
#else                                  // linux or whatever else
    constexpr size_t exp_files = 1;    // bra.sfx, bra.exe is just bra
#endif

    ASSERT_TRUE(bra::fs::wildcard_expand("*", files));
    ASSERT_TRUE(files.size() != 0);
    files.clear();

    ASSERT_TRUE(bra::fs::wildcard_expand("./*", files));
    ASSERT_TRUE(files.size() != 0);
    files.clear();

    ASSERT_TRUE(bra::fs::wildcard_expand("bra.*", files));
    ASSERT_EQ(files.size(), exp_files);
    files.clear();

    ASSERT_TRUE(bra::fs::wildcard_expand("./bra.*", files));
    ASSERT_EQ(files.size(), exp_files);
    files.clear();

    ASSERT_TRUE(bra::fs::wildcard_expand("bra*", files));
    ASSERT_EQ(files.size(), 2U);
    files.clear();

    ASSERT_TRUE(bra::fs::wildcard_expand("?ra.?fx", files));
    ASSERT_EQ(files.size(), 1U);
    ASSERT_EQ(files.begin()->string(), "bra.sfx");
    files.clear();

    ASSERT_TRUE(bra::fs::wildcard_expand("dir?", files));
    ASSERT_EQ(files.size(), 0U);    // matches 'dir1' but as it is a dir without recursion is not added.
    // ASSERT_EQ(files.begin()->string(), "dir1");
    files.clear();

    return 0;
}

int test_bra_fs_file_exists()
{
    PRINT_TEST_NAME;

    ASSERT_TRUE(bra::fs::file_exists("test.txt"));
    ASSERT_TRUE(!bra::fs::file_exists("test99.txt"));
    ASSERT_TRUE(!bra::fs::file_exists("dir1"));

    return 0;
}

int test_bra_fs_dir_exists()
{
    PRINT_TEST_NAME;

    ASSERT_TRUE(bra::fs::dir_exists("dir1"));
    ASSERT_TRUE(!bra::fs::dir_exists("dir99"));
    ASSERT_TRUE(!bra::fs::dir_exists("test.txt"));

    return 0;
}

int test_bra_fs_dir_make()
{
    PRINT_TEST_NAME;

    std::optional<bool> b;
    const fs::path      dir1 = "dir1/test";

    fs::remove(dir1);
    b = bra::fs::dir_exists(dir1);
    ASSERT_TRUE(b && !*b);
    ASSERT_TRUE(bra::fs::dir_make(dir1));
    b = bra::fs::dir_exists(dir1);
    ASSERT_TRUE(b && *b);
    ASSERT_TRUE(bra::fs::dir_make(dir1));
    b = bra::fs::dir_exists(dir1);
    ASSERT_TRUE(b && *b);

    fs::remove(dir1);
    b = bra::fs::dir_exists(dir1);
    ASSERT_TRUE(b && !*b);

    return 0;
}

int test_bra_fs_try_sanitize_path()
{
    PRINT_TEST_NAME;

    fs::path p;

#if defined(_WIN32)
    p = "c:\\not_sane";
    ASSERT_TRUE(!bra::fs::try_sanitize(p));
    p = "..\\not_sane";
    ASSERT_TRUE(!bra::fs::try_sanitize(p));
#endif
    p = fs::current_path().parent_path();
    ASSERT_TRUE(!bra::fs::try_sanitize(p));
    p = fs::current_path() / "..";
    ASSERT_TRUE(!bra::fs::try_sanitize(p));

    p = fs::current_path();
    ASSERT_TRUE(bra::fs::try_sanitize(p));
    ASSERT_EQ(p.string(), ".");

    p = fs::current_path() / "test.txt";
    ASSERT_TRUE(bra::fs::try_sanitize(p));
    ASSERT_EQ(p.string(), "test.txt");

    p = fs::current_path() / "not_existing_dir" / ".." / "test.txt";
    ASSERT_TRUE(bra::fs::try_sanitize(p));
    ASSERT_EQ(p.string(), "test.txt");

    p = "./wildcards/*";
    ASSERT_TRUE(bra::fs::try_sanitize(p));
    ASSERT_EQ(p.string(), "wildcards/*");

    p = "./*";
    ASSERT_TRUE(bra::fs::try_sanitize(p));
    ASSERT_EQ(p.string(), "*");

    p = "*";
    ASSERT_TRUE(bra::fs::try_sanitize(p));
    ASSERT_EQ(p.string(), "*");

    p = "./bra.*";
    ASSERT_TRUE(bra::fs::try_sanitize(p));
    ASSERT_EQ(p.string(), "bra.*");

    p = "bra.*";
    ASSERT_TRUE(bra::fs::try_sanitize(p));
    ASSERT_EQ(p.string(), "bra.*");


    p = "dir?";
    ASSERT_TRUE(bra::fs::try_sanitize(p));
    ASSERT_EQ(p.string(), "dir?");

    return 0;
}

int test_bra_fs_sfx_filename_adjust()
{
    PRINT_TEST_NAME;

    auto exp = [](const string& s, const bool tmp) {
        return format("{}{}{}", s, BRA_FILE_EXT, tmp ? BRA_SFX_TMP_FILE_EXT : BRA_SFX_FILE_EXT);
    };

    bool tmp = false;
    ASSERT_EQ(bra::fs::filename_sfx_adjust("test", tmp).string(), exp("test", tmp));
    ASSERT_EQ(bra::fs::filename_sfx_adjust(string("test") + BRA_FILE_EXT, tmp).string(), exp("test", tmp));
    ASSERT_EQ(bra::fs::filename_sfx_adjust(string("test") + BRA_FILE_EXT + BRA_SFX_FILE_EXT, tmp).string(), exp("test", tmp));

    tmp = true;
    ASSERT_EQ(bra::fs::filename_sfx_adjust("test", tmp).string(), exp("test", tmp));
    ASSERT_EQ(bra::fs::filename_sfx_adjust(string("test") + BRA_FILE_EXT, tmp).string(), exp("test", tmp));
    ASSERT_EQ(bra::fs::filename_sfx_adjust(string("test") + BRA_FILE_EXT + BRA_SFX_FILE_EXT, tmp).string(), exp("test", tmp));

    return 0;
}

int test_bra_fs_wildcard_extract_dir()
{
    PRINT_TEST_NAME;

    fs::path wildcard;

    wildcard = "*";
    ASSERT_EQ(bra::fs::wildcard_extract_dir(wildcard).string(), string("./"));
    ASSERT_EQ(wildcard.string(), "*");

    wildcard = "dir/*";
    ASSERT_EQ(bra::fs::wildcard_extract_dir(wildcard).string(), string("dir/"));
    ASSERT_EQ(wildcard.string(), "*");

    // wildcard = "dir\\*";
    // ASSERT_TRUE(try_sanitize(wildcard));
    // ASSERT_EQ(wildcard_extract_dir(wildcard).string(), string("dir/"));
    // ASSERT_EQ(wildcard.string(), "*");

    // not a wild card, in this case this function shouldn't be called.
    wildcard = "dir/a";
    ASSERT_EQ(bra::fs::wildcard_extract_dir(wildcard).string(), string("dir/"));
    ASSERT_EQ(wildcard.string(), "");

    // wildcard = "dir\\a";
    // ASSERT_EQ(wildcard_extract_dir(wildcard).string(), string("dir/"));
    // ASSERT_EQ(wildcard.string(), "");

    wildcard = "no_dir_no_wildcard";
    ASSERT_EQ(bra::fs::wildcard_extract_dir(wildcard).string(), string("./"));
    ASSERT_EQ(wildcard.string(), "");

    wildcard = "dir/file?.txt";
    ASSERT_EQ(bra::fs::wildcard_extract_dir(wildcard).string(), string("dir/"));
    ASSERT_EQ(wildcard.string(), "file?.txt");

    // TODO: to be implemented as it is required a dir struct.
    // wildcard = "d?r/*";
    // ASSERT_EQ(wildcard_extract_dir(wildcard).string(), string("d?r/"));
    // ASSERT_EQ(wildcard, "*");

    // wildcard = "dir\\*";
    // ASSERT_EQ(wildcard_extract_dir(wildcard).string(), string("dir/"));
    // ASSERT_EQ(wildcard, "*");

    // wildcard = "dir/a";
    // ASSERT_EQ(wildcard_extract_dir(wildcard).string(), string("dir/"));
    // ASSERT_EQ(wildcard, "");

    // wildcard = "dir\\a";
    // ASSERT_EQ(wildcard_extract_dir(wildcard).string(), string("dir/"));
    // ASSERT_EQ(wildcard, "");

    return 0;
}

int test_bra_fs_wildcard_to_regexp()
{
    PRINT_TEST_NAME;

    const std::regex r(bra::fs::wildcard_to_regexp("file+name?.txt"));
    ASSERT_TRUE(std::regex_match("file+name1.txt", r));
    ASSERT_TRUE(!std::regex_match("filename1.txt", r));    // '+' must be literal
    return 0;
}

int main(int argc, char* argv[])
{
    return test_main(argc, argv, {
                                     {TEST_FUNC(test_bra_fs_is_wildcards)},
                                     {TEST_FUNC(test_bra_fs_wildcard_expand)},
                                     {TEST_FUNC(test_bra_fs_file_exists)},
                                     {TEST_FUNC(test_bra_fs_dir_exists)},
                                     {TEST_FUNC(test_bra_fs_dir_make)},
                                     {TEST_FUNC(test_bra_fs_try_sanitize_path)},
                                     {TEST_FUNC(test_bra_fs_sfx_filename_adjust)},
                                     {TEST_FUNC(test_bra_fs_wildcard_extract_dir)},
                                     {TEST_FUNC(test_bra_fs_wildcard_to_regexp)},
                                 });
}
