#include "bra_huffman.h"
#include <lib_bra_defs.h>
#include <log/bra_log.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>

/**
 * @brief bra_huffman_node_t
 */
typedef struct bra_huffman_node_t
{
    uint8_t                    symbol;    //!< symbol
    uint32_t                   freq;      //!< max 4GB for 1 symbol
    struct bra_huffman_node_t* left;      //!< left branch of the tree
    struct bra_huffman_node_t* right;     //!< right branch of the tree
} bra_huffman_node_t;

/**
 * @brief minHeapNode_t
 */
typedef struct minHeapNode_t
{
    bra_huffman_node_t*   node;    //!< node
    struct minHeapNode_t* next;    //!< next
} minHeapNode_t;

///////////////////////////////////////////////////////////////////////////////

static bra_huffman_node_t* bra_huffman_create_node(const uint8_t symbol, const uint32_t freq)
{
    bra_huffman_node_t* node = malloc(sizeof(bra_huffman_node_t));
    if (node == NULL)
    {
        bra_log_error("unable to alloc huffman node");
        return NULL;
    }

    node->symbol = symbol;
    node->freq   = freq;
    node->left = node->right = NULL;

    return node;
}

static void bra_huffman_tree_free(bra_huffman_node_t** node)
{
    assert(node != NULL);

    if (*node == NULL)
        return;

    bra_huffman_tree_free(&(*node)->left);
    bra_huffman_tree_free(&(*node)->right);
    free(*node);
    *node = NULL;
}

static minHeapNode_t* bra_minHeapNode_create(bra_huffman_node_t* node)
{
    assert(node != NULL);

    minHeapNode_t* mhn = malloc(sizeof(minHeapNode_t));
    if (mhn == NULL)
        return NULL;

    mhn->node = node;
    mhn->next = NULL;
    return mhn;
}

static bra_huffman_node_t* bra_minHeap_extractMin(minHeapNode_t** head)
{
    assert(head != NULL);

    if (*head == NULL)
    {
        bra_log_error("minHeap head is null");
        return NULL;
    }

    minHeapNode_t* tmp       = *head;
    *head                    = (*head)->next;
    bra_huffman_node_t* node = tmp->node;
    free(tmp);
    return node;
}

static bool bra_minHeap_insert(minHeapNode_t** head, bra_huffman_node_t* node)
{
    assert(head != NULL);
    assert(node != NULL);

    minHeapNode_t* new_node = bra_minHeapNode_create(node);
    if (new_node == NULL)
    {
        bra_log_error("unable to create huffman new node");
        return false;
    }

    if (*head == NULL || (*head)->node->freq > node->freq)
    {
        new_node->next = *head;
        *head          = new_node;
        return true;
    }

    minHeapNode_t* cur = *head;
    while (cur->next != NULL && cur->next->node->freq < node->freq)
    {
        cur = cur->next;
    }

    new_node->next = cur->next;
    cur->next      = new_node;
    return true;
}

static void bra_minHeap_clear(minHeapNode_t** head)
{
    if (head == NULL)
        return;

    while (*head != NULL)
    {
        bra_huffman_node_t* n = bra_minHeap_extractMin(head);
        bra_huffman_tree_free(&n);
    }
}

static bra_huffman_node_t* bra_huffman_tree_build(uint32_t freq[BRA_ALPHABET_SIZE])
{
    assert(freq != NULL);

    bra_huffman_node_t* n       = NULL;
    bra_huffman_node_t* l       = NULL;
    bra_huffman_node_t* r       = NULL;
    minHeapNode_t*      minHeap = NULL;
    for (int i = 0; i < BRA_ALPHABET_SIZE; ++i)
    {
        if (freq[i] > 0)
        {
            n = bra_huffman_create_node(i, freq[i]);
            if (n == NULL)
                goto BRA_HUFFMAN_TREE_BUILD_ERROR;

            if (!bra_minHeap_insert(&minHeap, n))
                goto BRA_HUFFMAN_TREE_BUILD_ERROR;

            n = NULL;
        }
    }

    if (minHeap == NULL)
        goto BRA_HUFFMAN_TREE_BUILD_ERROR;

    while (minHeap->next != NULL)
    {
        n = NULL;
        l = bra_minHeap_extractMin(&minHeap);
        r = bra_minHeap_extractMin(&minHeap);
        if (l == NULL || r == NULL)
            goto BRA_HUFFMAN_TREE_BUILD_ERROR;

        n = bra_huffman_create_node(0, l->freq + r->freq);
        if (n == NULL)
            goto BRA_HUFFMAN_TREE_BUILD_ERROR;

        n->left  = l;
        n->right = r;
        l = r = NULL;
        if (!bra_minHeap_insert(&minHeap, n))
            goto BRA_HUFFMAN_TREE_BUILD_ERROR;
    }

    return bra_minHeap_extractMin(&minHeap);

BRA_HUFFMAN_TREE_BUILD_ERROR:
    bra_log_error("unable to build huffman tree");
    bra_huffman_tree_free(&n);
    bra_huffman_tree_free(&l);
    bra_huffman_tree_free(&r);
    bra_minHeap_clear(&minHeap);
    return NULL;
}

static void bra_huffman_generate_codes(bra_huffman_node_t* root, uint8_t codes[BRA_ALPHABET_SIZE][BRA_ALPHABET_SIZE], uint8_t lengths[BRA_ALPHABET_SIZE], uint8_t code[BRA_ALPHABET_SIZE], int len)
{
    assert(codes != NULL);
    assert(lengths != NULL);
    assert(code != NULL);

    if (root == NULL)
    {
        bra_log_warn("unable to generate huffman's codes: huffman tree is null");
        return;
    }

    // if only 1 node
    if (root->left == NULL && root->right == NULL)
    {
        if (len == 0)
        {
            lengths[root->symbol]  = 1;
            codes[root->symbol][0] = 0;
        }
        else
        {
            lengths[root->symbol] = len;
            memcpy(codes[root->symbol], code, len);
        }
        return;
    }

    code[len] = 0;
    bra_huffman_generate_codes(root->left, codes, lengths, code, len + 1);
    code[len] = 1;
    bra_huffman_generate_codes(root->right, codes, lengths, code, len + 1);
}

/**
 * @brief Compute canonical Huffman codes from code lengths
 * @param lengths Input code lengths for each symbol
 * @param codes Output canonical codes (bit arrays)
 */
static void bra_huffman_compute_canonical_codes(const uint8_t lengths[BRA_ALPHABET_SIZE], uint8_t codes[BRA_ALPHABET_SIZE][BRA_ALPHABET_SIZE])
{
    // Count symbols per length
    uint32_t count[BRA_ALPHABET_SIZE + 1] = {0};
    for (int i = 0; i < BRA_ALPHABET_SIZE; ++i)
    {
        if (lengths[i] > 0)
            ++count[lengths[i]];
    }

    // Compute starting code for each length
    uint32_t code = 0;
    count[0]      = 0;    // Length 0 not used
    for (int len = 1; len <= BRA_ALPHABET_SIZE; ++len)
    {
        code          <<= 1;
        uint32_t temp   = count[len];
        count[len]      = code;
        code           += temp;
    }

    // Assign codes to symbols in order
    for (int i = 0; i < BRA_ALPHABET_SIZE; ++i)
    {
        if (lengths[i] == 0)
            continue;

        uint32_t c = count[lengths[i]]++;
        for (int j = lengths[i] - 1; j >= 0; --j)
        {
            codes[i][j]   = c & 1;
            c           >>= 1;
        }
    }
}

static bra_huffman_node_t* bra_huffman_tree_build_from_lengths(const uint8_t lengths[BRA_ALPHABET_SIZE])
{
    assert(lengths != NULL);

    // Compute canonical codes
    uint8_t codes[BRA_ALPHABET_SIZE][BRA_ALPHABET_SIZE];
    bra_huffman_compute_canonical_codes(lengths, codes);

    bra_huffman_node_t* n    = NULL;
    bra_huffman_node_t* root = bra_huffman_create_node(0, 0);
    if (root == NULL)
        goto BRA_HUFFMAN_DECODE_ERROR;

    for (int i = 0; i < BRA_ALPHABET_SIZE; ++i)
    {
        if (lengths[i] == 0)
            continue;

        bra_huffman_node_t* cur = root;
        for (int j = 0; j < lengths[i]; ++j)
        {
            const uint8_t bit = codes[i][j];
            if (j == lengths[i] - 1)
            {
                // Last bit, assign symbol
                n = bra_huffman_create_node(i, 0);
                if (n == NULL)
                    goto BRA_HUFFMAN_DECODE_ERROR;

                if (bit == 0)
                {
                    if (cur->left != NULL)
                        goto BRA_HUFFMAN_DECODE_ERROR;

                    cur->left = n;
                }
                else
                {
                    if (cur->right != NULL)
                        goto BRA_HUFFMAN_DECODE_ERROR;

                    cur->right = n;
                }

                n = NULL;
            }
            else
            {
                if (bit == 0)
                {
                    if (cur->left == NULL)
                    {
                        n = bra_huffman_create_node(0, 0);
                        if (n == NULL)
                            goto BRA_HUFFMAN_DECODE_ERROR;

                        cur->left = n;
                        n         = NULL;
                    }
                    cur = cur->left;
                }
                else    // bit == 1
                {
                    if (cur->right == NULL)
                    {
                        n = bra_huffman_create_node(0, 0);
                        if (n == NULL)
                            goto BRA_HUFFMAN_DECODE_ERROR;

                        cur->right = n;
                        n          = NULL;
                    }
                    cur = cur->right;
                }
            }
        }
    }

    return root;

BRA_HUFFMAN_DECODE_ERROR:
    bra_log_error("unable to rebuild huffman tree");
    bra_huffman_tree_free(&n);
    bra_huffman_tree_free(&root);
    return NULL;
}

//////////////////////////////////////////////////////////////////////////////////

bra_huffman_chunk_t* bra_huffman_encode(const uint8_t* buf, const uint32_t buf_size)
{
    assert(buf != NULL);

    uint32_t freq[BRA_ALPHABET_SIZE] = {0};

    bra_huffman_chunk_t* output = malloc(sizeof(bra_huffman_chunk_t));
    if (output == NULL)
    {
        bra_log_error("unable to encode huffman");
        return NULL;
    }

    output->meta.encoded_size = 0;
    output->data              = NULL;

    // 1. count frequencies
    for (uint32_t i = 0; i < buf_size; ++i)
        ++freq[buf[i]];

    // 2. build Huffman Tree
    bra_huffman_node_t* root = bra_huffman_tree_build(freq);
    if (root == NULL)
    {
        bra_log_error("unable to huffman encode");
        free(output);
        return NULL;
    }

    // 3. Generate codes
    uint8_t codes[BRA_ALPHABET_SIZE][BRA_ALPHABET_SIZE];
    // uint8_t lengths[BRA_ALPHABET_SIZE] = {0};
    memset(output->meta.lengths, 0, BRA_ALPHABET_SIZE * sizeof(uint8_t));
    uint8_t code[BRA_ALPHABET_SIZE];
    bra_huffman_generate_codes(root, codes, output->meta.lengths, code, 0);
    bra_huffman_compute_canonical_codes(output->meta.lengths, codes);

    // 4. calculate output size
    uint32_t bit_count = 0;
    for (uint32_t i = 0; i < buf_size; ++i)
        bit_count += output->meta.lengths[buf[i]];

    output->meta.orig_size    = buf_size;
    output->meta.encoded_size = (bit_count + 7) / 8;    // Code lengths + packed data
    output->data              = malloc(output->meta.encoded_size);
    if (output->data == NULL)
    {
        bra_log_error("unable to encode huffman");
        bra_huffman_tree_free(&root);
        free(output);
        return NULL;
    }

    // 5. encode data
    uint8_t* data_ptr = output->data;
    uint8_t  cur_byte = 0;
    int      bit_pos  = 0;
    for (uint32_t i = 0; i < buf_size; ++i)
    {
        const uint8_t symbol = buf[i];
        for (int j = 0; j < output->meta.lengths[symbol]; ++j)
        {
            if (codes[symbol][j] > 0)
                cur_byte |= (1 << (7 - bit_pos));

            ++bit_pos;
            if (bit_pos == 8)
            {
                *data_ptr++ = cur_byte;
                cur_byte    = 0;
                bit_pos     = 0;
            }
        }
    }

    if (bit_pos > 0)
        *data_ptr = cur_byte;

    bra_huffman_tree_free(&root);
    return output;
}

uint8_t* bra_huffman_decode(const bra_huffman_t* meta, const uint8_t* data, uint32_t* out_size)
{
    assert(meta != NULL);
    assert(data != NULL);
    assert(out_size != NULL);

    *out_size                = 0;
    bra_huffman_node_t* root = bra_huffman_tree_build_from_lengths(meta->lengths);
    if (root == NULL)
        return NULL;

    // Decode data
    const uint32_t data_size = meta->encoded_size;
    uint8_t*       decoded   = (uint8_t*) malloc(meta->orig_size);
    if (decoded == NULL)
    {
        bra_log_error("unable to decode huffman");
        bra_huffman_tree_free(&root);
        return NULL;
    }

    uint32_t            decoded_idx = 0;
    bra_huffman_node_t* cur         = root;
    for (uint32_t i = 0; i < data_size; ++i)
    {
        const uint8_t byte = data[i];
        for (int bit = 7; bit >= 0; bit--)
        {
            const uint8_t bit_val = (byte >> bit) & 1;
            cur                   = bit_val == 0 ? cur->left : cur->right;

            // sanity check
            if (cur == NULL)
            {
                bra_log_error("huffman decode error: invalid code sequence");
                goto BRA_HUFFMAN_DECODE_ERROR;
            }

            if (cur->left == NULL && cur->right == NULL)
            {
                // Leaf node
                decoded[decoded_idx++] = cur->symbol;
                cur                    = root;
                // NOTE: this is required to skip the eventual padding bits
                if (decoded_idx >= meta->orig_size)
                    break;
            }
        }
    }

    bra_huffman_tree_free(&root);
    if (decoded_idx != meta->orig_size)
    {
        bra_log_error("huffman decode error: decoded data:%u - original_data:%u", decoded_idx, meta->orig_size);
        goto BRA_HUFFMAN_DECODE_ERROR;
    }

    *out_size = decoded_idx;
    return decoded;

BRA_HUFFMAN_DECODE_ERROR:
    bra_huffman_tree_free(&root);
    free(decoded);
    return NULL;
}

void bra_huffman_chunk_free(bra_huffman_chunk_t* chunk)
{
    if (chunk == NULL)
        return;

    if (chunk->data != NULL)
    {
        free(chunk->data);
        chunk->data = NULL;
    }

    free(chunk);
}
