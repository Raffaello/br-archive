#pragma once

#include <lib_bra_types.h>

#include <assert.h>
#include <stdbool.h>

//////////////////////////////////////////////////////////////////////////////////////////////////

#define assert_bra_io_file_t(x) assert((x) != NULL && (x)->f != NULL && (x)->fn != NULL)

//////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Return the minimum of two unsigned 64-bit integers.s
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
 * @brief Validate that a meta filename is safe for extraction.
 *
 * @param mf Meta file structure containing the filename to validate
 * @return true if the filename is safe (no absolute paths or directory traversal)
 * @return false if the filename contains dangerous patterns
 */
bool _bra_validate_meta_filename(const bra_meta_file_t* mf);

/**
 * @brief
 *
 * @param buf
 * @param buf_length
 * @param max_length
 */
void _bra_print_string_max_length(const char* buf, const int buf_length, const int max_length);
