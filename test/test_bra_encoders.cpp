#include "bra_test.hpp"

#ifdef __cplusplus
extern "C" {
#endif

#include <encoders/bra_rle.h>
#include <encoders/bra_bwt.h>
#include <encoders/bra_mtf.h>

#ifdef __cplusplus
}
#endif

#include <fs/bra_fs.hpp>

#include <cstring>

///////////////////////////////////////////////////////////////////////////////

TEST(test_bra_encoders_encode_decode_rle_1)
{
    constexpr const int buf_size = 10;

    uint8_t          buf[buf_size];
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
    uint8_t          buf2[buf_size];
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

    uint8_t          buf[buf_size];
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
    uint8_t          buf2[buf_size];
    size_t           buf_i    = 0;
    bra_rle_chunk_t* rle_head = rle_list;

    ASSERT_TRUE(bra_decode_rle(&rle_list, buf2, buf_size, &buf_i));
    ASSERT_EQ(buf_i, 10U);
    for (size_t i = 0; i < buf_i; i++)
        ASSERT_EQ(buf2[i], buf[i]);

    ASSERT_TRUE(rle_list == nullptr);
    ASSERT_FALSE(bra_decode_rle(&rle_list, buf2, buf_size, &buf_i));

    ASSERT_TRUE(bra_encode_rle_free_list(&rle_head));
    return 0;
}

TEST(test_bra_encoders_encode_decode_rle_3)
{
    constexpr const int buf_size = 5;

    uint8_t          buf[buf_size];
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
    uint8_t          buf2[buf_size];
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
    ASSERT_TRUE(rle_list == nullptr);
    ASSERT_FALSE(bra_decode_rle(&rle_list, buf2, buf_size, &buf_i));

    ASSERT_TRUE(bra_encode_rle_free_list(&rle_head));
    return 0;
}

TEST(test_bra_encoders_encode_decode_rle_3b)
{
    constexpr const int buf_size = 5;

    uint8_t          buf[buf_size];
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
    uint8_t          buf2[buf_size];
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
    ASSERT_TRUE(rle_list == nullptr);
    ASSERT_FALSE(bra_decode_rle(&rle_list, buf2, buf_size, &buf_i));

    ASSERT_TRUE(bra_encode_rle_free_list(&rle_head));
    return 0;
}

static int _test_bra_encoders_encode_decode_bwt(const uint8_t* buf, const size_t buf_size, const uint8_t* exp_buf, const size_t exp_primary_index)
{
    size_t   primary_index;
    uint8_t* out_buf = bra_bwt_encode(buf, buf_size, &primary_index);
    ASSERT_TRUE(out_buf != nullptr);
    ASSERT_EQ(primary_index, exp_primary_index);
    ASSERT_EQ(memcmp(out_buf, exp_buf, buf_size), 0);

    uint8_t* out_buf2 = bra_bwt_decode(out_buf, buf_size, primary_index);
    ASSERT_TRUE(out_buf2 != nullptr);
    ASSERT_EQ(memcmp(out_buf2, buf, buf_size), 0);

    if (out_buf != nullptr)
        free(out_buf);
    if (out_buf2 != nullptr)
        free(out_buf2);

    return 0;
}

TEST(test_bra_encoders_encode_decode_bwt_1)
{
    const uint8_t* buf      = (const uint8_t*) "BANANA";
    const uint8_t* exp_buf  = (const uint8_t*) "NNBAAA";
    const size_t   buf_size = 6;

    return _test_bra_encoders_encode_decode_bwt(buf, buf_size, exp_buf, 3U);
}

TEST(test_bra_encoders_encode_decode_bwt_2)
{
    const uint8_t* buf      = (const uint8_t*) "The quick brown fox jumps over the lazy dog.";
    const uint8_t* exp_buf  = (const uint8_t*) "kynxeserg.l i hhv otTu c uwd rfm ebp qjoooza";
    const size_t   buf_size = strlen((const char*) buf);

    return _test_bra_encoders_encode_decode_bwt(buf, buf_size, exp_buf, 9U);
}

TEST(test_bra_encoders_encode_decode_mtf_1)
{
    const uint8_t* buf       = (const uint8_t*) "BANANA";
    const uint8_t  exp_buf[] = {'B', 'B', 'N', 1, 1, 1};
    const size_t   buf_size  = strlen((const char*) buf);

    uint8_t* out_buf = bra_mtf_encode(buf, buf_size);
    ASSERT_TRUE(out_buf != nullptr);
    ASSERT_EQ(memcmp(out_buf, exp_buf, buf_size), 0);

    uint8_t* out_buf2 = bra_mtf_decode(out_buf, buf_size);
    ASSERT_TRUE(out_buf2 != nullptr);
    ASSERT_EQ(memcmp(out_buf2, buf, buf_size), 0);

    return 0;
}

int main(int argc, char* argv[])
{
    const std::map<std::string, std::function<int()>> m = {
        {TEST_FUNC(test_bra_encoders_encode_decode_rle_1)},
        {TEST_FUNC(test_bra_encoders_encode_decode_rle_2)},
        {TEST_FUNC(test_bra_encoders_encode_decode_rle_3)},
        {TEST_FUNC(test_bra_encoders_encode_decode_rle_3b)},

        {TEST_FUNC(test_bra_encoders_encode_decode_bwt_1)},
        {TEST_FUNC(test_bra_encoders_encode_decode_bwt_2)},

        {TEST_FUNC(test_bra_encoders_encode_decode_mtf_1)},

    };

    return test_main(argc, argv, m);
}
