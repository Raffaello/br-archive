#pragma once
#ifndef BRA_TREE_DIR_H
#define BRA_TREE_DIR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <lib_bra_types.h>


#define BRA_TREE_NODE_ROOT_INDEX 0

#ifndef NDEBUG
/**
 * @brief Print the tree starting from @p node.
 *
 * @param node
 */
void bra_tree_node_print(const bra_tree_node_t* node);
#endif

/**
 * @brief Create a new directory tree. It adds the root node (".").
 *
 * @return bra_tree_dir_t*
 */
bra_tree_dir_t* bra_tree_dir_create(void);

/**
 * @brief Destroy a directory tree.
 *
 * @param tree
 */
void bra_tree_dir_destroy(bra_tree_dir_t** tree);

/**
 * @brief Add a directory to the tree.
 *
 * @param tree
 * @param dirname is the directory to add (e.g. "dir/subdir")
 * @return bra_tree_node_t* the inserted node, @c NULL on error.
 */
bra_tree_node_t* bra_tree_dir_add(bra_tree_dir_t* tree, const char* dirname);

/**
 * @brief Search a node by its index (0 is root) and return the node pointer.
 *
 * @param tree
 * @param dirname
 * @param parent_index
 * @return bra_tree_node_t* the found node, @c NULL if not found.
 */
bra_tree_node_t* bra_tree_dir_parent_index_search(const bra_tree_dir_t* tree, const uint32_t parent_index);

/**
 * @brief Insert a directory at the specified parent index.
 *
 * @param tree
 * @param parent_index
 * @param dirname
 * @return bra_tree_node_t* the inserted node, @c NULL on error.
 */
bra_tree_node_t* bra_tree_dir_insert_at_parent(bra_tree_dir_t* tree, const uint32_t parent_index, const char* dirname);

/**
 * @brief Reconstruct the full path from the root to the given node.
 *        The return string must be freed by the caller.
 *        Root dir return an empty string.
 *        On error returns @c NULL.
 *
 * @param node
 * @return char*
 */
char* bra_tree_dir_reconstruct_path(const bra_tree_node_t* node);

#ifdef __cplusplus
}
#endif

#endif    // BRA_TREE_DIR_H
