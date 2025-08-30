#include "bra_test.hpp"
#include <bra_fs.hpp>

using namespace std;

int test_bra_fs_wildcards()
{
    ASSERT_TRUE(bra_fs_isWildcard("*"));
    ASSERT_TRUE(bra_fs_isWildcard(".*"));
    ASSERT_TRUE(bra_fs_isWildcard("AbcD?"));

    ASSERT_TRUE(!bra_fs_isWildcard(""));
    ASSERT_TRUE(!bra_fs_isWildcard("1234"));

    return 0;
}

int main(int argc, char* argv[])
{
    return test_main(argc, argv, {
                                     {TEST_FUNC(test_bra_fs_wildcards)},
                                 });
}
