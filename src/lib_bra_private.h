#pragma once

#include <lib_bra_types.h>

#include <assert.h>
#include <stdbool.h>

//////////////////////////////////////////////////////////////////////////////////////////////////

#define assert_bra_io_file_t(x)     assert((x) != NULL && (x)->f != NULL && (x)->fn != NULL)
#define assert_bra_io_file_cxt_t(x) assert((x) != NULL && (x)->f.f != NULL && (x)->f.fn != NULL)

//////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Return the minimum of two unsigned 64-bit integers.
 *
 * @param a First value to compare.
 * @param b Second value to compare.
 * @return uint64_t The smaller of a and b
 */
uint64_t _bra_min(const uint64_t a, const uint64_t b);

/**
 * @brief strdup()
 *
 * @details Returns @c NULL if @p str is @c NULL or on allocation failure.
 *          The caller owns the returned buffer and must free() it.
 *
 * @todo remove when switching to C23
 *
 * @param str
 * @return char*
 */
char* _bra_strdup(const char* str);

/**
 * @brief Validate that a filename is safe for extraction.
 *
 * @param fn        filename to validate
 * @param fn_size   size of filename in bytes
 * @retval true     if the filename is safe (no absolute paths or directory traversal)
 * @retval false    if the filename contains dangerous patterns
 */
bool _bra_validate_filename(const char* fn, const size_t fn_size);

/**
 * @brief Log a string with fixed width, truncating with "..." if it exceeds max_length.
 *
 * @param buf           Input string (may be non-NUL-terminated).
 * @param buf_length    Number of valid bytes in buf.
 * @param max_length    Field width; if buf_length > max_length, prints max_length-3 chars + "...".
 */
void _bra_print_string_max_length(const char* buf, const int buf_length, const int max_length);

/**
 * @brief Compute the CRC32C checksum for the header of a metadata entry.
 *
 * @param filename_len Length of the filename.
 * @param filename     Pointer to the filename string.
 * @param me           Pointer to the metadata entry.
 * @retval true
 * @retval false
 */
bool _bra_compute_header_crc32(const size_t filename_len, const char* filename, bra_meta_entry_t* me);
