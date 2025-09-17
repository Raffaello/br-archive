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

_Static_assert(sizeof(bra_meta_file_attr_u) == sizeof(bra_attr_t), "bra_meta_file_attr_u must be exactly 1 byte.");

/////////////////////////////////////////////////////////////////////////////

char bra_format_meta_attributes(const bra_meta_file_attr_u attributes)
{
    switch (attributes.bra_meta_file_attr_t.type)
    {
    case BRA_ATTR_TYPE_FILE:
        return 'f';
    case BRA_ATTR_TYPE_DIR:
        return 'd';
    case BRA_ATTR_TYPE_SYM:
        return 's';
    case BRA_ATTR_TYPE_SUB_DIR:
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

void bra_meta_file_free(bra_meta_file_t* mf)
{
    assert(mf != NULL);

    mf->data_size       = 0;
    mf->name_size       = 0;
    mf->attributes.attr = 0;
    if (mf->name != NULL)
    {
        free(mf->name);
        mf->name = NULL;
    }
}

bool bra_io_print_meta_file_ctx(bra_io_file_ctx_t* ctx)
{
    assert(ctx != NULL);
    assert_bra_io_file_t(&ctx->f);

    bra_meta_file_t mf;
    char            bytes[BRA_PRINTF_FMT_BYTES_BUF_SIZE];

    if (!bra_io_file_ctx_read_meta_file(ctx, &mf))
        return false;

    const uint64_t ds   = mf.data_size;
    const char     attr = bra_format_meta_attributes(mf.attributes);
    bra_format_bytes(mf.data_size, bytes);

    bra_log_printf("|   %c  | %s | ", attr, bytes);
    _bra_print_string_max_length(mf.name, mf.name_size, BRA_PRINTF_FMT_FILENAME_MAX_LENGTH);

    // print last dir to understand internal structure
#ifndef NDEBUG
    bra_log_debug("| %s ", ctx->last_dir);
#endif

    bra_log_printf("|\n");
    bra_meta_file_free(&mf);
    // skip data content
    if (!bra_io_file_skip_data(&ctx->f, ds))
    {
        bra_io_file_seek_error(&ctx->f);
        return false;
    }

    return true;
}
