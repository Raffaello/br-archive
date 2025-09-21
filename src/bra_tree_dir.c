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

static bra_tree_node_t* _bra_tree_node_parent_index_search(bra_tree_node_t* node, const uint32_t parent_index)
{
    bra_tree_node_t* res = node;
    while (res != NULL)
    {
        if (res->index == parent_index)
            return res;

        if (res->firstChild != NULL)
        {
            bra_tree_node_t* r = _bra_tree_node_parent_index_search(res->firstChild, parent_index);
            if (r != NULL)
                return r;
        }

        res = res->next;
    }

    return NULL;
}

static bra_tree_node_t* _bra_tree_dir_add_child_node(bra_tree_dir_t* tree, bra_tree_node_t* parent, const char* dirname)
{
    if (tree == NULL || parent == NULL || dirname == NULL || dirname[0] == '\0')
        return NULL;

    // check if it is not already present first
    bra_tree_node_t* sibling = parent->firstChild;
    while (sibling != NULL)
    {
        if (strcmp(sibling->dirname, dirname) == 0)
            return sibling;    // already present
        sibling = sibling->next;
    }

    bra_tree_node_t* new_node = _bra_tree_node_alloc();
    if (new_node == NULL)
        return NULL;

    new_node->dirname = _bra_strdup(dirname);
    if (new_node->dirname == NULL)
    {
        free(new_node);
        return NULL;
    }
    new_node->parent = parent;
    new_node->index  = tree->num_nodes;
    if (parent->firstChild == NULL)
        parent->firstChild = new_node;
    else
    {
        // TODO: better to add as first child instead of last.
        //       this will change the order though.
        //       alternative track the tail too (lastChild for faster insertion)
        // new_node->next     = parent->firstChild;
        // parent->firstChild = new_node;
        sibling = parent->firstChild;
        while (sibling->next != NULL)
            sibling = sibling->next;
        sibling->next = new_node;
    }
    ++tree->num_nodes;
    return new_node;
}

////////////////////////////////////////////////////////////////////

#ifndef NDEBUG
void bra_tree_node_print(const bra_tree_node_t* node)
{
    while (node != NULL)
    {
        bra_log_debug("tree: [%u] %s", node->index, node->dirname);

        bra_tree_node_print(node->firstChild);

        node = node->next;
    }
}
#endif

bra_tree_dir_t* bra_tree_dir_create()
{
    bra_tree_dir_t* tree = (bra_tree_dir_t*) malloc(sizeof(bra_tree_dir_t));
    if (tree == NULL)
        return NULL;

    tree->root      = _bra_tree_node_alloc();    // alloc root with './' current dir
    tree->num_nodes = 1U;

    if (tree->root == NULL)
        bra_tree_dir_destroy(&tree);

    return tree;
}

void bra_tree_dir_destroy(bra_tree_dir_t** tree)
{
    if (tree == NULL || *tree == NULL)
        return;

    const uint32_t count = _bra_tree_node_destroy_subtree(&(*tree)->root);
    if (count != (*tree)->num_nodes)
        bra_log_warn("tree destroy count mismatch: %u != %u", count, (*tree)->num_nodes);
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
    {
        bra_log_error("invalid dirname, can't tokenize: %s", dirname);
        free(parts);
        return NULL;
    }

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
        parent = _bra_tree_dir_add_child_node(tree, parent, part);
        if (parent == NULL)
            goto BRA_TREE_DIR_ADD_NULL;
        part = strtok(NULL, BRA_DIR_DELIM);
    }
    while (part != NULL);

    free(parts);
    return parent;

BRA_TREE_DIR_ADD_NULL:
    free(parts);
    return NULL;
}

bra_tree_node_t* bra_tree_dir_parent_index_search(const bra_tree_dir_t* tree, const uint32_t parent_index)
{
    if (tree == NULL)
        return NULL;

    if (parent_index == 0U)
        return tree->root;

    return _bra_tree_node_parent_index_search(tree->root, parent_index);
}

bra_tree_node_t* bra_tree_dir_insert_at_parent(bra_tree_dir_t* tree, const uint32_t parent_index, const char* dirname)
{
    if (tree == NULL || dirname == NULL || dirname[0] == '\0')
        return NULL;

    bra_tree_node_t* parent = bra_tree_dir_parent_index_search(tree, parent_index);
    if (parent == NULL)
    {
        bra_log_error("parent index %u not found in tree", parent_index);
        return NULL;
    }

    // dirname is the dirname, not the final dir
    // so need to be divided and inserted in the tree in chunks
    char* parts = _bra_strdup(dirname);
    if (parts == NULL)
        return NULL;
    char* part = strtok(parts, BRA_DIR_DELIM);
    if (part == NULL)
        part = parts;

    do
    {
        parent = _bra_tree_dir_add_child_node(tree, parent, part);
        if (parent == NULL)
            goto BRA_TREE_DIR_INSERT_AT_PARENT_NULL;
        part = strtok(NULL, BRA_DIR_DELIM);
    }
    while (part != NULL);

    free(parts);
    return parent;

BRA_TREE_DIR_INSERT_AT_PARENT_NULL:
    free(parts);
    return NULL;
}

char* bra_tree_dir_reconstruct_path(const bra_tree_node_t* node)
{
    assert(node != NULL);

    size_t           len = 0U;
    bra_tree_node_t* n   = (bra_tree_node_t*) node;
    while (n->index != 0U)
    {
        len += strlen(n->dirname) + 1U;    // +1 for delimiter
        n    = n->parent;
    }

    if (len == 0U)
        return NULL;                     // root dir

    char* path = (char*) malloc(len);    // last delimiter is '\0'
    if (path == NULL)
        return NULL;

    // Reconstruct the path
    n          = (bra_tree_node_t*) node;
    size_t pos = len - 1;
    while (n->index != 0U)
    {
        const size_t dir_len = strlen(n->dirname);
        path[pos]            = BRA_DIR_DELIM[0];
        assert(pos >= dir_len);
        memcpy(&path[pos - dir_len], n->dirname, dir_len);
        pos -= (dir_len + 1);
        n    = n->parent;
    }

    assert(pos == (size_t) -1);
    path[len - 1] = '\0';    // Null-terminate the string
    return path;
}
