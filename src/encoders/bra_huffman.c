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

static void bra_minHeap_insert(minHeapNode_t** head, bra_huffman_node_t* node)
{
    assert(head != NULL);
    assert(node != NULL);

    minHeapNode_t* new_node = bra_minHeapNode_create(node);
    if (*head == NULL || (*head)->node->freq > node->freq)
    {
        new_node->next = *head;
        *head          = new_node;
        return;
    }

    minHeapNode_t* cur = *head;
    while (cur->next != NULL && cur->next->node->freq <= node->freq)
    {
        cur = cur->next;
    }

    new_node->next = cur->next;
    cur->next      = new_node;
}

static bra_huffman_node_t* bra_huffman_tree_build(uint32_t freq[BRA_ALPHABET_SIZE])
{
    assert(freq != NULL);

    minHeapNode_t* minHeap = NULL;
    for (int i = 0; i < BRA_ALPHABET_SIZE; ++i)
    {
        if (freq[i] > 0)
        {
            bra_huffman_node_t* n = bra_huffman_create_node(i, freq[i]);
            if (n == NULL)
            {
                bra_log_error("unable to build huffman tree");
                return NULL;
            }

            bra_minHeap_insert(&minHeap, n);
        }
    }

    if (minHeap == NULL)
    {
        bra_log_warn("unable to build tree, no data");
        return NULL;
    }

    while (minHeap->next != NULL)
    {
        bra_huffman_node_t* l = bra_minHeap_extractMin(&minHeap);
        bra_huffman_node_t* r = bra_minHeap_extractMin(&minHeap);
        bra_huffman_node_t* n = bra_huffman_create_node(0, l->freq + r->freq);
        n->left               = l;
        n->right              = r;
        bra_minHeap_insert(&minHeap, n);
    }

    return bra_minHeap_extractMin(&minHeap);
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

static bra_huffman_node_t* bra_huffman_tree_build_from_lengths(const uint8_t lengths[BRA_ALPHABET_SIZE])
{
    assert(lengths != NULL);

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
            if (j == lengths[i] - 1)
            {
                // Last bit, assign symbol
                bra_huffman_node_t* n = bra_huffman_create_node(i, 0);
                if (n == NULL)
                    goto BRA_HUFFMAN_DECODE_ERROR;

                if (cur->left == NULL)
                    cur->left = n;
                else
                    cur->right = n;
            }
            else
            {
                if (cur->left == NULL)
                {
                    bra_huffman_node_t* n = bra_huffman_create_node(0, 0);
                    if (n == NULL)
                        goto BRA_HUFFMAN_DECODE_ERROR;

                    cur->left = n;
                    cur       = cur->left;
                }
                else if (cur->right == NULL)
                {
                    bra_huffman_node_t* n = bra_huffman_create_node(0, 0);
                    if (n == NULL)
                        goto BRA_HUFFMAN_DECODE_ERROR;

                    cur->right = n;
                    cur        = cur->right;
                }
                else
                    cur = cur->right;
            }
        }
    }

    return root;

BRA_HUFFMAN_DECODE_ERROR:
    bra_log_error("unable to rebuild huffman tree");
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

    output->size = 0;
    output->data = NULL;

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
        // output->data = malloc(buf_size);
        // if (output->data == NULL)
        // {
        //     bra_log_error("unable to encode huffman");
        //     free(output);
        //     return NULL;
        // }

        // output->size = buf_size;
        // memset(output->lengths, 0, 256);
        // output->data[buf[0]] = 1;
        // memcpy(output->data, buf, buf_size);
        // return output;
    }

    // 3. Generate codes
    uint8_t codes[BRA_ALPHABET_SIZE][BRA_ALPHABET_SIZE];
    // uint8_t lengths[BRA_ALPHABET_SIZE] = {0};
    memset(output->lengths, 0, BRA_ALPHABET_SIZE * sizeof(uint8_t));
    uint8_t code[BRA_ALPHABET_SIZE];
    bra_huffman_generate_codes(root, codes, output->lengths, code, 0);

    // 4. calculate output size
    uint32_t bit_count = 0;
    for (uint32_t i = 0; i < buf_size; ++i)
        bit_count += output->lengths[buf[i]];

    output->orig_size = buf_size;
    output->size      = (bit_count + 7) / 8;    // Code lengths + packed data
    output->data      = malloc(output->size);
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
        for (int j = 0; j < output->lengths[symbol]; ++j)
        {
            if (codes[symbol][j] > 0)
            {
                cur_byte |= (1 << (7 - bit_pos));
            }
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

uint8_t* bra_huffman_decode(const bra_huffman_chunk_t* chunk, uint32_t* out_size)
{
    assert(chunk != NULL);
    assert(out_size != NULL);

    *out_size                = 0;
    bra_huffman_node_t* root = bra_huffman_tree_build_from_lengths(chunk->lengths);
    if (root == NULL)
        return NULL;

    // Decode data
    const uint8_t* data_ptr  = chunk->data;
    uint32_t       data_size = chunk->size;
    uint8_t*       decoded   = (uint8_t*) malloc(data_size * 8);    // Overestimate size
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
        const uint8_t byte = data_ptr[i];
        for (int bit = 7; bit >= 0; bit--)
        {
            int bit_val = (byte >> bit) & 1;
            cur         = bit_val == 0 ? cur->left : cur->right;
            if (cur->left == NULL && cur->right == NULL)
            {
                // Leaf node
                decoded[decoded_idx++] = cur->symbol;
                cur                    = root;
                if (decoded_idx == chunk->orig_size)
                    break;
            }
        }
    }

    bra_huffman_tree_free(&root);
    uint8_t* output = realloc(decoded, decoded_idx);
    if (output == NULL)
    {
        bra_log_error("unable to realloc huffman decoded data");
        free(decoded);
        return NULL;
    }

    *out_size = decoded_idx;
    return output;    // Caller must free
}

void bra_huffman_free(bra_huffman_chunk_t* chunk)
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
