#include "bra_test.hpp"
#include "lib_bra_defs.h"

#include <bra_fs.hpp>

using namespace std;

int test_bra_fs_wildcards()
{
    PRINT_TEST_NAME;

    ASSERT_TRUE(bra_fs_isWildcard("*"));
    ASSERT_TRUE(bra_fs_isWildcard(".*"));
    ASSERT_TRUE(bra_fs_isWildcard("AbcD?"));

    ASSERT_TRUE(!bra_fs_isWildcard(""));
    ASSERT_TRUE(!bra_fs_isWildcard("1234"));

    return 0;
}

int test_bra_fs_sfx_filename_adjust()
{
    PRINT_TEST_NAME;

    auto exp = [](const string& s, const bool tmp) {
        return format("{}{}{}", s, BRA_FILE_EXT, tmp ? BRA_SFX_TMP_FILE_EXT : BRA_SFX_FILE_EXT);
    };

    bool tmp = false;
    ASSERT_EQ(bra_fs_filename_sfx_adjust("test", tmp).string(), exp("test", tmp));
    ASSERT_EQ(bra_fs_filename_sfx_adjust(string("test") + BRA_FILE_EXT, tmp).string(), exp("test", tmp));
    ASSERT_EQ(bra_fs_filename_sfx_adjust(string("test") + BRA_FILE_EXT + BRA_SFX_FILE_EXT, tmp).string(), exp("test", tmp));

    tmp = true;
    ASSERT_EQ(bra_fs_filename_sfx_adjust("test", tmp).string(), exp("test", tmp));
    ASSERT_EQ(bra_fs_filename_sfx_adjust(string("test") + BRA_FILE_EXT, tmp).string(), exp("test", tmp));
    ASSERT_EQ(bra_fs_filename_sfx_adjust(string("test") + BRA_FILE_EXT + BRA_SFX_FILE_EXT, tmp).string(), exp("test", tmp));

    return 0;
}

int main(int argc, char* argv[])
{
    return test_main(argc, argv, {
                                     {TEST_FUNC(test_bra_fs_wildcards)},
                                     {TEST_FUNC(test_bra_fs_sfx_filename_adjust)},
                                 });
}
