#include "bra_test.hpp"
#include "lib_bra_defs.h"

#include <bra_wildcards.hpp>

#include <regex>

using namespace std;
using namespace bra::wildcards;

namespace fs = std::filesystem;

TEST(test_bra_wildcards_is_wildcard)
{
    ASSERT_TRUE(is_wildcard("*"));
    ASSERT_TRUE(is_wildcard(".*"));
    ASSERT_TRUE(is_wildcard("AbcD?"));
    // ASSERT_TRUE(is_wildcard("t[1-9]")); // TODO

    ASSERT_TRUE(!is_wildcard(""));
    ASSERT_TRUE(!is_wildcard("1234"));
    // ASSERT_TRUE(!is_wildcard("t]1-9["));

    return 0;
}

TEST(test_bra_wildcards_extract_dir)
{
    fs::path wildcard;

    wildcard = "*";
    ASSERT_EQ(wildcard_extract_dir(wildcard).string(), string("./"));
    ASSERT_EQ(wildcard.string(), "*");

    wildcard = "dir/*";
    ASSERT_EQ(wildcard_extract_dir(wildcard).string(), string("dir/"));
    ASSERT_EQ(wildcard.string(), "*");

    // wildcard = "dir\\*";
    // ASSERT_TRUE(try_sanitize(wildcard));
    // ASSERT_EQ(wildcard_extract_dir(wildcard).string(), string("dir/"));
    // ASSERT_EQ(wildcard.string(), "*");

    // not a wild card, in this case this function shouldn't be called.
    wildcard = "dir/a";
    ASSERT_EQ(wildcard_extract_dir(wildcard).string(), string("dir/"));
    ASSERT_EQ(wildcard.string(), "");

    // wildcard = "dir\\a";
    // ASSERT_EQ(wildcard_extract_dir(wildcard).string(), string("dir/"));
    // ASSERT_EQ(wildcard.string(), "");

    wildcard = "no_dir_no_wildcard";
    ASSERT_EQ(wildcard_extract_dir(wildcard).string(), string("./"));
    ASSERT_EQ(wildcard.string(), "");

    wildcard = "dir/file?.txt";
    ASSERT_EQ(wildcard_extract_dir(wildcard).string(), string("dir/"));
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

TEST(test_bra_wildcards_to_regexp)
{
    const std::regex r(wildcard_to_regexp("file+name?.txt"));
    ASSERT_TRUE(std::regex_match("file+name1.txt", r));
    ASSERT_TRUE(!std::regex_match("filename1.txt", r));    // '+' must be literal

    return 0;
}

int main(int argc, char* argv[])
{
    return test_main(argc, argv, {
                                     {TEST_FUNC(test_bra_wildcards_is_wildcard)},

                                     {TEST_FUNC(test_bra_wildcards_extract_dir)},
                                     {TEST_FUNC(test_bra_wildcards_to_regexp)},
                                 });
}
