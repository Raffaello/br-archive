#include "lib_bra_private.h"

#include <log/bra_log.h>

inline void _bra_print_string_max_length(const char* buf, const int buf_length, const int max_length)
{
    if (buf_length > max_length)
        bra_log_printf("%.*s...", max_length - 3, buf);
    else
        bra_log_printf("%*.*s", -max_length, buf_length, buf);
}
