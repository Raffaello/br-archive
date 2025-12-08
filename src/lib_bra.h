/**
 * @file lib_bra.h
 * @author Raffaello Bertini
 * @brief
 */
#pragma once


#ifdef __cplusplus
extern "C" {
#endif

#include <lib_bra_defs.h>
#include <lib_bra_types.h>

#include <utils/lib_bra_crc32c.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>


/////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Initialize the library.
 *
 * @retval true
 * @retval false
 */
bool bra_init(void);

/**
 * @brief Check if the CPU has SSE4.2 support.
 *
 * @retval true
 * @retval false
 */
bool bra_has_sse42(void);

/**
 * @brief Convert meta file @p attributes into a char.
 *
 * @param attributes
 * @return char
 */
char bra_format_meta_attributes(const bra_attr_t attributes);

/**
 * @brief Convert @p bytes into a human readable string stored in @p buf.
 *        @p buf must provide at least #BRA_PRINTF_FMT_BYTES_BUF_SIZE bytes.
 *
 * @param bytes
 * @param buf
 */
void bra_format_bytes(const size_t bytes, char buf[BRA_PRINTF_FMT_BYTES_BUF_SIZE]);

/**
 * @brief Initialize a metadata entry based on its attribute for the specific data to allocate.
 *
 * @param me
 * @param attr
 * @param filename
 * @param filename_size
 * @retval true
 * @retval false
 */
bool bra_meta_entry_init(bra_meta_entry_t* me, const bra_attr_t attr, const char* filename, const uint8_t filename_size);

/**
 * @brief Free any allocated content in @p me (e.g., me->name) and zero fields.
 *
 * @pre  @p me @c != @c NULL.
 *
 * @note Idempotent: safe to call multiple times; fields are zeroed after the first call.
 *
 * @param me
 */
void bra_meta_entry_free(bra_meta_entry_t* me);

/**
 * @brief Initialize a metadata entry for a regular file.
 *
 * @param me
 * @param data_size
 * @retval true
 * @retval false
 */
bool bra_meta_entry_file_init(bra_meta_entry_t* me, const uint64_t data_size);

/**
 * @brief Initialize a metadata entry for a subdirectory.
 *
 * @param me
 * @param parent_index
 * @retval true
 * @retval false
 */
bool bra_meta_entry_subdir_init(bra_meta_entry_t* me, const uint32_t parent_index);


#ifdef __cplusplus
}
#endif
