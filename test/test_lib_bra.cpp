#include "bra_test.hpp"

#include <lib_bra.h>

///////////////////////////////////////////////////////////////////////////////

TEST(test_lib_bra_none)
{
    return 0;
}

int main(int argc, char* argv[])
{
    const std::map<std::string, std::function<int()>> m = {
        {TEST_FUNC(test_lib_bra_none)},
    };

    return test_main(argc, argv, m);
}
