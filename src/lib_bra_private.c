#include "lib_bra_private.h"

#include <utils/lib_bra_crc32c.h>
#include <log/bra_log.h>

#include <string.h>
#include <stdlib.h>
#include <assert.h>

////////////////////////////////////////////////////////////////////////////

uint64_t _bra_min(const uint64_t a, const uint64_t b)
{
    return a < b ? a : b;
}

char* _bra_strdup(const char* str)
{
    if (str == NULL)
        return NULL;

#ifdef __GNUC__
    return strdup(str);
#else

    const size_t sz = strlen(str) + 1;
    char*        c  = malloc(sz);

    if (c == NULL)
        return NULL;

    memcpy(c, str, sizeof(char) * sz);
    return c;
#endif
}

bool _bra_validate_filename(const char* fn, const size_t fn_size)
{
    // sanitize output path: reject absolute or parent traversal
    // POSIX absolute, Windows drive letter, and leading backslash
    if (fn[0] == BRA_DIR_DELIM[0] || fn[0] == '\\' ||
        (fn_size >= 2 &&
         ((fn[1] == ':' &&
           ((fn[0] >= 'A' && fn[0] <= 'Z') ||
            (fn[0] >= 'a' && fn[0] <= 'z'))))))
    {
        bra_log_error("absolute output path: %s", fn);
        return false;
    }
    // Reject common traversal patterns
    if (strstr(fn, "/../") != NULL || strstr(fn, "\\..\\") != NULL ||
        strncmp(fn, "../", 3) == 0 || strncmp(fn, "..\\", 3) == 0)
    {
        bra_log_error("invalid output path (contains '..'): %s", fn);
        return false;
    }

    return true;
}

void _bra_print_string_max_length(const char* buf, const int buf_length, const int max_length)
{
    if (buf_length > max_length)
        bra_log_printf("%.*s...", max_length - 3, buf);
    else
        bra_log_printf("%*.*s", -max_length, buf_length, buf);
}

bool _bra_compute_header_crc32(const size_t filename_len, const char* filename, bra_meta_entry_t* me)
{
    assert(filename != NULL);
    assert(me != NULL);

    if (filename_len > UINT16_MAX || filename_len == 0)
    {
        bra_log_critical("filename '%s' too long or empty %zu", filename, filename_len);
        return false;
    }

    const uint16_t filename_len_u16 = (uint16_t) filename_len;

    me->crc32 = bra_crc32c(&me->attributes, sizeof(bra_attr_t), BRA_CRC32C_INIT);
    me->crc32 = bra_crc32c(&filename_len_u16, sizeof(uint16_t), me->crc32);
    me->crc32 = bra_crc32c(filename, filename_len, me->crc32);

    return true;
}
