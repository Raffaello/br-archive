#include "bra_log.h"
#include "lib_bra_defs.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef __GNUC__
#include <unistd.h>    // for isatty
#elif defined(_WIN32) || defined(_WIN64)
#include <io.h>
#include <windows.h>
#endif

#ifndef NDEBUG
static bra_log_level_e g_log_level = BRA_LOG_LEVEL_DEBUG;
#else
static bra_log_level_e g_log_level = BRA_LOG_LEVEL_INFO;
#endif

static bool g_use_ansi_color;

#if !defined(__GNUC__) && (defined(_WIN32) || defined(_WIN64))
static bool g_log_isInit = false;
#endif

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

////////////////////////////////////////////////////////////////////////////

static bra_message_callback_f* g_msg_cb = vprintf;

///////////////////////////////////////////////////////////////////////////

// Function to be executed before main() (in GCC)
BRA_FUNC_ATTR_CONSTRUCTOR static void _init_bra_log()
{
#ifdef __GNUC__
    g_use_ansi_color = isatty(STDERR_FILENO) != 0;
#elif defined(_WIN32) || defined(_WIN64)
    g_use_ansi_color = _isatty(_fileno(stderr)) != 0;
    // enable ANSI VT sequences when available
    HANDLE h = GetStdHandle(STD_ERROR_HANDLE);
    if (h != INVALID_HANDLE_VALUE)
    {
        DWORD mode = 0;
        if (GetConsoleMode(h, &mode))
        {
            SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }
    }

    g_log_isInit = true;
#endif
}

static inline void _bra_log_set_no_color(void)
{
    fputs("\033[0m", stderr);
}

static inline void _bra_log_set_color(const int fg)
{
    fprintf(stderr, "\033[%dm", fg);
}

static inline void _bra_log_set_color2(const int fg, const int bg)
{
    fprintf(stderr, "\033[%d;%dm", fg, bg);
}

static inline void _bra_log_set_color3(const int fg, const int bg, const uint8_t special)
{
    fprintf(stderr, "\033[%d;%d;%dm", special, fg, bg);
}

static inline void _bra_log_set_ansi_color(const bra_log_level_e level)
{
    switch (level)
    {
    case BRA_LOG_LEVEL_VERBOSE:
        _bra_log_set_color(BRIGHT_COL(BLACK));
        break;
    case BRA_LOG_LEVEL_DEBUG:
        _bra_log_set_color(CYAN);
        break;
    case BRA_LOG_LEVEL_INFO:
        _bra_log_set_no_color();
        break;
    case BRA_LOG_LEVEL_WARN:
        _bra_log_set_color(YELLOW);
        break;
    case BRA_LOG_LEVEL_ERROR:
        _bra_log_set_color(RED);
        break;
    case BRA_LOG_LEVEL_CRITICAL:
        _bra_log_set_color3(BRIGHT_COL(WHITE), BG_COL(RED), UNDERLINE);
        break;
    default:
        break;
    }
}

////////////////////////////////////////////////////////////////////////////


void bra_set_message_callback(bra_message_callback_f* msg_cb)
{
    if (msg_cb == NULL)
        g_msg_cb = vprintf;
    else
        g_msg_cb = msg_cb;
}

int bra_vprintf(const char* fmt, va_list args)
{
    return g_msg_cb(fmt, args);
}

int bra_printf(const char* fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    const int res = bra_vprintf(fmt, args);
    va_end(args);

    return res;
}

void bra_log_verbose(const char* fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    bra_log_v(BRA_LOG_LEVEL_VERBOSE, fmt, args);
    va_end(args);
}

void bra_log_debug(const char* fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    bra_log_v(BRA_LOG_LEVEL_DEBUG, fmt, args);
    va_end(args);
}

void bra_log_info(const char* fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    bra_log_v(BRA_LOG_LEVEL_INFO, fmt, args);
    va_end(args);
}

void bra_log_warn(const char* fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    bra_log_v(BRA_LOG_LEVEL_WARN, fmt, args);
    va_end(args);
}

void bra_log_error(const char* fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    bra_log_v(BRA_LOG_LEVEL_ERROR, fmt, args);
    va_end(args);
}

void bra_log_critical(const char* fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    bra_log_v(BRA_LOG_LEVEL_CRITICAL, fmt, args);
    va_end(args);
}

void bra_log(const bra_log_level_e level, const char* fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    bra_log_v(level, fmt, args);
    va_end(args);
}

void bra_log_v(const bra_log_level_e level, const char* fmt, va_list args)
{
    if (g_log_level > level || level == BRA_LOG_LEVEL_QUIET)
        return;

#if !defined(__GNUC__) && (defined(_WIN32) || defined(_WIN64))
    if (!g_log_isInit)
        _init_bra_log();
#endif

    if (g_use_ansi_color)
        _bra_log_set_ansi_color(level);

    switch (level)
    {
    case BRA_LOG_LEVEL_VERBOSE:
        fputs("VERBOSE: ", stderr);
        break;
    case BRA_LOG_LEVEL_DEBUG:
        fputs("DEBUG: ", stderr);
        break;
    case BRA_LOG_LEVEL_INFO:
        fputs("INFO: ", stderr);
        break;
    case BRA_LOG_LEVEL_WARN:
        fputs("WARN: ", stderr);
        break;
    case BRA_LOG_LEVEL_ERROR:
        fputs("ERROR: ", stderr);
        break;
    case BRA_LOG_LEVEL_CRITICAL:
        fputs("CRITICAL: ", stderr);
        break;
    case BRA_LOG_LEVEL_QUIET:
        break;
    default:
        fputs("UNKNOWN: ", stderr);
        break;
    }

    vfprintf(stderr, fmt, args);

    if (g_use_ansi_color)
    {
        _bra_log_set_no_color();
        fputs("\033[K", stderr);
    }

    fputs("\n", stderr);
    fflush(stderr);
}

void bra_log_set_level(const bra_log_level_e level)
{
    g_log_level = level;
}

bra_log_level_e bra_log_get_level(void)
{
    return g_log_level;
}
