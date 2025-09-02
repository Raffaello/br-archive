#pragma once

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
    BRA_LOG_LEVEL_QUIET,       //!< Log level for quiet mode, where no messages are logged

} bra_log_level_e;

/**
 * @brief Define ANSI color codes https://en.wikipedia.org/wiki/ANSI_escape_code#SGR
 */
#define RESET       0
#define BOLD        1
#define FAINT       2
#define ITALIC      3
#define UNDERLINE   4
#define SLOW_BLINK  5
#define RAPID_BLINK 6
// #define INVERT      7

#define BLACK   30
#define RED     31
#define GREEN   32
#define YELLOW  33
#define BLUE    34
#define MAGENTA 35
#define CYAN    36
#define WHITE   37

#define BG_COL(x)        (x + 10)
#define BRIGHT_COL(x)    (x + 60)
#define BRIGHT_BG_COL(x) (BRIGHT_COL(BG_COL(x)))


void bra_log_verbose(const char* fmt, ...);
void bra_log_debug(const char* fmt, ...);
void bra_log_info(const char* fmt, ...);
void bra_log_warn(const char* fmt, ...);
void bra_log_error(const char* fmt, ...);
void bra_log_critical(const char* fmt, ...);

void bra_logger(const bra_log_level_e level, const char* fmt, ...);
void bra_log_v(const bra_log_level_e level, const char* fmt, va_list args);

void            bra_log_set_level(const bra_log_level_e level);
bra_log_level_e bra_log_get_level(void);
