#include "bra_test.hpp"

#include <lib_bra.h>
#include <io/lib_bra_io_file.h>


///////////////////////////////////////////////////////////////////////////////

char* g_argv0 = nullptr;

TEST(test_lib_bra_can_be_sfx)
{
#if defined(__linux__) || defined(__unix__) || defined(__APPLE__)
    ASSERT_TRUE(bra_io_file_is_elf(g_argv0));
#elif defined(_WIN32) || defined(_WIN64)
    ASSERT_TRUE(bra_io_file_is_pe_exe(g_argv0));
#else
    return 1;    // what is this?
#endif

    ASSERT_TRUE(bra_io_file_can_be_sfx(g_argv0));
    return 0;
}

int main(int argc, char* argv[])
{
    g_argv0 = argv[0];

    ASSERT_TRUE(g_argv0 != nullptr);

    const std::map<std::string, std::function<int()>> m = {
        {TEST_FUNC(test_lib_bra_can_be_sfx)},
    };

    return test_main(argc, argv, m);
}
