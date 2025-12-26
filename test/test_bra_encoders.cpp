#include "bra_test.hpp"

#ifdef __cplusplus
extern "C" {
#endif

#include <encoders/bra_rle.h>
#include <encoders/bra_bwt.h>
#include <encoders/bra_mtf.h>
#include <encoders/bra_huffman.h>

#ifdef __cplusplus
}
#endif

#include <fs/bra_fs.hpp>

#include <cstring>
#include <cstdlib>

///////////////////////////////////////////////////////////////////////////////

TEST(test_bra_encoders_encode_decode_rle_1)
{
    constexpr const size_t buf_size = 10;

    uint8_t* out_buf      = nullptr;
    size_t   out_buf_size = 0;
    uint8_t  buf[buf_size];

    for (size_t i = 0; i < buf_size; ++i)
        buf[i] = 'A';

    ASSERT_TRUE(bra_rle_encode(buf, buf_size, &out_buf, &out_buf_size));
    ASSERT_EQ(out_buf_size, 2U);
    ASSERT_EQ(out_buf[0], (uint8_t) (-(buf_size - 1)));
    ASSERT_EQ(out_buf[1], 'A');

    ////// decode //////
    uint8_t* buf2   = NULL;
    size_t   buf2_s = 0;

    ASSERT_TRUE(bra_rle_decode(out_buf, out_buf_size, &buf2, &buf2_s));
    ASSERT_EQ(buf2_s, buf_size);
    ASSERT_EQ(memcmp(buf2, buf, buf_size), 0);

    free(buf2);
    free(out_buf);
    return 0;
}

TEST(test_bra_encoders_encode_decode_rle_2)
{
    constexpr const int buf_size = 10;

    uint8_t* out_buf      = nullptr;
    size_t   out_buf_size = 0;
    uint8_t  buf[buf_size];

    for (int i = 0; i < buf_size / 2; ++i)
        buf[i] = 'A';

    for (int i = 5; i < 8; ++i)
        buf[i] = 'B';

    buf[8] = 'C';
    buf[9] = 'D';

    ASSERT_TRUE(bra_rle_encode(buf, buf_size, &out_buf, &out_buf_size));
    ASSERT_EQ(out_buf_size, 4U + 3U);
    ASSERT_EQ(out_buf[0], (uint8_t) (-(5 - 1)));
    ASSERT_EQ(out_buf[1], 'A');
    ASSERT_EQ(out_buf[2], (uint8_t) (-(3 - 1)));
    ASSERT_EQ(out_buf[3], 'B');
    ASSERT_EQ(out_buf[4], 2 - 1U);
    ASSERT_EQ(out_buf[5], 'C');
    ASSERT_EQ(out_buf[6], 'D');

    ////// decode //////
    uint8_t* buf2   = nullptr;
    size_t   buf2_s = 0;

    ASSERT_TRUE(bra_rle_decode(out_buf, out_buf_size, &buf2, &buf2_s));
    ASSERT_EQ(buf2_s, 10U);
    ASSERT_EQ(memcmp(buf, buf2, buf_size), 0);

    free(buf2);
    free(out_buf);
    return 0;
}

TEST(test_bra_encoders_rle_encode_3)
{
    constexpr size_t buf_size      = 8;
    uint8_t          buf[buf_size] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'};
    uint8_t*         out_buf       = nullptr;
    size_t           out_buf_s     = 0;

    // in this way it should return false, but it is a valid encoding even if it bigger.
    // what it should be detected is if it is better with huffman encoded or not.
    // but that will be done automatically.

    // ASSERT_FALSE(bra_rle_encode(buf, buf_size, &out_buf, &out_buf_s));
    // ASSERT_TRUE(out_buf == nullptr);
    // ASSERT_EQ(out_buf_s, 0U);

    // It should encode anyway the caller should decide if discard it or not when is bigger
    ASSERT_TRUE(bra_rle_encode(buf, buf_size, &out_buf, &out_buf_s));
    ASSERT_EQ(out_buf_s, 9U);
    ASSERT_TRUE(out_buf_s > buf_size);

    free(out_buf);
    return 0;
}

static int _test_bra_encoders_encode_decode_bwt(const uint8_t* buf, const size_t buf_size, const uint8_t* exp_buf, const bra_bwt_index_t exp_primary_index)
{
    bra_bwt_index_t primary_index;
    uint8_t*        out_buf = bra_bwt_encode(buf, buf_size, &primary_index);
    ASSERT_TRUE(out_buf != nullptr);
    ASSERT_EQ(primary_index, exp_primary_index);
    ASSERT_EQ(memcmp(out_buf, exp_buf, buf_size), 0);

    uint8_t* out_buf2 = bra_bwt_decode(out_buf, buf_size, primary_index);
    ASSERT_TRUE(out_buf2 != nullptr);
    ASSERT_EQ(memcmp(out_buf2, buf, buf_size), 0);

    free(out_buf);
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

    free(out_buf2);
    free(out_buf);

    return 0;
}

TEST(test_bra_encoders_encode_decode_bwt_mtf_1)
{
    const uint8_t* buf      = (const uint8_t*) "BANANA";
    const size_t   buf_size = strlen((const char*) buf);

    bra_bwt_index_t primary_index;
    uint8_t*        out_buf = bra_bwt_encode(buf, buf_size, &primary_index);
    ASSERT_TRUE(out_buf != nullptr);

    uint8_t* out_buf2 = bra_mtf_encode(out_buf, buf_size);
    ASSERT_TRUE(out_buf2 != nullptr);

    uint8_t* out_buf4 = bra_mtf_decode(out_buf2, buf_size);
    ASSERT_TRUE(out_buf4 != nullptr);

    uint8_t* out_buf5 = bra_bwt_decode(out_buf4, buf_size, primary_index);
    ASSERT_TRUE(out_buf5 != nullptr);

    ASSERT_TRUE(memcmp(buf, out_buf5, buf_size) == 0);

    free(out_buf5);
    free(out_buf4);
    free(out_buf2);
    free(out_buf);
    return 0;
}

TEST(test_bra_encoders_decode_mtf_bwt_1)
{
    constexpr auto buf_size      = 6;
    uint8_t        encoded_mtf[] = {0x4E, 0x00, 0x43, 0x43, 0x00, 0x00};
    uint8_t        encoded_bwt[] = {0x4E, 0x4E, 0x42, 0x41, 0x41, 0x41};

    uint8_t* out_buf = bra_mtf_decode(encoded_mtf, buf_size);
    ASSERT_TRUE(out_buf != nullptr);
    ASSERT_EQ(memcmp(out_buf, encoded_bwt, buf_size), 0);

    uint8_t* out_buf2 = bra_bwt_decode(out_buf, buf_size, 3);
    ASSERT_TRUE(out_buf2 != nullptr);

    ASSERT_EQ(memcmp(out_buf2, "BANANA", buf_size), 0);

    free(out_buf2);
    free(out_buf);

    return 0;
}

TEST(test_bra_encoders_encode_decode_bwt_mtf_rle_1)
{
    const uint8_t* buf      = (const uint8_t*) "BANANA";
    const size_t   buf_size = strlen((const char*) buf);

    bra_bwt_index_t primary_index;
    uint8_t*        out_buf = bra_bwt_encode(buf, buf_size, &primary_index);
    ASSERT_TRUE(out_buf != nullptr);

    uint8_t* out_buf2 = bra_mtf_encode(out_buf, buf_size);
    ASSERT_TRUE(out_buf2 != nullptr);


    uint8_t* buf3   = nullptr;
    size_t   buf3_s = 0;
    ASSERT_TRUE(bra_rle_encode(out_buf2, buf_size, &buf3, &buf3_s));

    ////// decode //////
    uint8_t* buf4   = nullptr;
    size_t   buf4_s = 0;
    ASSERT_TRUE(bra_rle_decode(buf3, buf3_s, &buf4, &buf4_s));
    ASSERT_EQ(buf_size, buf4_s);
    ASSERT_EQ(memcmp(buf4, out_buf2, buf_size), 0);
    free(buf3);

    uint8_t* out_buf4 = bra_mtf_decode(buf4, buf_size);
    ASSERT_TRUE(out_buf4 != nullptr);
    ASSERT_EQ(memcmp(out_buf4, out_buf, buf_size), 0);
    free(buf4);

    uint8_t* out_buf5 = bra_bwt_decode(out_buf4, buf_size, primary_index);
    ASSERT_TRUE(out_buf5 != nullptr);
    ASSERT_EQ(memcmp(out_buf5, buf, buf_size), 0);

    free(out_buf5);
    free(out_buf4);
    free(out_buf2);
    free(out_buf);

    return 0;
}

TEST(test_bra_encoders_encode_decode_huffman_1)
{
    const uint8_t* buf      = (const uint8_t*) "BANANA";
    const size_t   buf_size = 6;

    bra_huffman_chunk_t* huffman = bra_huffman_encode(buf, buf_size);
    ASSERT_TRUE(huffman != nullptr);

    ASSERT_EQ(huffman->meta.orig_size, 6U);
    ASSERT_EQ(huffman->meta.encoded_size, 2U);
    ASSERT_EQ(huffman->meta.lengths[0], 0U);
    ASSERT_EQ(huffman->meta.lengths['B'], 2U);    // least repeated symbol
    ASSERT_EQ(huffman->meta.lengths['A'], 1U);    // most repeated symbol
    ASSERT_EQ(huffman->meta.lengths['N'], 2U);
    ASSERT_EQ(huffman->data[0], 155);
    ASSERT_EQ(huffman->data[1], 0);    // only 1 bit is used here, other are just padding

    uint32_t out_size;
    uint8_t* out_buf = bra_huffman_decode(&huffman->meta, huffman->data, &out_size);
    ASSERT_TRUE(out_buf != nullptr);
    ASSERT_EQ(out_size, buf_size);
    ASSERT_EQ(memcmp(buf, out_buf, buf_size), 0);

    bra_huffman_chunk_free(huffman);
    free(out_buf);
    return 0;
}

TEST(test_bra_encoders_encode_decode_huffman_2)
{
    const uint8_t* buf      = (const uint8_t*) "AAAAAAAA";
    const size_t   buf_size = 8;

    // first time just encode 'AAAAA'
    bra_huffman_chunk_t* huffman = bra_huffman_encode(buf, 5);
    ASSERT_TRUE(huffman != nullptr);

    ASSERT_EQ(huffman->meta.orig_size, 5U);
    ASSERT_EQ(huffman->meta.encoded_size, 1U);
    ASSERT_EQ(huffman->meta.lengths[0], 0U);
    ASSERT_EQ(huffman->meta.lengths['A'], 1U);    // 1 because the most repeated symbol
    ASSERT_EQ(huffman->data[0], 0);

    uint32_t out_size;
    uint8_t* out_buf = bra_huffman_decode(&huffman->meta, huffman->data, &out_size);
    ASSERT_TRUE(out_buf != nullptr);
    ASSERT_EQ(out_size, 5U);
    ASSERT_EQ(memcmp(buf, out_buf, 5), 0);

    bra_huffman_chunk_free(huffman);
    free(out_buf);

    // now the whole buffer
    huffman = bra_huffman_encode(buf, buf_size);
    ASSERT_TRUE(huffman != nullptr);

    ASSERT_EQ(huffman->meta.orig_size, buf_size);
    ASSERT_EQ(huffman->meta.encoded_size, 1U);
    ASSERT_EQ(huffman->meta.lengths[0], 0U);
    ASSERT_EQ(huffman->meta.lengths['A'], 1U);    // 1 because the most repeated symbol
    ASSERT_EQ(huffman->data[0], 0);

    // uint32_t out_size;
    out_buf = bra_huffman_decode(&huffman->meta, huffman->data, &out_size);
    ASSERT_TRUE(out_buf != nullptr);
    ASSERT_EQ(out_size, buf_size);
    ASSERT_EQ(memcmp(buf, out_buf, buf_size), 0);

    bra_huffman_chunk_free(huffman);
    free(out_buf);
    return 0;
}

TEST(test_bra_encoders_encode_decode_huffman_3)
{
    constexpr uint32_t buf_size      = 6;
    uint8_t            encoded_mtf[] = {0x4E, 0x00, 0x43, 0x43, 0x00, 0x00};

    bra_huffman_chunk_t* huffman = bra_huffman_encode(encoded_mtf, buf_size);
    ASSERT_TRUE(huffman != nullptr);
    ASSERT_TRUE(huffman->meta.lengths[0] > 0);
    ASSERT_EQ(huffman->meta.orig_size, buf_size);
    ASSERT_EQ(huffman->meta.encoded_size, 2U);

    uint32_t out_size = 0;
    uint8_t* out_buf  = bra_huffman_decode(&huffman->meta, huffman->data, &out_size);
    ASSERT_TRUE(out_buf != nullptr);
    ASSERT_EQ(out_size, buf_size);
    ASSERT_EQ(memcmp(out_buf, encoded_mtf, buf_size), 0);

    bra_huffman_chunk_free(huffman);
    free(out_buf);

    return 0;
}

TEST(test_bra_encoders_encode_decode_huffman_4)
{
    uint8_t              buf[1]  = {0};
    bra_huffman_chunk_t* huffman = bra_huffman_encode(buf, 0);
    ASSERT_TRUE(huffman == nullptr);

    return 0;
}

TEST(test_bra_encoders_encode_decode_bwt_mtf_huffman_1)
{
    const uint8_t* buf      = (const uint8_t*) "BANANA";
    const size_t   buf_size = strlen((const char*) buf);

    bra_bwt_index_t primary_index;
    uint8_t*        out_buf = bra_bwt_encode(buf, buf_size, &primary_index);
    ASSERT_TRUE(out_buf != nullptr);

    uint8_t* out_buf2 = bra_mtf_encode(out_buf, buf_size);
    ASSERT_TRUE(out_buf2 != nullptr);

    bra_huffman_chunk_t* huffman = bra_huffman_encode(out_buf2, buf_size);
    ASSERT_TRUE(huffman != nullptr);

    uint32_t out_size = 0;
    uint8_t* out_buf3 = bra_huffman_decode(&huffman->meta, huffman->data, &out_size);
    ASSERT_TRUE(out_buf3 != nullptr);
    ASSERT_EQ(out_size, buf_size);

    uint8_t* out_buf4 = bra_mtf_decode(out_buf3, out_size);
    ASSERT_TRUE(out_buf4 != nullptr);

    uint8_t* out_buf5 = bra_bwt_decode(out_buf4, out_size, primary_index);
    ASSERT_TRUE(out_buf5 != nullptr);

    ASSERT_TRUE(memcmp(buf, out_buf5, out_size) == 0);

    free(out_buf5);
    free(out_buf4);
    free(out_buf3);
    bra_huffman_chunk_free(huffman);
    free(out_buf2);
    free(out_buf);
    return 0;
}

int main(int argc, char* argv[])
{
    const std::map<std::string, std::function<int()>> m = {
        {TEST_FUNC(test_bra_encoders_encode_decode_rle_1)},
        {TEST_FUNC(test_bra_encoders_encode_decode_rle_2)},

        {TEST_FUNC(test_bra_encoders_rle_encode_3)},

        {TEST_FUNC(test_bra_encoders_encode_decode_bwt_1)},
        {TEST_FUNC(test_bra_encoders_encode_decode_bwt_2)},

        {TEST_FUNC(test_bra_encoders_encode_decode_mtf_1)},

        {TEST_FUNC(test_bra_encoders_encode_decode_bwt_mtf_1)},

        {TEST_FUNC(test_bra_encoders_decode_mtf_bwt_1)},

        {TEST_FUNC(test_bra_encoders_encode_decode_bwt_mtf_rle_1)},

        {TEST_FUNC(test_bra_encoders_encode_decode_huffman_1)},
        {TEST_FUNC(test_bra_encoders_encode_decode_huffman_2)},
        {TEST_FUNC(test_bra_encoders_encode_decode_huffman_3)},
        {TEST_FUNC(test_bra_encoders_encode_decode_huffman_4)},

        {TEST_FUNC(test_bra_encoders_encode_decode_bwt_mtf_huffman_1)},
    };

    return test_main(argc, argv, m);
}
