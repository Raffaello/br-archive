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
    constexpr const int buf_size = 10;

    uint8_t* out_buf      = nullptr;
    size_t   out_buf_size = 0;
    uint8_t  buf[buf_size];
    // uint64_t num_rle_chunks = 0;
    // bra_rle_chunk_t* rle_list       = nullptr;

    for (int i = 0; i < buf_size; ++i)
        buf[i] = 'A';

    ASSERT_TRUE(bra_rle_encode(buf, buf_size, &out_buf, &out_buf_size));
    ASSERT_EQ(out_buf_size, 2U);
    ASSERT_EQ(out_buf[0], (uint8_t) (-(buf_size - 1)));
    ASSERT_EQ(out_buf[1], 'A');

    // ASSERT_TRUE(bra_encode_rle(buf, buf_size, &num_rle_chunks, &rle_list));
    // ASSERT_EQ(num_rle_chunks, 1U);
    // ASSERT_EQ(rle_list->counts, buf_size - 1U);
    // ASSERT_EQ(rle_list->value, 'A');
    // ASSERT_TRUE(rle_list->pNext == nullptr);

    ////// decode //////
    // uint8_t          buf2[buf_size];
    // size_t           buf_i    = 0;
    // bra_rle_chunk_t* rle_head = rle_list;

    // ASSERT_TRUE(bra_decode_rle(&rle_list, buf2, buf_size, &buf_i));
    // ASSERT_EQ(buf_i, 10U);
    // for (size_t i = 0; i < buf_i; i++)
    //     ASSERT_EQ(buf2[i], 'A');

    // ASSERT_TRUE(rle_list == nullptr);
    // ASSERT_FALSE(bra_decode_rle(&rle_list, buf2, buf_size, &buf_i));

    // ASSERT_TRUE(bra_encode_rle_free_list(&rle_head));
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
    const uint8_t* buf = (const uint8_t*) "BANANA";
    // const uint8_t  exp_buf[] = {'B', 'B', 'N', 1, 1, 1};
    const size_t buf_size = strlen((const char*) buf);

    bra_bwt_index_t primary_index;
    uint8_t*        out_buf = bra_bwt_encode(buf, buf_size, &primary_index);
    ASSERT_TRUE(out_buf != nullptr);

    uint8_t* out_buf2 = bra_mtf_encode(out_buf, buf_size);
    ASSERT_TRUE(out_buf2 != nullptr);


    uint8_t*         buf3           = (uint8_t*) calloc(1, buf_size);
    uint64_t         num_rle_chunks = 0;
    bra_rle_chunk_t* rle_list       = nullptr;
    bra_rle_chunk_t* rle_head       = nullptr;
    size_t           buf_i          = 0;

    ASSERT_TRUE(buf3 != nullptr);
    ASSERT_TRUE(bra_encode_rle(out_buf2, buf_size, &num_rle_chunks, &rle_head));
    rle_list = rle_head;

    ////// decode //////
    ASSERT_TRUE(bra_decode_rle(&rle_list, buf3, buf_size, &buf_i));
    ASSERT_EQ(buf_i, buf_size);
    ASSERT_EQ(memcmp(buf3, out_buf2, buf_size), 0);

    uint8_t* out_buf4 = bra_mtf_decode(buf3, buf_size);
    ASSERT_TRUE(out_buf4 != nullptr);
    ASSERT_EQ(memcmp(out_buf4, out_buf, buf_size), 0);

    uint8_t* out_buf5 = bra_bwt_decode(out_buf4, buf_size, primary_index);
    ASSERT_TRUE(out_buf5 != nullptr);
    ASSERT_EQ(memcmp(out_buf5, buf, buf_size), 0);

    ASSERT_TRUE(bra_encode_rle_free_list(&rle_head));

    free(out_buf5);
    free(out_buf4);
    free(buf3);
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
        {TEST_FUNC(test_bra_encoders_encode_decode_rle_3)},
        {TEST_FUNC(test_bra_encoders_encode_decode_rle_3b)},

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
