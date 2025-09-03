#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "lib_bra_defs.h"

#include <stdarg.h>

/**
 * @brief enum values for the log levels.
 *
 * @details The log levels are used to categorize the importance of log messages.
 */
typedef enum bra_log_level_e
{
    BRA_LOG_LEVEL_VERBOSE,     //!< Log level for verbose messages, useful for detailed debugging information
    BRA_LOG_LEVEL_DEBUG,       //!< Log level for debug messages, useful for debugging purposes
    BRA_LOG_LEVEL_INFO,        //!< Log level for informational messages, useful for general information
    BRA_LOG_LEVEL_WARN,        //!< Log level for warning messages, useful for indicating potential issues
    BRA_LOG_LEVEL_ERROR,       //!< Log level for error messages, useful for reporting errors that occurred
    BRA_LOG_LEVEL_CRITICAL,    //!< Log level for critical messages, useful for reporting critical errors that may cause the program to terminate
    BRA_LOG_LEVEL_QUIET,       //!< Log level for no messages.

} bra_log_level_e;

/**
 * @brief Write a level #BRA_LOG_LEVEL_VERBOSE log of @p fmt.
 *
 * @see bra_log
 *
 * @param fmt
 * @param ...
 */
void bra_log_verbose(const char* fmt, ...) BRA_FUNC_ATTR_FMT_PRINTF(1, 2);

/**
 * @brief Write a level #BRA_LOG_LEVEL_DEBUG log of @p fmt.
 *
 * @see bra_log
 *
 * @param fmt
 * @param ...
 */
void bra_log_debug(const char* fmt, ...) BRA_FUNC_ATTR_FMT_PRINTF(1, 2);

/**
 * @brief Write a level #BRA_LOG_LEVEL_INFO log of @p fmt.
 *
 * @see bra_log
 *
 * @param fmt
 * @param ...
 */
void bra_log_info(const char* fmt, ...) BRA_FUNC_ATTR_FMT_PRINTF(1, 2);

/**
 * @brief Write a level #BRA_LOG_LEVEL_WARN log of @p fmt.
 *
 * @see bra_log
 *
 * @param fmt
 * @param ...
 */
void bra_log_warn(const char* fmt, ...) BRA_FUNC_ATTR_FMT_PRINTF(1, 2);

/**
 * @brief Write a level #BRA_LOG_LEVEL_ERROR log of @p fmt.
 *
 * @see bra_log
 *
 * @param fmt
 * @param ...
 */
void bra_log_error(const char* fmt, ...) BRA_FUNC_ATTR_FMT_PRINTF(1, 2);

/**
 * @brief Write a level #BRA_LOG_LEVEL_CRITICAL log of @p fmt.
 *
 * @see bra_log
 *
 * @param fmt
 * @param ...
 */
void bra_log_critical(const char* fmt, ...) BRA_FUNC_ATTR_FMT_PRINTF(1, 2);

/**
 * @brief Write to @c stderr the printf-like string @p fmt of the specified @p level.
 *        Support ANSI Colors when available.
 *
 * @param level
 * @param fmt
 * @param ...
 */
void bra_log(const bra_log_level_e level, const char* fmt, ...) BRA_FUNC_ATTR_FMT_PRINTF(2, 3);

/**
 * @brief Same as @ref bra_log but with a @c va_list of @p args.
 *
 * @see bra_log
 *
 * @param fmt
 * @param ...
 */
void bra_log_v(const bra_log_level_e level, const char* fmt, va_list args) BRA_FUNC_ATTR_FMT_PRINTF(2, 0);

/**
 * @brief Set the log level.
 *
 * @param level
 */
void bra_log_set_level(const bra_log_level_e level);

/**
 * @brief Get the log level.
 *
 * @return bra_log_level_e
 */
bra_log_level_e bra_log_get_level(void);

#ifdef __cplusplus
}
#endif
