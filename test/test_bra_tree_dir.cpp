#include "bra_test.hpp"
#include <bra_tree_dir.h>

#include <string.h>

///////////////////////////////////////////////////////////////////////////////

static int _test_bra_tree_node(const bra_tree_node_t* n, const uint32_t index, const char* dirname, const bra_tree_node_t* parent, const bool nextIsNull, const bool fistChildIsNull)
{
    ASSERT_TRUE(n != nullptr);

    ASSERT_EQ(n->index, index);
    ASSERT_EQ(strcmp(n->dirname, dirname), 0);
    ASSERT_TRUE(n->parent == parent);

    if (nextIsNull)
    {
        ASSERT_TRUE(n->next == nullptr);
    }
    else
    {
        ASSERT_TRUE(n->next != nullptr);
    }

    if (fistChildIsNull)
    {
        ASSERT_TRUE(n->firstChild == nullptr);
    }
    else
    {
        ASSERT_TRUE(n->firstChild != nullptr);
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////

const char* g_argv0 = nullptr;

TEST(test_bra_tree_dir_add1)
{
    bra_tree_dir_t* tree = bra_tree_dir_create();
    ASSERT_TRUE(tree != nullptr);

    bra_tree_node_t* n1 = bra_tree_dir_add(tree, "dir1");
    ASSERT_TRUE(n1 != nullptr);
    ASSERT_TRUE(bra_tree_dir_add(tree, "dir1") == nullptr);

    ASSERT_EQ(tree->num_nodes, 2U);
    ASSERT_TRUE(tree->root->dirname == nullptr);
    ASSERT_EQ(tree->root->index, 0U);
    ASSERT_TRUE(tree->root->next == nullptr);
    ASSERT_TRUE(tree->root->firstChild != nullptr);

    bra_tree_node_t* n = tree->root->firstChild;
    ASSERT_TRUE(n == n1);
    ASSERT_EQ(n->index, 1U);
    ASSERT_EQ(strncmp(n->dirname, "dir1", 5), 0);
    ASSERT_TRUE(n->parent == tree->root);
    bra_tree_dir_destroy(&tree);
    ASSERT_TRUE(tree == nullptr);
    return 0;
}

TEST(test_bra_tree_dir_add2)
{
    bra_tree_dir_t* tree = bra_tree_dir_create();
    ASSERT_TRUE(tree != nullptr);

    bra_tree_node_t* n1 = bra_tree_dir_add(tree, "dir1");
    bra_tree_node_t* n2 = bra_tree_dir_add(tree, "dir2");
    bra_tree_node_t* n3 = bra_tree_dir_add(tree, "dir1/dir11");
    bra_tree_node_t* n4 = bra_tree_dir_add(tree, "dir1/dir11/dir111");
    ASSERT_TRUE(n1 != nullptr);
    ASSERT_TRUE(n2 != nullptr);
    ASSERT_TRUE(n3 != nullptr);
    ASSERT_TRUE(n4 != nullptr);

    ASSERT_EQ(tree->num_nodes, 5U);
    ASSERT_TRUE(tree->root->firstChild != nullptr);

    bra_tree_node_t* n = tree->root->firstChild;
    ASSERT_EQ(_test_bra_tree_node(n, 1U, "dir1", tree->root, false, false), 0);
    ASSERT_EQ(_test_bra_tree_node(n->next, 2U, "dir2", tree->root, true, true), 0);
    ASSERT_EQ(_test_bra_tree_node(n->firstChild, 3U, "dir11", n, true, false), 0);
    ASSERT_EQ(_test_bra_tree_node(n->firstChild->firstChild, 4U, "dir111", n->firstChild, true, true), 0);

    ASSERT_TRUE(n1 == n);
    ASSERT_TRUE(n2 == n->next);
    ASSERT_TRUE(n3 == n->firstChild);
    ASSERT_TRUE(n4 == n->firstChild->firstChild);

    bra_tree_dir_destroy(&tree);
    ASSERT_TRUE(tree == nullptr);
    return 0;
}

TEST(test_bra_tree_dir_add3)
{
    bra_tree_dir_t* tree = bra_tree_dir_create();
    ASSERT_TRUE(tree != nullptr);

    ASSERT_TRUE(bra_tree_dir_add(tree, "dir1") != nullptr);
    ASSERT_TRUE(bra_tree_dir_add(tree, "dir2") != nullptr);
    ASSERT_TRUE(bra_tree_dir_add(tree, "dir1/dir11") != nullptr);
    ASSERT_TRUE(bra_tree_dir_add(tree, "dir1/dir11/dir111") != nullptr);
    ASSERT_TRUE(bra_tree_dir_add(tree, "dir3/dir33/dir333/dir3333") != nullptr);

    ASSERT_TRUE(bra_tree_dir_add(tree, "dir3/dir33/dir333/dir3333") == nullptr);

    ASSERT_TRUE(bra_tree_dir_insert_at_parent(tree, 2U, "dir22") != nullptr);


    ASSERT_EQ(tree->num_nodes, 10U);
    ASSERT_TRUE(tree->root->firstChild != nullptr);
    bra_tree_node_t* n = tree->root->firstChild;
    ASSERT_EQ(_test_bra_tree_node(n, 1U, "dir1", tree->root, false, false), 0);
    ASSERT_EQ(_test_bra_tree_node(n->next, 2U, "dir2", tree->root, false, false), 0);
    ASSERT_EQ(_test_bra_tree_node(n->firstChild, 3U, "dir11", n, true, false), 0);

    bra_tree_node_t* n2 = n->firstChild->firstChild;    // dir111
    ASSERT_EQ(_test_bra_tree_node(n2, 4U, "dir111", n->firstChild, true, true), 0);

    n2 = n->next;    // dir2
    // ASSERT_EQ(_test_bra_tree_node(n2, 2U, "dir2", tree->root, false, true), 0);
    ASSERT_EQ(_test_bra_tree_node(n2->firstChild, 9U, "dir22", n2, true, true), 0);

    n2 = n->next->next;    // dir3
    ASSERT_EQ(_test_bra_tree_node(n2, 5U, "dir3", tree->root, true, false), 0);
    ASSERT_EQ(_test_bra_tree_node(n2->firstChild, 6U, "dir33", n2, true, false), 0);
    n2 = n2->firstChild;    // dir33
    ASSERT_EQ(_test_bra_tree_node(n2->firstChild, 7U, "dir333", n2, true, false), 0);
    n2 = n2->firstChild;    // dir333
    ASSERT_EQ(_test_bra_tree_node(n2->firstChild, 8U, "dir3333", n2, true, true), 0);

    bra_tree_dir_destroy(&tree);
    ASSERT_TRUE(tree == nullptr);
    return 0;
}

int main(int argc, char* argv[])
{
    g_argv0 = argv[0];

    ASSERT_TRUE(g_argv0 != nullptr);

    const std::map<std::string, std::function<int()>> m = {
        {TEST_FUNC(test_bra_tree_dir_add1)},
        {TEST_FUNC(test_bra_tree_dir_add2)},
        {TEST_FUNC(test_bra_tree_dir_add3)},
    };

    return test_main(argc, argv, m);
}
