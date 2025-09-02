#include "bra_log.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>    // for isatty

static bra_log_level_e g_log_level;
static bool            g_use_ansi_color;

////////////////////////////////////////////////////////////////////////////

// Function to be executed before main()
void __attribute__((constructor)) _init_bra_log()
{
    g_use_ansi_color = isatty(2) != 0;
#ifndef NDEBUG
    g_log_level = BRA_LOG_LEVEL_DEBUG;
#else
    g_log_level = BRA_LOG_LEVEL_INFO;
#endif
}

static inline void _bra_log_set_no_color(void)
{
    fprintf(stderr, "\033[0m");
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
        _bra_log_set_color2(BRIGHT_COL(WHITE), BG_COL(RED));
        // _bra_log_set_color(BRIGHT_COL(RED));
        _bra_log_set_color3(BRIGHT_COL(WHITE), BG_COL(RED), UNDERLINE);
        break;
    default:
        break;
    }
}

////////////////////////////////////////////////////////////////////////////

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

void bra_logger(const bra_log_level_e level, const char* fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    bra_log_v(level, fmt, args);
    va_end(args);
}

void bra_log_v(const bra_log_level_e level, const char* fmt, va_list args)
{
    if (g_log_level > level)
        return;

    // g_use_ansi_color = isatty(2) != 0;
    if (g_use_ansi_color)
        _bra_log_set_ansi_color(level);

    bra_logger(level, fmt, args);

    if (g_use_ansi_color)
    {
        _bra_log_set_no_color();
        fprintf(stderr, "\033[K");
    }
}

void bra_log_set_level(const bra_log_level_e level)
{
    g_log_level = level;
}

bra_log_level_e bra_log_get_level()
{
    return g_log_level;
}
