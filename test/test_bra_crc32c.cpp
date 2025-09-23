#include "bra_test.hpp"

#ifdef __cplusplus
extern "C" {

#include <lib_bra_crc32c.h>
}
#endif


///////////////////////////////////////////////////////////////////////////////

TEST(test_bra_crc32c_compute_crc32)
{
    // CRC‑32C("123456789") = 0xE3069283.
    constexpr char data1[] = "123456789";

    ASSERT_EQ(bra_crc32c(data1, sizeof(data1) - 1, 0), 0xE3069283);
    ASSERT_EQ(bra_crc32c("", 0, BRA_CRC32C_INIT), BRA_CRC32C_INIT);

    return 0;
}

int main(int argc, char* argv[])
{
    const std::map<std::string, std::function<int()>> m = {
        {TEST_FUNC(test_bra_crc32c_compute_crc32)},
    };

    return test_main(argc, argv, m);
}
