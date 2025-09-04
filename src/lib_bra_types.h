#pragma once

#include <stdint.h>


typedef uint8_t bra_attr_t;    //!< file attribute type

/**
 * @brief define a file overwrite mode policy.
 */
typedef enum bra_fs_overwrite_policy_e
{
    BRA_OVERWRITE_ASK        = 0,
    BRA_OVERWRITE_ALWAYS_YES = 1,
    BRA_OVERWRITE_ALWAYS_NO  = 2,
} bra_fs_overwrite_policy_e;
