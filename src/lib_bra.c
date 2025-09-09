#include <lib_bra.h>
#include <bra_fs_c.h>
#include <bra_log.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#define assert_bra_io_file_t(x) assert((x) != NULL && (x)->f != NULL && (x)->fn != NULL)

_Static_assert(BRA_MAX_PATH_LENGTH > UINT8_MAX, "BRA_MAX_PATH_LENGTH must be greater than bra_meta_file_t.name_size max value");

/////////////////////////////////////////////////////////////////////////////

inline uint64_t bra_min(const uint64_t a, const uint64_t b)
{
    return a < b ? a : b;
}

char* bra_strdup(const char* str)
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

bool bra_validate_meta_filename(const bra_meta_file_t* mf)
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

char bra_format_meta_attributes(const bra_attr_t attributes)
{
    switch (attributes)
    {
    case BRA_ATTR_FILE:
        return 'f';
    case BRA_ATTR_DIR:
        return 'd';
    default:
        return '?';
    }
}

void bra_format_bytes(const size_t bytes, char buf[BRA_PRINTF_FMT_BYTES_BUF_SIZE])
{
    static const size_t KB = 1024;
    static const size_t MB = KB * 1024;
    static const size_t GB = MB * 1024;
    static const size_t TB = GB * 1024;
    static const size_t PB = TB * 1024;
    static const size_t EB = PB * 1024;    // fits in uint64_t up to ~16 EB (platform dependent)

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
#endif
    if (bytes >= EB)
        snprintf(buf, BRA_PRINTF_FMT_BYTES_BUF_SIZE, "%6.1f EB", (double) bytes / EB);
    else if (bytes >= PB)
        snprintf(buf, BRA_PRINTF_FMT_BYTES_BUF_SIZE, "%6.1f PB", (double) bytes / PB);
    else if (bytes >= TB)
        snprintf(buf, BRA_PRINTF_FMT_BYTES_BUF_SIZE, "%6.1f TB", (double) bytes / TB);
    else if (bytes >= GB)
        snprintf(buf, BRA_PRINTF_FMT_BYTES_BUF_SIZE, "%6.1f GB", (double) bytes / GB);
    else if (bytes >= MB)
        snprintf(buf, BRA_PRINTF_FMT_BYTES_BUF_SIZE, "%6.1f MB", (double) bytes / MB);
    else if (bytes >= KB)
        snprintf(buf, BRA_PRINTF_FMT_BYTES_BUF_SIZE, "%6.1f KB", (double) bytes / KB);
    else
        snprintf(buf, BRA_PRINTF_FMT_BYTES_BUF_SIZE, "%6zu  B", bytes);
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
}

void bra_meta_file_free(bra_meta_file_t* mf)
{
    assert(mf != NULL);

    mf->data_size  = 0;
    mf->name_size  = 0;
    mf->attributes = 0;
    if (mf->name != NULL)
    {
        free(mf->name);
        mf->name = NULL;
    }
}
