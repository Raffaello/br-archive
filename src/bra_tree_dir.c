#include <bra_tree_dir.h>
#include <lib_bra_private.h>    // _bra_strdup
#include <log/bra_log.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define BRA_DIR_DELIM "/"

static uint32_t _bra_tree_node_destroy_subtree(bra_tree_node_t** node)
{
    if (node == NULL || *node == NULL)
        return 0;

    uint32_t         count = 0;
    bra_tree_node_t* child = (*node)->firstChild;
    while (child != NULL)
    {
        bra_tree_node_t* next  = child->next;
        count                 += _bra_tree_node_destroy_subtree(&child);
        child                  = next;
    }
    if ((*node)->dirname != NULL)
    {
        free((*node)->dirname);
        (*node)->dirname = NULL;
    }
    free(*node);
    ++count;
    *node = NULL;
    return count;
}

static bra_tree_node_t* _bra_tree_node_alloc()
{
    bra_tree_node_t* node = (bra_tree_node_t*) malloc(sizeof(bra_tree_node_t));
    if (node != NULL)
    {
        node->parent     = NULL;
        node->dirname    = NULL;
        node->firstChild = NULL;
        node->next       = NULL;
        node->index      = 0U;
    }

    return node;
}

// static bool _bra_tree_node_search(bra_tree_node_t** node, const char* dirname_part)
// {
//     if (node == NULL || *node == NULL || dirname_part == NULL || dirname_part[0] == '\0')
//         return false;

// bra_tree_node_t* cur = *node;
// while (cur != NULL)
// {
//     if (strcmp(cur->dirname, dirname_part) == 0)
//     {
//         *node = cur;
//         return true;
//     }

// // if (cur->firstChild != NULL)
//     // _bra_tree_node_search(cur->firstChild, dirname_part);

// cur = cur->next;
// }

// return false;
// }

// static bra_tree_node_t* _bra_tree_node_parent_index_search(const bra_tree_node_t* node, const uint32_t parent_index)
// {
//     if (node == NULL)
//         return NULL;

// bra_tree_node_t* res = node;

// while (res != NULL)
// {
//     if (res->index == parent_index)
//         return node;

// if (res->firstChild != NULL)
// {
//     res = _bra_tree_node_parent_index_search(res->firstChild, parent_index);
//     if (res != NULL)
//         return res;
// }

// res = res->next;
// }

// return NULL;
// }

////////////////////////////////////////////////////////////////////

bra_tree_dir_t* bra_tree_dir_create()
{
    bra_tree_dir_t* tree = (bra_tree_dir_t*) malloc(sizeof(bra_tree_dir_t));
    if (tree == NULL)
        return NULL;

    tree->root      = _bra_tree_node_alloc();    // TODO alloc root with './' current dir
    tree->num_nodes = 1U;
    // tree->cur_index = 0U;

    if (tree->root == NULL)
        bra_tree_dir_destroy(&tree);

    return tree;
}

void bra_tree_dir_destroy(bra_tree_dir_t** tree)
{
    if (tree == NULL || *tree == NULL)
        return;

    uint32_t count = _bra_tree_node_destroy_subtree(&(*tree)->root);
    assert(count == (*tree)->num_nodes);
    free(*tree);
    *tree = NULL;
}

bra_tree_node_t* bra_tree_dir_add(bra_tree_dir_t* tree, const char* dirname)
{
    if (tree == NULL || dirname == NULL || dirname[0] == '\0')
        return NULL;

    // dirname is the dirname, not the final dir
    // so need to be divided and inserted in the tree in chunks
    char* parts = _bra_strdup(dirname);
    if (parts == NULL)
        return NULL;

    char* part = strtok(parts, BRA_DIR_DELIM);
    if (part == NULL)
        part = parts;

    // bra_tree_node_t* cur    = tree->root;
    // bra_tree_node_t* parent = NULL;

    // skip the root as it is current directory always
    bra_tree_node_t* parent = tree->root;
    bra_tree_node_t* cur    = parent->firstChild;
    while (cur != NULL)
    {
        if (strcmp(cur->dirname, part) == 0)
        {
            // already exists
            // next token
            part = strtok(NULL, BRA_DIR_DELIM);
            if (part == NULL)
            {
                // it was already in there (actually error)
                bra_log_error("directory %s already in tree", dirname);
                goto BRA_TREE_DIR_ADD_NULL;
            }

            if (cur->firstChild != NULL)
            {
                parent = cur;
                cur    = cur->firstChild;    // going down one level
            }
            else
            {
                parent = cur;
                cur    = NULL;    // will exit loop and add new node as first child
            }
        }
        else
        {
            cur = cur->next;
        }
    }

    // if it is here it wasn't found, so add it
    assert(part != NULL && part[0] != '\0');
    do
    {
        bra_tree_node_t* new_node = _bra_tree_node_alloc();
        if (new_node == NULL)
            goto BRA_TREE_DIR_ADD_NULL;

        new_node->dirname = _bra_strdup(part);
        if (new_node->dirname == NULL)
        {
            free(new_node);
            goto BRA_TREE_DIR_ADD_NULL;
        }
        new_node->parent = parent;
        new_node->index  = tree->num_nodes;
        if (parent->firstChild == NULL)
            parent->firstChild = new_node;
        else
        {
            // TODO: better to add as first child instead of last.
            bra_tree_node_t* sibling = parent->firstChild;
            while (sibling->next != NULL)
                sibling = sibling->next;
            sibling->next = new_node;
        }
        ++tree->num_nodes;
        cur    = new_node;
        parent = new_node;
        part   = strtok(NULL, BRA_DIR_DELIM);
    }
    while (part != NULL);

    free(parts);
    return cur;

BRA_TREE_DIR_ADD_NULL:
    free(parts);
    return NULL;
}

// bra_tree_node_t* bra_tree_dir_parent_index_search(const bra_tree_dir_t* tree, const uint32_t parent_index)
// {
//     if (tree == NULL)
//         return NULL;

// if (parent_index == 0U)
//     return tree->root;

// bra_tree_node_t* cur = tree->root->firstChild;
// while (cur != NULL)
// {
//     if (cur->index == parent_index)
//         return cur;

// if (cur->firstChild != NULL)
// {
//     // bra_tree_node_t* res = bra_tree_dir_parent_index_search((bra_tree_dir_t*) tree, parent_index);
//     bra_tree_node_t* res = _bra_tree_node_parent_index_search(cur->firstChild, parent_index);
//     if (res != NULL)
//         return res;
// }

// cur = cur->next;
// }

// return NULL;
// }

// bool bra_tree_dir_extract()
// {
//     // TODO: this will rebuild the dirname from the parent index.
// }
