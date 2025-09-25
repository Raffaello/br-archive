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

bool bra_init(void)
{
    bra_log_init();
    bra_crc32c_use_sse42(true);

    return true;
}

bool bra_has_sse42(void)
{
#if defined(__GNUC__) || defined(__clang__)
    __builtin_cpu_init();

#if defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86)
    return __builtin_cpu_supports("sse4.2");
#else
    return false;
#endif    // defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86)CPU

#elif defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
    int info[4] = {0};
    __cpuidex(info, 1, 0);
    // ECX bit 20 indicates SSE4.2 support
    return (info[2] & (1 << 20)) != 0;
#else
    return false;
#endif    // defined(__GNUC__) || defined(__clang__)
}

char bra_format_meta_attributes(const bra_attr_t attributes)
{
    switch (BRA_ATTR_TYPE(attributes))
    {
    case BRA_ATTR_TYPE_FILE:
        return 'f';
    case BRA_ATTR_TYPE_DIR:
    // [[fallthrough]];
    case BRA_ATTR_TYPE_SUBDIR:
        return 'd';
    case BRA_ATTR_TYPE_SYM:
        return 's';
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

bool bra_meta_entry_init(bra_meta_entry_t* me, const bra_attr_t attr, const char* filename, const uint8_t filename_size)
{
    assert(me != NULL);

    me->attributes = attr;
    switch (BRA_ATTR_TYPE(attr))
    {
    case BRA_ATTR_TYPE_FILE:
        me->entry_data = malloc(sizeof(bra_meta_entry_file_t));
        if (me->entry_data == NULL)
            return false;
        break;
    case BRA_ATTR_TYPE_SUBDIR:
        me->entry_data = malloc(sizeof(bra_meta_entry_subdir_t));    // parent tree index
        if (me->entry_data == NULL)
            return false;
        break;
    case BRA_ATTR_TYPE_DIR:
    // [[fallthrough]];
    case BRA_ATTR_TYPE_SYM:
        me->entry_data = NULL;
        break;

    default:
        return false;
    }

    me->name_size = filename_size;
    if (filename != NULL)
    {
        me->name = _bra_strdup(filename);
        if (me->name == NULL)
        {
            bra_meta_entry_free(me);
            return false;
        }
    }
    else
        me->name = NULL;

    me->crc32 = BRA_CRC32C_INIT;
    return true;
}

void bra_meta_entry_free(bra_meta_entry_t* me)
{
    assert(me != NULL);

    if (me->entry_data != NULL)
    {
        free(me->entry_data);
        me->entry_data = NULL;
    }
    me->name_size  = 0;
    me->attributes = 0;
    me->crc32      = 0;
    if (me->name != NULL)
    {
        free(me->name);
        me->name = NULL;
    }
}

bool bra_meta_entry_file_init(bra_meta_entry_t* me, const uint64_t data_size)
{
    assert(me != NULL);

    if (BRA_ATTR_TYPE(me->attributes) != BRA_ATTR_TYPE_FILE)
        return false;

    bra_meta_entry_file_t* mef = me->entry_data;
    mef->data_size             = data_size;
    return true;
}

bool bra_meta_entry_subdir_init(bra_meta_entry_t* me, const uint32_t parent_index)
{
    assert(me != NULL);

    if (BRA_ATTR_TYPE(me->attributes) != BRA_ATTR_TYPE_SUBDIR)
        return false;

    bra_meta_entry_subdir_t* mes = me->entry_data;
    mes->parent_index            = parent_index;
    return true;
}
