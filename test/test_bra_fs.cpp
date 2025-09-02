#include "bra_test.hpp"
#include "lib_bra_defs.h"

#include <bra_fs.hpp>

using namespace std;


namespace fs = std::filesystem;

int test_bra_fs_is_wildcards()
{
    PRINT_TEST_NAME;

    ASSERT_TRUE(bra::fs::isWildcard("*"));
    ASSERT_TRUE(bra::fs::isWildcard(".*"));
    ASSERT_TRUE(bra::fs::isWildcard("AbcD?"));
    // ASSERT_TRUE(isWildcard("t[1-9]")); // TODO

    ASSERT_TRUE(!bra::fs::isWildcard(""));
    ASSERT_TRUE(!bra::fs::isWildcard("1234"));
    // ASSERT_TRUE(!isWildcard("t]1-9["));

    return 0;
}

int test_bra_fs_dir_make()
{
    PRINT_TEST_NAME;

    const fs::path dir1 = "dir1/test";

    fs::remove(dir1);
    ASSERT_TRUE(!bra::fs::dir_exists(dir1))

    ASSERT_TRUE(bra::fs::dir_make(dir1));
    ASSERT_TRUE(bra::fs::dir_exists(dir1))
    ASSERT_TRUE(bra::fs::dir_make(dir1));
    ASSERT_TRUE(bra::fs::dir_exists(dir1))

    fs::remove(dir1);
    ASSERT_TRUE(!bra::fs::dir_exists(dir1))


    return 0;
}

int test_bra_fs_try_sanitize_path()
{
    PRINT_TEST_NAME;

    fs::path p;

    p = "c:\\not_sane";
    ASSERT_TRUE(!bra::fs::try_sanitize(p));
    p = "..\\not_sane";
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

int main(int argc, char* argv[])
{
    return test_main(argc, argv, {
                                     {TEST_FUNC(test_bra_fs_is_wildcards)},
                                     {TEST_FUNC(test_bra_fs_dir_make)},
                                     {TEST_FUNC(test_bra_fs_try_sanitize_path)},
                                     {TEST_FUNC(test_bra_fs_sfx_filename_adjust)},
                                     {TEST_FUNC(test_bra_fs_wildcard_extract_dir)},
                                 });
}
