#pragma once


#ifdef __cplusplus
extern "C" {
#endif

#include <lib_bra_defs.h>
#include <lib_bra_types.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>


/////////////////////////////////////////////////////////////////////////////////////////////////////////

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
 * @brief Print a meta file @p f using @ref bra_log_printf to display attributes, size and filename.
 *
 * @param f Archive file whose meta will be printed.
 * @return true
 * @return false
 */
bool bra_io_print_meta_file_ctx(bra_io_file_ctx_t* ctx);

/**
 * @brief Free any eventual content on @p mf.
 *
 * @param mf
 */
void bra_meta_file_free(bra_meta_file_t* mf);


#ifdef __cplusplus
}
#endif
