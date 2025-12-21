#include "bra_test.hpp"

#ifdef __cplusplus
extern "C" {

#include <utils/lib_bra_crc32c.h>
}
#endif

#include <cstdio>


///////////////////////////////////////////////////////////////////////////////

constexpr char     data1[]   = "123456789";
constexpr size_t   data1_len = sizeof(data1) - 1;    // Exclude null terminator
constexpr uint32_t exp_crc1  = 0xE3069283;           // Precomputed CRC32C for "123456789"
constexpr char     data2[]   = "Hello World!";
constexpr size_t   data2_len = sizeof(data2) - 1;    // Exclude null terminator
constexpr uint32_t exp_crc2  = 0xFE6CF1DC;           // Precomputed CRC32C for "Hello World!"

TEST(test_bra_crc32c_compute_crc32)
{
    ASSERT_EQ(bra_crc32c(data1, data1_len, 0), exp_crc1);

    uint32_t crc = bra_crc32c(data1, 5, BRA_CRC32C_INIT);    // "12345"
    crc          = bra_crc32c(&data1[5], 4, crc);            // "6789"
    ASSERT_EQ(crc, exp_crc1);

    return 0;
}

TEST(test_bra_crc32c_empty_input)
{
    ASSERT_EQ(bra_crc32c("", 0, BRA_CRC32C_INIT), BRA_CRC32C_INIT);
    ASSERT_EQ(bra_crc32c(nullptr, 0, BRA_CRC32C_INIT), BRA_CRC32C_INIT);
    return 0;
}

TEST(test_bra_crc32c_table_compute_crc32)
{
    ASSERT_EQ(bra_crc32c_table(data1, data1_len, 0), exp_crc1);

    uint32_t crc = bra_crc32c_table(data1, 5, BRA_CRC32C_INIT);    // "12345"
    crc          = bra_crc32c_table(&data1[5], 4, crc);            // "6789"
    ASSERT_EQ(crc, exp_crc1);

    ASSERT_EQ(bra_crc32c_table("", 0, BRA_CRC32C_INIT), BRA_CRC32C_INIT);
    ASSERT_EQ(bra_crc32c_table(nullptr, 0, BRA_CRC32C_INIT), BRA_CRC32C_INIT);

    ASSERT_EQ(bra_crc32c_table(data2, data2_len, BRA_CRC32C_INIT), exp_crc2);

    return 0;
}

TEST(test_bra_crc32c_sse42_compute_crc32)
{
    ASSERT_EQ(bra_crc32c_sse42(data1, data1_len, 0), exp_crc1);

    uint32_t crc = bra_crc32c_sse42(data1, 5, BRA_CRC32C_INIT);    // "12345"
    crc          = bra_crc32c_sse42(&data1[5], 4, crc);            // "6789"
    ASSERT_EQ(crc, exp_crc1);

    ASSERT_EQ(bra_crc32c_sse42(data2, data2_len, BRA_CRC32C_INIT), exp_crc2);

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

    uint32_t crc_table = bra_crc32c_table(test_data, test_len, BRA_CRC32C_INIT);
    uint32_t crc_sse42 = bra_crc32c_sse42(test_data, test_len, BRA_CRC32C_INIT);

    ASSERT_EQ(crc_table, crc_sse42);

    // Test with various lengths to ensure byte-level processing works
    for (uint64_t i = 1; i <= test_len; ++i)
    {
        crc_table = bra_crc32c_table(test_data, i, BRA_CRC32C_INIT);
        crc_sse42 = bra_crc32c_sse42(test_data, i, BRA_CRC32C_INIT);
        ASSERT_EQ(crc_table, crc_sse42);
    }

    return 0;
}

TEST(test_bra_crc32c_combine)
{
    const uint32_t crc1     = bra_crc32c(data2, 6, BRA_CRC32C_INIT);
    const uint32_t crc2     = bra_crc32c(&data2[6], 6, BRA_CRC32C_INIT);
    const uint32_t crc      = bra_crc32c(data2, 12, BRA_CRC32C_INIT);
    const uint32_t crc_comb = bra_crc32c_combine(crc1, crc2, 6);

    ASSERT_EQ(crc, crc_comb);
    return 0;
}

TEST(test_bra_crc32c_combine2)
{
    FILE* f = fopen("fixtures/lorem.txt", "rb");
    ASSERT_TRUE(f != nullptr);

    constexpr const int buf_size  = 3048;
    constexpr const int buf_split = 50;
    char                buf[buf_size];
    ASSERT_EQ(fread(buf, buf_size, 1, f), 1U);
    fclose(f);

    const uint32_t crc      = bra_crc32c(buf, buf_size, BRA_CRC32C_INIT);
    const uint32_t crc1     = bra_crc32c(buf, buf_split, BRA_CRC32C_INIT);
    const uint32_t crc2     = bra_crc32c(&buf[buf_split], buf_size - buf_split, BRA_CRC32C_INIT);
    const uint32_t crc_comb = bra_crc32c_combine(crc1, crc2, buf_size - buf_split);

    ASSERT_EQ(crc, crc_comb);

    return 0;
}

int main(int argc, char* argv[])
{
    const std::map<std::string, std::function<int()>> m = {
        {TEST_FUNC(test_bra_crc32c_compute_crc32)},
        {TEST_FUNC(test_bra_crc32c_empty_input)},
        {TEST_FUNC(test_bra_crc32c_table_compute_crc32)},
        {TEST_FUNC(test_bra_crc32c_sse42_compute_crc32)},
        {TEST_FUNC(test_bra_crc32c_sse42_empty_input)},
        {TEST_FUNC(test_bra_crc32c_consistency)},
        {TEST_FUNC(test_bra_crc32c_combine)},
        {TEST_FUNC(test_bra_crc32c_combine2)},
    };

    return test_main(argc, argv, m);
}
