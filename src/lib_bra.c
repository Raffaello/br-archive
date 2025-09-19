#include <lib_bra.h>
#include <io/lib_bra_io_file.h>
#include <io/lib_bra_io_file_ctx.h>

#include <lib_bra_private.h>

#include <fs/bra_fs_c.h>
#include <log/bra_log.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

/////////////////////////////////////////////////////////////////////////////

_Static_assert(BRA_ATTR_TYPE_FILE == 0, "BRA_ATTR_TYPE_FILE index changed");
_Static_assert(BRA_ATTR_TYPE_DIR == 1, "BRA_ATTR_TYPE_DIR index changed");
_Static_assert(BRA_ATTR_TYPE_SYM == 2, "BRA_ATTR_TYPE_SYM index changed");
_Static_assert(BRA_ATTR_TYPE_SUBDIR == 3, "BRA_ATTR_TYPE_SUBDIR index changed");

/////////////////////////////////////////////////////////////////////////////

char bra_format_meta_attributes(const bra_attr_t attributes)
{
    switch (BRA_ATTR_TYPE(attributes))
    {
    case BRA_ATTR_TYPE_FILE:
        return 'f';
    case BRA_ATTR_TYPE_DIR:
        return 'd';
    case BRA_ATTR_TYPE_SYM:
        return 's';
    case BRA_ATTR_TYPE_SUBDIR:
        return 'D';    // TODO: this should be the same of dir, but now for debugging...
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

void bra_meta_entry_free(bra_meta_entry_t* mf)
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
