#pragma once

#include "lib_bra_defs.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>


typedef uint8_t bra_attr_t;          //!< file attribute type
typedef uint8_t bra_rle_counts_t;    //!< stored as run_length - 1 (0 => 1, 255 => 256)

/**
 * @brief Define a file overwrite policy.
 */
typedef enum bra_fs_overwrite_policy_e
{
    BRA_OVERWRITE_ASK        = 0,
    BRA_OVERWRITE_ALWAYS_YES = 1,
    BRA_OVERWRITE_ALWAYS_NO  = 2,
} bra_fs_overwrite_policy_e;

#pragma pack(push, 1)

/**
 * @brief BR-Archive File Header.
 */
typedef struct bra_io_header_t
{
    uint32_t magic;    //!< 'BR-a'

    // uint8_t  version;    //!< Archive format version
    // compression type ? (archive, best, fast, ... ??)
    // crc32 / md5 ?
    uint32_t num_files;

} bra_io_header_t;

/**
 * @brief BR-Sfx File Footer.
 */
typedef struct bra_io_footer_t
{
    uint32_t magic;            //!< 'BR-x'
    int64_t  header_offset;    //!< absolute offset of the header chunk from file start.
} bra_io_footer_t;

#pragma pack(pop)

/**
 * @brief Type used to perform I/O from the disk.
 *        It is just a simple wrapper around @c FILE,
 *        but it carries on the filename @p fn associated with it.
 */
typedef struct bra_io_file_t
{
    FILE* f;     //!< File Pointer representing a file on the disk.
    char* fn;    //!< the filename of the file on disk.
} bra_io_file_t;

/**
 * @brief This is the metadata of each file stored in a BR-archive.
 *        The file data is just after for @p data_size bytes.
 */
typedef struct bra_meta_entry_t
{
    // TODO: add CRC ... file permissions, etc... ?
    bra_attr_t attributes;    //!< file attributes: #BRA_ATTR_TYPE_FILE, #BRA_ATTR_TYPE_DIR, #BRA_ATTR_TYPE_SYMLINK, #BRA_ATTR_TYPE_SUBDIR
    uint8_t    name_size;     //!< length in bytes excluding the trailing NUL; [1..UINT8_MAX]
    char*      name;          //!< filename/dirname (owned; free via @ref bra_meta_entry_free)
    void*      entry_data;    //!< Type-specific entry data: file size for files, parent tree index for subdirs, NULL for dirs and symlinks. (owned; free via @ref bra_meta_entry_free)
    uint32_t   crc32;         //!< CRC32C of the file data; 0 if not computed.
} bra_meta_entry_t;

// typedef struct bra_meta_entry_footer_t
// {
//     uint32_t crc32;
// } bra_meta_entry_footer_t;

/**
 * @brief Metadata for a file entry in a BR-archive.
 */
typedef struct bra_meta_entry_file_t

{
    uint64_t data_size;    //!< file contents size in bytes.
} bra_meta_entry_file_t;

/**
 * @brief Metadata for a directory entry in a BR-archive.
 */
typedef struct bra_meta_entry_subdir_t
{
    uint32_t parent_index;    //!< index of the parent directory in the archive; 0 for root.
} bra_meta_entry_subdir_t;

/**
 * @brief RLE chunk representing a run of repeated characters.
 */
typedef struct bra_rle_chunk_t
{
    bra_rle_counts_t        counts;    //!< counts is stored as -1, i.e. 0 means 1 and 255 means 256
    uint8_t                 value;     //!< the repeated char.
    struct bra_rle_chunk_t* pNext;     //!< pointer to next chunk; NULL if last.
} bra_rle_chunk_t;

/**
 * @brief Directory tree node.
 */
typedef struct bra_tree_node_t
{
    uint32_t                index;
    char*                   dirname;
    struct bra_tree_node_t* parent;
    struct bra_tree_node_t* firstChild;
    struct bra_tree_node_t* next;
} bra_tree_node_t;

/**
 * @brief Directory tree.
 */
typedef struct bra_tree_dir_t
{
    bra_tree_node_t* root;
    uint32_t         num_nodes;
} bra_tree_dir_t;

/**
 * @brief Archive File Context.
 */
typedef struct bra_io_file_ctx_t
{
    bra_io_file_t    f;
    uint32_t         num_files;        //!< num files to be written in the header.
    uint32_t         cur_files;        //!< entries written in this session; used to reconcile header on close
    char*            last_dir;         //!< the last encoded or decoded directory. used for files as they don't know where they belong.
    size_t           last_dir_size;    //!< length of last_dir in bytes;
    bra_tree_dir_t*  tree;             //!< directory tree used when encoding.
    bra_tree_node_t* last_dir_node;    //!< pointer to the node of last_dir in the tree; NULL for root.

} bra_io_file_ctx_t;
