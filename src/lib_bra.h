#pragma once


#ifdef __cplusplus
extern "C" {
#endif

#include "lib_bra_defs.h"
#include "lib_bra_types.h"

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
 * @param f
 * @param bh
 * @return true
 * @return false
 */
bool bra_io_print_meta_file(bra_io_file_t* f);

/**
 * @brief Free any eventual content on @p mf.
 *
 * @param mf
 */
void bra_meta_file_free(bra_meta_file_t* mf);


/////////////////////////////////////////////////////////////////////////////////

// /**
//  * @brief
//  *
//  * @param buf
//  * @param buf_size
//  * @param num_rle_chunks this is not really required.. but *2 give the total size in bytes... so know the compression size
//  * @param chunk_head
//  * @return true
//  * @return false
//  */
// bool bra_encode_rle(const char* buf, const size_t buf_size, uint64_t* num_rle_chunks, bra_rle_chunk_t** chunk_head);

// /**
//  * @brief
//  *
//  * @param rle
//  * @return true
//  * @return false
//  */
// bool bra_encode_rle_free_list(bra_rle_chunk_t** rle);

// /**
//  * @brief
//  *
//  * @param cur
//  * @param buf
//  * @param buf_size
//  * @param buf_i
//  * @return true
//  * @return false
//  */
// bool bra_decode_rle(bra_rle_chunk_t** cur, char* buf, const size_t buf_size, size_t* buf_i);

#ifdef __cplusplus
}
#endif
