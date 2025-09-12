#include "lib_bra_private.h"

#include <log/bra_log.h>

#include <string.h>
#include <stdlib.h>

////////////////////////////////////////////////////////////////////////////

inline uint64_t _bra_min(const uint64_t a, const uint64_t b)
{
    return a < b ? a : b;
}

char* _bra_strdup(const char* str)
{
    if (str == NULL)
        return NULL;

    const size_t sz = strlen(str) + 1;
    char*        c  = malloc(sz);

    if (c == NULL)
        return NULL;

    memcpy(c, str, sizeof(char) * sz);
    return c;
}

bool _bra_validate_meta_filename(const bra_meta_file_t* mf)
{
    // sanitize output path: reject absolute or parent traversal
    // POSIX absolute, Windows drive letter, and leading backslash
    if (mf->name[0] == '/' || mf->name[0] == '\\' ||
        (mf->name_size >= 2 &&
         ((mf->name[1] == ':' &&
           ((mf->name[0] >= 'A' && mf->name[0] <= 'Z') ||
            (mf->name[0] >= 'a' && mf->name[0] <= 'z'))))))
    {
        bra_log_error("absolute output path: %s", mf->name);
        return false;
    }
    // Reject common traversal patterns
    if (strstr(mf->name, "/../") != NULL || strstr(mf->name, "\\..\\") != NULL ||
        strncmp(mf->name, "../", 3) == 0 || strncmp(mf->name, "..\\", 3) == 0)
    {
        bra_log_error("invalid output path (contains '..'): %s", mf->name);
        return false;
    }

    return true;
}

inline void _bra_print_string_max_length(const char* buf, const int buf_length, const int max_length)
{
    if (buf_length > max_length)
        bra_log_printf("%.*s...", max_length - 3, buf);
    else
        bra_log_printf("%*.*s", -max_length, buf_length, buf);
}
