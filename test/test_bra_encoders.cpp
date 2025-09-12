#include "bra_test.hpp"

#include <encoders/bra_rle.h>

#include <fs/bra_fs.hpp>

///////////////////////////////////////////////////////////////////////////////

TEST(test_bra_encoders_encode_decode_rle_1)
{
    constexpr const int buf_size = 10;

    char             buf[buf_size];
    uint64_t         num_rle_chunks = 0;
    bra_rle_chunk_t* rle_list       = nullptr;

    for (int i = 0; i < buf_size; ++i)
        buf[i] = 'A';

    ASSERT_TRUE(bra_encode_rle(buf, buf_size, &num_rle_chunks, &rle_list));
    ASSERT_EQ(num_rle_chunks, 1U);
    ASSERT_EQ(rle_list->counts, buf_size - 1U);
    ASSERT_EQ(rle_list->value, 'A');
    ASSERT_TRUE(rle_list->pNext == nullptr);

    ////// decode //////
    char             buf2[buf_size];
    size_t           buf_i    = 0;
    bra_rle_chunk_t* rle_head = rle_list;

    ASSERT_TRUE(bra_decode_rle(&rle_list, buf2, buf_size, &buf_i));
    ASSERT_EQ(buf_i, 10U);
    for (size_t i = 0; i < buf_i; i++)
        ASSERT_EQ(buf2[i], 'A');
    ASSERT_TRUE(rle_list == nullptr);
    ASSERT_FALSE(bra_decode_rle(&rle_list, buf2, buf_size, &buf_i));

    ASSERT_TRUE(bra_encode_rle_free_list(&rle_head));
    return 0;
}

TEST(test_bra_encoders_encode_decode_rle_2)
{
    constexpr const int buf_size = 10;

    char             buf[buf_size];
    uint64_t         num_rle_chunks = 0;
    bra_rle_chunk_t* rle_list       = nullptr;

    for (int i = 0; i < buf_size / 2; ++i)
        buf[i] = 'A';

    for (int i = 5; i < buf_size; ++i)
        buf[i] = 'B';

    ASSERT_TRUE(bra_encode_rle(buf, buf_size, &num_rle_chunks, &rle_list));
    ASSERT_EQ(num_rle_chunks, 2U);
    ASSERT_EQ(rle_list->counts, 5U - 1U);
    ASSERT_EQ(rle_list->value, 'A');
    ASSERT_EQ(rle_list->pNext->counts, 5U - 1U);
    ASSERT_EQ(rle_list->pNext->value, 'B');
    ASSERT_TRUE(rle_list->pNext->pNext == nullptr);

    ////// decode //////
    char             buf2[buf_size];
    size_t           buf_i    = 0;
    bra_rle_chunk_t* rle_head = rle_list;

    ASSERT_TRUE(bra_decode_rle(&rle_list, buf2, buf_size, &buf_i));
    ASSERT_EQ(buf_i, 10U);
    for (size_t i = 0; i < buf_i; i++)
        ASSERT_EQ(buf2[i], buf[i]);
    ASSERT_FALSE(bra_decode_rle(&rle_list, buf2, buf_size, &buf_i));

    ASSERT_TRUE(bra_encode_rle_free_list(&rle_head));
    return 0;
}

TEST(test_bra_encoders_encode_decode_rle_3)
{
    constexpr const int buf_size = 5;

    char             buf[buf_size];
    uint64_t         num_rle_chunks = 0;
    bra_rle_chunk_t* rle_list       = nullptr;

    for (int i = 0; i < buf_size; ++i)
        buf[i] = 'A';

    ASSERT_TRUE(bra_encode_rle(buf, buf_size, &num_rle_chunks, &rle_list));
    ASSERT_EQ(num_rle_chunks, 1U);
    ASSERT_EQ(rle_list->counts, 5U - 1U);
    ASSERT_EQ(rle_list->value, 'A');
    ASSERT_TRUE(rle_list->pNext == nullptr);


    for (int i = 0; i < buf_size; ++i)
        buf[i] = 'B';

    ASSERT_TRUE(bra_encode_rle(buf, buf_size, &num_rle_chunks, &rle_list));
    ASSERT_EQ(num_rle_chunks, 2U);
    ASSERT_EQ(rle_list->counts, 5U - 1U);
    ASSERT_EQ(rle_list->value, 'A');
    ASSERT_EQ(rle_list->pNext->counts, 5U - 1U);
    ASSERT_EQ(rle_list->pNext->value, 'B');
    ASSERT_TRUE(rle_list->pNext->pNext == nullptr);

    ////// decode //////
    char             buf2[buf_size];
    size_t           buf_i    = 0;
    bra_rle_chunk_t* rle_head = rle_list;

    ASSERT_TRUE(bra_decode_rle(&rle_list, buf2, buf_size, &buf_i));
    ASSERT_EQ(buf_i, 5U);
    for (size_t i = 0; i < buf_i; i++)
        ASSERT_EQ(buf2[i], 'A');


    buf_i = 0;

    ASSERT_TRUE(bra_decode_rle(&rle_list, buf2, buf_size, &buf_i));
    ASSERT_EQ(buf_i, 5U);
    for (size_t i = 0; i < buf_i; i++)
        ASSERT_EQ(buf2[i], 'B');

    buf_i = 0;
    ASSERT_FALSE(bra_decode_rle(&rle_list, buf2, buf_size, &buf_i));

    ASSERT_TRUE(bra_encode_rle_free_list(&rle_head));
    return 0;
}

TEST(test_bra_encoders_encode_decode_rle_3b)
{
    constexpr const int buf_size = 5;

    char             buf[buf_size];
    uint64_t         num_rle_chunks = 0;
    bra_rle_chunk_t* rle_list       = nullptr;

    for (int i = 0; i < buf_size; ++i)
        buf[i] = 'A';

    ASSERT_TRUE(bra_encode_rle(buf, buf_size, &num_rle_chunks, &rle_list));
    ASSERT_EQ(num_rle_chunks, 1U);
    ASSERT_EQ(rle_list->counts, 5U - 1U);
    ASSERT_EQ(rle_list->value, 'A');
    ASSERT_TRUE(rle_list->pNext == nullptr);

    for (int i = 1; i < buf_size; ++i)
        buf[i] = 'B';

    ASSERT_TRUE(bra_encode_rle(buf, buf_size, &num_rle_chunks, &rle_list));
    ASSERT_EQ(num_rle_chunks, 2U);
    ASSERT_EQ(rle_list->counts, 6U - 1U);
    ASSERT_EQ(rle_list->value, 'A');
    ASSERT_EQ(rle_list->pNext->counts, 4U - 1U);
    ASSERT_EQ(rle_list->pNext->value, 'B');
    ASSERT_TRUE(rle_list->pNext->pNext == nullptr);

    ////// decode //////
    char             buf2[buf_size];
    size_t           buf_i    = 0;
    bra_rle_chunk_t* rle_head = rle_list;

    ASSERT_TRUE(bra_decode_rle(&rle_list, buf2, buf_size, &buf_i));
    ASSERT_EQ(buf_i, 5U);
    for (size_t i = 0; i < buf_i; i++)
        ASSERT_EQ(buf2[i], 'A');

    buf_i = 0;
    ASSERT_TRUE(bra_decode_rle(&rle_list, buf2, buf_size, &buf_i));
    ASSERT_EQ(buf_i, 5U);
    ASSERT_EQ(buf2[0], 'A');
    for (size_t i = 1; i < buf_i; i++)
        ASSERT_EQ(buf2[i], 'B');

    buf_i = 0;
    ASSERT_FALSE(bra_decode_rle(&rle_list, buf2, buf_size, &buf_i));

    ASSERT_TRUE(bra_encode_rle_free_list(&rle_head));
    return 0;
}

int main(int argc, char* argv[])
{
    const std::map<std::string, std::function<int()>> m = {
        {TEST_FUNC(test_bra_encoders_encode_decode_rle_1)},
        {TEST_FUNC(test_bra_encoders_encode_decode_rle_2)},
        {TEST_FUNC(test_bra_encoders_encode_decode_rle_3)},
        {TEST_FUNC(test_bra_encoders_encode_decode_rle_3b)},
    };

    return test_main(argc, argv, m);
}
