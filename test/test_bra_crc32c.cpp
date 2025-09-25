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

    uint32_t crc = bra_crc32c(data1, 5, BRA_CRC32C_INIT);    // "12345"
    crc          = bra_crc32c(&data1[5], 4, crc);            // "6789"
    ASSERT_EQ(crc, 0xE3069283);

    return 0;
}

TEST(test_bra_crc32c_empty_input)
{
    ASSERT_EQ(bra_crc32c("", 0, BRA_CRC32C_INIT), BRA_CRC32C_INIT);
    ASSERT_EQ(bra_crc32c(nullptr, 0, BRA_CRC32C_INIT), BRA_CRC32C_INIT);
    return 0;
}

TEST(test_bra_crc32c_sse42_compute_crc32)
{
    // CRC‑32C("123456789") = 0xE3069283.
    constexpr char data1[] = "123456789";

    ASSERT_EQ(bra_crc32c_sse42(data1, sizeof(data1) - 1, 0), 0xE3069283);

    uint32_t crc = bra_crc32c_sse42(data1, 5, BRA_CRC32C_INIT);    // "12345"
    crc          = bra_crc32c_sse42(&data1[5], 4, crc);            // "6789"
    ASSERT_EQ(crc, 0xE3069283);

    return 0;
}

TEST(test_bra_crc32c_sse42_empty_input)
{
    ASSERT_EQ(bra_crc32c_sse42("", 0, BRA_CRC32C_INIT), BRA_CRC32C_INIT);
    ASSERT_EQ(bra_crc32c_sse42(nullptr, 0, BRA_CRC32C_INIT), BRA_CRC32C_INIT);
    return 0;
}

TEST(test_bra_crc32c_consistency)
{
    // Test that both implementations produce the same results
    constexpr char test_data[] = "The quick brown fox jumps over the lazy dog";
    const uint64_t test_len    = sizeof(test_data) - 1;

    uint32_t crc_table = bra_crc32c(test_data, test_len, BRA_CRC32C_INIT);
    uint32_t crc_sse42 = bra_crc32c_sse42(test_data, test_len, BRA_CRC32C_INIT);

    ASSERT_EQ(crc_table, crc_sse42);

    // Test with various lengths to ensure byte-level processing works
    for (uint64_t i = 1; i <= test_len; ++i)
    {
        crc_table = bra_crc32c(test_data, i, BRA_CRC32C_INIT);
        crc_sse42 = bra_crc32c_sse42(test_data, i, BRA_CRC32C_INIT);
        ASSERT_EQ(crc_table, crc_sse42);
    }

    return 0;
}

int main(int argc, char* argv[])
{
    const std::map<std::string, std::function<int()>> m = {
        {TEST_FUNC(test_bra_crc32c_compute_crc32)},
        {TEST_FUNC(test_bra_crc32c_empty_input)},
        {TEST_FUNC(test_bra_crc32c_sse42_compute_crc32)},
        {TEST_FUNC(test_bra_crc32c_sse42_empty_input)},
        {TEST_FUNC(test_bra_crc32c_consistency)},
    };

    return test_main(argc, argv, m);
}
