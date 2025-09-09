#include "bra_test.hpp"

// #include <encoders/bra_rle.h>
#include <lib_bra.h>

///////////////////////////////////////////////////////////////////////////////

TEST(test_bra_encoders_encode_rle_1)
{
    constexpr const int buf_size = 10;

    char             buf[buf_size];
    uint64_t         num_rle_chunks = 0;
    bra_rle_chunk_t* rle_data       = nullptr;

    for (int i = 0; i < buf_size; ++i)
        buf[i] = 'A';

    ASSERT_TRUE(bra_encode_rle(buf, buf_size, &num_rle_chunks, &rle_data));
    ASSERT_EQ(num_rle_chunks, 1U);
    ASSERT_EQ(rle_data[0].counts, buf_size - 1U);
    ASSERT_EQ(rle_data[0].value, 'A');

    return 0;
}

TEST(test_bra_encoders_encode_rle_2)
{
    constexpr const int buf_size = 10;

    char             buf[buf_size];
    uint64_t         num_rle_chunks = 0;
    bra_rle_chunk_t* rle_data       = nullptr;

    for (int i = 0; i < buf_size / 2; ++i)
        buf[i] = 'A';

    for (int i = 5; i < buf_size; ++i)
        buf[i] = 'B';

    ASSERT_TRUE(bra_encode_rle(buf, buf_size, &num_rle_chunks, &rle_data));
    ASSERT_EQ(num_rle_chunks, 2U);
    ASSERT_EQ(rle_data[0].counts, 5U - 1U);
    ASSERT_EQ(rle_data[0].value, 'A');
    ASSERT_EQ(rle_data[1].counts, 5U - 1U);
    ASSERT_EQ(rle_data[1].value, 'B');

    return 0;
}

TEST(test_bra_encoders_encode_rle_3)
{
    constexpr const int buf_size = 5;

    char             buf[buf_size];
    uint64_t         num_rle_chunks = 0;
    bra_rle_chunk_t* rle_data       = nullptr;

    for (int i = 0; i < buf_size; ++i)
        buf[i] = 'A';

    ASSERT_TRUE(bra_encode_rle(buf, buf_size, &num_rle_chunks, &rle_data));
    ASSERT_EQ(num_rle_chunks, 1U);
    ASSERT_EQ(rle_data[0].counts, 5U - 1U);
    ASSERT_EQ(rle_data[0].value, 'A');

    for (int i = 0; i < buf_size; ++i)
        buf[i] = 'B';

    ASSERT_TRUE(bra_encode_rle(buf, buf_size, &num_rle_chunks, &rle_data));
    ASSERT_EQ(num_rle_chunks, 2U);
    ASSERT_EQ(rle_data[0].counts, 5U - 1U);
    ASSERT_EQ(rle_data[0].value, 'A');
    ASSERT_EQ(rle_data[1].counts, 5U - 1U);
    ASSERT_EQ(rle_data[1].value, 'B');

    return 0;
}

TEST(test_bra_encoders_encode_rle_3b)
{
    constexpr const int buf_size = 5;

    char             buf[buf_size];
    uint64_t         num_rle_chunks = 0;
    bra_rle_chunk_t* rle_data       = nullptr;

    for (int i = 0; i < buf_size + 1; ++i)
        buf[i] = 'A';

    ASSERT_TRUE(bra_encode_rle(buf, buf_size, &num_rle_chunks, &rle_data));
    ASSERT_EQ(num_rle_chunks, 1U);
    ASSERT_EQ(rle_data[0].counts, 5U - 1U);
    ASSERT_EQ(rle_data[0].value, 'A');

    for (int i = 1; i < buf_size; ++i)
        buf[i] = 'B';

    ASSERT_TRUE(bra_encode_rle(buf, buf_size, &num_rle_chunks, &rle_data));
    ASSERT_EQ(num_rle_chunks, 2U);
    ASSERT_EQ(rle_data[0].counts, 6U - 1U);
    ASSERT_EQ(rle_data[0].value, 'A');
    ASSERT_EQ(rle_data[1].counts, 4U - 1U);
    ASSERT_EQ(rle_data[1].value, 'B');

    return 0;
}

int main(int argc, char* argv[])
{
    const std::map<std::string, std::function<int()>> m = {
        {TEST_FUNC(test_bra_encoders_encode_rle_1)},
        {TEST_FUNC(test_bra_encoders_encode_rle_2)},
        {TEST_FUNC(test_bra_encoders_encode_rle_3)},
        {TEST_FUNC(test_bra_encoders_encode_rle_3b)},
    };

    return test_main(argc, argv, m);
}
