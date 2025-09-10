#pragma once

#include <stdint.h>


typedef uint8_t bra_attr_t;          //!< file attribute type
typedef uint8_t bra_rle_counts_t;    //!< rle counts type

/**
 * @brief Define a file overwrite policy.
 */
typedef enum bra_fs_overwrite_policy_e
{
    BRA_OVERWRITE_ASK        = 0,
    BRA_OVERWRITE_ALWAYS_YES = 1,
    BRA_OVERWRITE_ALWAYS_NO  = 2,
} bra_fs_overwrite_policy_e;

/**
 * @brief
 */
typedef struct bra_rle_chunk_t
{
    bra_rle_counts_t        counts;    //!< counts is stored as -1, i.e. 0 means 1 and 255 means 256
    char                    value;     //!< the repeated char.
    struct bra_rle_chunk_t* pNext;     //!< to the next chunk, NULL if it is the last one.
} bra_rle_chunk_t;
