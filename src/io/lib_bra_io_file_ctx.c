#include <io/lib_bra_io_file_ctx.h>
#include <lib_bra_defs.h>
#include <lib_bra_private.h>
#include <io/lib_bra_io_file.h>
#include <log/bra_log.h>
#include <fs/bra_fs_c.h>
#include <lib_bra.h>
#include <bra_tree_dir.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static const char  g_end_messages[][5] = {" OK ", "SKIP"};
static const char* g_attr_type_names[] = {"file", "dir", "symlink", "subdir"};

///////////////////////////////////////////////////////////////////////////////////////////////////////

static char* _bra_io_file_ctx_reconstruct_meta_entry_name(bra_io_file_ctx_t* ctx, bra_meta_entry_t* me, size_t* len)
{
    assert_bra_io_file_cxt_t(ctx);
    assert(me != NULL);

    char* dirname = bra_tree_dir_reconstruct_path(ctx->last_dir_node);
    if (dirname == NULL)
        return NULL;

    const size_t dirname_len = strlen(dirname);
    char*        fn          = NULL;

    switch (BRA_ATTR_TYPE(me->attributes))
    {
    case BRA_ATTR_TYPE_FILE:
    {
        if (dirname_len > 0)
        {
            fn = malloc((dirname_len + 1 + me->name_size + 1) * sizeof(char));    // +1 for '/' and +1 for '\0'
            if (fn == NULL)
                goto _BRA_IO_FILE_CTX_RECONSTRUCT_META_ENTRY_NAME_EXIT;

            memcpy(fn, dirname, dirname_len);
            fn[dirname_len] = BRA_DIR_DELIM[0];
            memcpy(&fn[dirname_len + 1], me->name, me->name_size);
            fn[dirname_len + 1 + me->name_size] = '\0';
            if (len != NULL)
                *len = dirname_len + me->name_size + 1;
        }
        else
        {
            fn = _bra_strdup(me->name);
            if (fn == NULL)
                goto _BRA_IO_FILE_CTX_RECONSTRUCT_META_ENTRY_NAME_EXIT;
            if (len != NULL)
                *len = me->name_size;
        }

    _BRA_IO_FILE_CTX_RECONSTRUCT_META_ENTRY_NAME_EXIT:
        free(dirname);
        return fn;
    }
    break;
    case BRA_ATTR_TYPE_DIR:
    // [[fallthrough]];
    case BRA_ATTR_TYPE_SUBDIR:
    {
        if (len != NULL)
            *len = dirname_len;

        return dirname;
    }
    break;
    }

    return NULL;
}

static bool _bra_io_file_ctx_write_meta_entry_common(bra_io_file_ctx_t* ctx, const bra_attr_t attr, const char* filename, const uint8_t filename_size)
{
    assert_bra_io_file_cxt_t(ctx);
    assert(filename != NULL);
    assert(filename_size > 0);

    // 1. attributes
    if (fwrite(&attr, sizeof(bra_attr_t), 1, ctx->f.f) != 1)
        return false;

    // 2. filename size
    if (fwrite(&filename_size, sizeof(uint8_t), 1, ctx->f.f) != 1)
        return false;

    // 3. filename
    if (fwrite(filename, sizeof(char), filename_size, ctx->f.f) != filename_size)
        return false;

    ++ctx->cur_files;    // all meta files are using this function, so best place to track an added file to the archive.
    return true;
}

static bool _bra_io_file_ctx_flush_type_dir(bra_io_file_ctx_t* ctx)
{
    assert_bra_io_file_cxt_t(ctx);
    assert(ctx->last_dir_node != NULL);
    assert(ctx->last_dir_node->parent != NULL);    // not root

    const size_t len = strnlen(ctx->last_dir_node->dirname, BRA_MAX_PATH_LENGTH);
    if (len == 0)
    {
        bra_log_critical("empty dirname for index %u", ctx->last_dir_node->index);
        return false;
    }
    else if (len > UINT8_MAX)
    {
        bra_log_critical("dirname %s too long %zu", ctx->last_dir_node->dirname, len);
        return false;
    }

    if (!_bra_io_file_ctx_write_meta_entry_common(ctx, ctx->last_dir_attr, ctx->last_dir_node->dirname, len))
        return false;

    return true;
}

static bool _bra_io_file_ctx_flush_dir(bra_io_file_ctx_t* ctx)
{
    assert_bra_io_file_cxt_t(ctx);

    switch (BRA_ATTR_TYPE(ctx->last_dir_attr))
    {
    case BRA_ATTR_TYPE_DIR:
    {
        return _bra_io_file_ctx_flush_type_dir(ctx);
    }
    break;
    case BRA_ATTR_TYPE_SUBDIR:
    {
        if (!_bra_io_file_ctx_flush_type_dir(ctx))
            return false;

        bra_meta_entry_subdir_t me_subdir;
        me_subdir.parent_index = ctx->last_dir_node->parent->index;
        if (fwrite(&me_subdir.parent_index, sizeof(uint32_t), 1, ctx->f.f) != 1)
            return false;
    }
    break;

    default:
        break;
    }

    return true;
}

static bool _bra_io_file_ctx_read_meta_entry_read_file(bra_io_file_ctx_t* ctx, bra_meta_entry_t* me, const char* buf, const uint8_t buf_size)
{
    assert_bra_io_file_cxt_t(ctx);
    assert(me != NULL);
    assert(me->entry_data != NULL);
    assert(buf != NULL);

    if (BRA_ATTR_TYPE(me->attributes) != BRA_ATTR_TYPE_FILE)
        return false;

    bra_meta_entry_file_t* mef = me->entry_data;
    if (fread(&mef->data_size, sizeof(uint64_t), 1, ctx->f.f) != 1)
        return false;

    me->name_size = (uint8_t) buf_size;
    me->name      = _bra_strdup(buf);
    if (me->name == NULL)
        return false;

    return true;
}

static bool _bra_io_file_ctx_read_meta_entry_read_dir(bra_meta_entry_t* me, const char* buf, const uint8_t buf_size)
{
    assert(me != NULL);
    assert(buf_size > 0);

    me->name_size = buf_size;
    me->name      = _bra_strdup(buf);
    if (me->name == NULL)
        return false;

    return true;
}

static bool _bra_io_file_ctx_read_meta_entry_read_subdir(bra_io_file_ctx_t* ctx, bra_meta_entry_t* me, const char* buf, const uint8_t buf_size)
{
    if (!_bra_io_file_ctx_read_meta_entry_read_dir(me, buf, buf_size))
        return false;

    // read parent index
    bra_meta_entry_subdir_t* mes = me->entry_data;
    if (fread(&mes->parent_index, sizeof(uint32_t), 1, ctx->f.f) != 1)
        return false;

    return true;
}

static bool _bra_io_file_ctx_write_meta_entry_process_write_file(bra_io_file_ctx_t* ctx, const bra_attr_t attributes, const char* filename)
{
    assert_bra_io_file_cxt_t(ctx);
    assert(filename != NULL);

    if (BRA_ATTR_TYPE(attributes) != BRA_ATTR_TYPE_FILE)
        return false;

    bra_meta_entry_t me;
    size_t           l = ctx->last_dir_size;    // strnlen(g_last_dir, BRA_MAX_PATH_LENGTH);
    if (filename[l] == BRA_DIR_DELIM[0])        // or when l == 0
        ++l;                                    // skip also '/'
    else if (ctx->last_dir_size > 0)
    {
        bra_log_critical("me->name: %s -- last_dir: %s", filename, ctx->last_dir);
        return false;
    }

    if (!bra_meta_entry_init(&me, attributes, &filename[l], strlen(&filename[l])))
        return false;

    uint64_t ds;
    if (!bra_fs_file_size(filename, &ds))
        goto BRA_IO_FILE_CTX_WRITE_META_ENTRY_PROCESS_WRITE_FILE_ERR;

    if (!bra_meta_entry_file_init(&me, ds))
        goto BRA_IO_FILE_CTX_WRITE_META_ENTRY_PROCESS_WRITE_FILE_ERR;

    // write common meta data (attribute, filename, filename_size)
    if (!_bra_io_file_ctx_write_meta_entry_common(ctx, attributes, me.name, (uint8_t) me.name_size))
        goto BRA_IO_FILE_CTX_WRITE_META_ENTRY_PROCESS_WRITE_FILE_ERR;

    // 3. file size
    const bra_meta_entry_file_t* mef = (const bra_meta_entry_file_t*) me.entry_data;
    assert(mef != NULL);
    if (fwrite(&mef->data_size, sizeof(uint64_t), 1, ctx->f.f) != 1)
        goto BRA_IO_FILE_CTX_WRITE_META_ENTRY_PROCESS_WRITE_FILE_ERR;

    // 4. file content
    bra_io_file_t f2;
    memset(&f2, 0, sizeof(bra_io_file_t));
    if (!bra_io_file_open(&f2, filename, "rb"))
        goto BRA_IO_FILE_CTX_WRITE_META_ENTRY_PROCESS_WRITE_FILE_ERR;

    if (!bra_io_file_copy_file_chunks(&ctx->f, &f2, mef->data_size))
        goto BRA_IO_FILE_CTX_WRITE_META_ENTRY_PROCESS_WRITE_FILE_ERR;

    bra_io_file_close(&f2);
    bra_meta_entry_free(&me);
    return true;

BRA_IO_FILE_CTX_WRITE_META_ENTRY_PROCESS_WRITE_FILE_ERR:
    bra_meta_entry_free(&me);
    return false;
}

static bool _bra_io_file_ctx_write_meta_entry_process_write_dir_subdir(bra_io_file_ctx_t* ctx, const bra_attr_t attributes, const char* dirname)
{
    assert_bra_io_file_cxt_t(ctx);
    assert(dirname != NULL);

    assert(ctx->last_dir_node != NULL);

    if (BRA_ATTR_TYPE(attributes) != BRA_ATTR_TYPE_SUBDIR && BRA_ATTR_TYPE(attributes) != (BRA_ATTR_TYPE_DIR))
        return false;

    bra_tree_node_t* node = bra_tree_dir_add(ctx->tree, dirname);
    if (node == NULL)
    {
        bra_log_error("unable to add dir %s to bra_tree", dirname);
        return false;
    }

    bra_meta_entry_t me;
    if (!bra_meta_entry_init(&me, attributes, node->dirname, strlen(node->dirname)))
        return false;

    if (node->parent->index == 0)
    {
        // parent is root
        me.attributes = BRA_ATTR_SET_TYPE(attributes, BRA_ATTR_TYPE_DIR);
    }
    else
    {
        me.attributes = BRA_ATTR_SET_TYPE(attributes, BRA_ATTR_TYPE_SUBDIR);
    }

    if (BRA_ATTR_TYPE(attributes) == BRA_ATTR_TYPE_SUBDIR)
    {
        if (!bra_meta_entry_subdir_init(&me, node->index))
            goto _BRA_IO_FILE_CTX_WRITE_META_ENTRY_PROCESS_WRITE_SUBDIR_ERR;
    }

    ctx->last_dir_attr = attributes;
    ctx->last_dir_node = node;
    if (ctx->last_dir != NULL)
    {
        free(ctx->last_dir);
        ctx->last_dir = NULL;
    }

    ctx->last_dir = _bra_strdup(dirname);
    if (ctx->last_dir == NULL)
        goto _BRA_IO_FILE_CTX_WRITE_META_ENTRY_PROCESS_WRITE_SUBDIR_ERR;

    ctx->last_dir_size = strlen(ctx->last_dir);
    if (!_bra_io_file_ctx_flush_dir(ctx))
        goto _BRA_IO_FILE_CTX_WRITE_META_ENTRY_PROCESS_WRITE_SUBDIR_ERR;

    bra_meta_entry_free(&me);
    return true;

_BRA_IO_FILE_CTX_WRITE_META_ENTRY_PROCESS_WRITE_SUBDIR_ERR:
    bra_meta_entry_free(&me);
    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

bool bra_io_file_ctx_open(bra_io_file_ctx_t* ctx, const char* fn, const char* mode)
{
    assert(ctx != NULL);
    assert(fn != NULL);
    assert(mode != NULL);

    memset(ctx, 0, sizeof(bra_io_file_ctx_t));
    ctx->tree = bra_tree_dir_create();
    if (ctx->tree == NULL)
        return false;
    ctx->last_dir = _bra_strdup("");
    if (ctx->last_dir == NULL)
        goto BRA_IO_FILE_CTX_OPEN_ERR;

    const bool res = bra_io_file_open(&ctx->f, fn, mode);
    if (!res)
        goto BRA_IO_FILE_CTX_OPEN_ERR;
    else
        ctx->last_dir_node = ctx->tree->root;    // root dir

    return res;

BRA_IO_FILE_CTX_OPEN_ERR:
    if (ctx->tree != NULL)
    {
        bra_tree_dir_destroy(&ctx->tree);
        ctx->tree = NULL;
    }
    if (ctx->last_dir != NULL)
    {
        free(ctx->last_dir);
        ctx->last_dir = NULL;
    }

    return false;
}

bool bra_io_file_ctx_sfx_open(bra_io_file_ctx_t* ctx, const char* fn, const char* mode)
{
    if (!bra_io_file_ctx_open(ctx, fn, mode))
        return false;

    // check if it can have the footer
    if (!bra_io_file_seek(&ctx->f, 0, SEEK_END))
    {
        bra_io_file_seek_error(&ctx->f);
        return false;
    }
    if (bra_io_file_tell(&ctx->f) < (int64_t) sizeof(bra_io_footer_t))
    {
        bra_log_error("%s-SFX module too small (missing footer placeholder): %s", BRA_NAME, ctx->f.fn);
        bra_io_file_close(&ctx->f);
        return false;
    }

    // Position at the footer start
    if (!bra_io_file_seek(&ctx->f, -1L * (int64_t) sizeof(bra_io_footer_t), SEEK_END))
    {
        bra_io_file_seek_error(&ctx->f);
        return false;
    }

    return true;
}

bool bra_io_file_ctx_close(bra_io_file_ctx_t* ctx)
{
    assert(ctx != NULL);

    bool res = true;

    if (ctx->tree != NULL)
    {
#ifndef NDEBUG
        // print the tree
        bra_tree_node_print(ctx->tree->root);
#endif

        bra_tree_dir_destroy(&ctx->tree);
        ctx->tree = NULL;
    }

    if (ctx->last_dir != NULL)
    {
        free(ctx->last_dir);
        ctx->last_dir = NULL;
    }

    bra_io_file_close(&ctx->f);
    return res;
}

bool bra_io_file_ctx_read_header(bra_io_file_ctx_t* ctx, bra_io_header_t* out_bh)
{
    assert_bra_io_file_cxt_t(ctx);
    assert(out_bh != NULL);

    if (fread(out_bh, sizeof(bra_io_header_t), 1, ctx->f.f) != 1)
    {
        bra_io_file_read_error(&ctx->f);
        return false;
    }

    // check header magic
    if (out_bh->magic != BRA_MAGIC)
    {
        bra_log_error("Not valid %s file: %s", BRA_NAME, ctx->f.fn);
        bra_io_file_close(&ctx->f);
        return false;
    }

    ctx->num_files = out_bh->num_files;
    return true;
}

bool bra_io_file_ctx_write_header(bra_io_file_ctx_t* ctx, const uint32_t num_files)
{
    assert_bra_io_file_cxt_t(ctx);

    const bra_io_header_t header = {
        .magic = BRA_MAGIC,
        // .version   = BRA_ARCHIVE_VERSION,
        .num_files = num_files,
    };

    if (fwrite(&header, sizeof(bra_io_header_t), 1, ctx->f.f) != 1)
    {
        bra_io_file_write_error(&ctx->f);
        return false;
    }

    ctx->num_files = num_files;
    return true;
}

bool bra_io_file_ctx_sfx_open_and_read_footer_header(const char* fn, bra_io_header_t* out_bh, bra_io_file_ctx_t* ctx)
{
    assert(fn != NULL);
    assert(out_bh != NULL);
    assert(ctx != NULL);

    if (!bra_io_file_ctx_sfx_open(ctx, fn, "rb"))
        return false;

    bra_io_footer_t bf;
    bf.header_offset = 0;
    bf.magic         = 0;

    if (!bra_io_file_read_footer(&ctx->f, &bf))
        return false;

    // Ensure header_offset is before the footer and that the header fits.
    const int64_t footer_start = bra_io_file_tell(&ctx->f) - (int64_t) sizeof(bra_io_footer_t);
    if (footer_start <= 0 ||
        bf.header_offset <= 0 ||
        bf.header_offset >= footer_start - (int64_t) sizeof(bra_io_header_t))
    {
        bra_log_error("corrupted or not valid %s-SFX file (header_offset out of bounds): %s",
                      BRA_NAME,
                      ctx->f.fn);
        bra_io_file_close(&ctx->f);
        return false;
    }

    // read header and check
    if (!bra_io_file_seek(&ctx->f, bf.header_offset, SEEK_SET))
    {
        bra_io_file_seek_error(&ctx->f);
        return false;
    }

    if (!bra_io_file_ctx_read_header(ctx, out_bh))
        return false;

    return true;
}

bool bra_io_file_ctx_read_meta_entry(bra_io_file_ctx_t* ctx, bra_meta_entry_t* me)
{
    assert_bra_io_file_cxt_t(ctx);
    assert(me != NULL);

    char     buf[BRA_MAX_PATH_LENGTH];
    unsigned buf_size = 0;

    me->name      = NULL;
    me->name_size = 0;

    // 1. attributes
    if (fread(&me->attributes, sizeof(bra_attr_t), 1, ctx->f.f) != 1)
    {
    BRA_IO_READ_ERR:
        bra_io_file_read_error(&ctx->f);
        return false;
    }

    // 2. filename size
    if (fread(&buf_size, sizeof(uint8_t), 1, ctx->f.f) != 1)
        goto BRA_IO_READ_ERR;
    if (buf_size == 0 || buf_size >= BRA_MAX_PATH_LENGTH)
        goto BRA_IO_READ_ERR;

    // 3. filename
    if (fread(buf, sizeof(char), buf_size, ctx->f.f) != buf_size)
        goto BRA_IO_READ_ERR;

    buf[buf_size] = '\0';

    // 4. entry data
    switch (BRA_ATTR_TYPE(me->attributes))
    {
    case BRA_ATTR_TYPE_DIR:
    {
        if (!_bra_io_file_ctx_read_meta_entry_read_dir(me, buf, buf_size))
            return false;

        ctx->last_dir_node = bra_tree_dir_insert_at_parent(ctx->tree, 0, me->name);
        if (ctx->last_dir_node == NULL)
            goto BRA_IO_READ_ERR;
    }
    break;
    case BRA_ATTR_TYPE_SUBDIR:
    {
        if (!bra_meta_entry_subdir_init(me, 0))
            goto BRA_IO_READ_ERR;
        if (!_bra_io_file_ctx_read_meta_entry_read_subdir(ctx, me, buf, (uint8_t) buf_size))
            goto BRA_IO_READ_ERR;

        ctx->last_dir_node = bra_tree_dir_insert_at_parent(ctx->tree, ((bra_meta_entry_subdir_t*) me->entry_data)->parent_index, me->name);
        if (ctx->last_dir_node == NULL)
            goto BRA_IO_READ_ERR;
    }
    break;
    case BRA_ATTR_TYPE_FILE:
    {
        if (!bra_meta_entry_file_init(me, 0))
            goto BRA_IO_READ_ERR;
        if (!_bra_io_file_ctx_read_meta_entry_read_file(ctx, me, buf, (uint8_t) buf_size))
            goto BRA_IO_READ_ERR;
    }
    break;
    case BRA_ATTR_TYPE_SYM:
        bra_log_critical("SYMLINK NOT IMPLEMENTED YET");
        return false;
        break;
    default:
        return false;
    }

    return true;
}

bool bra_io_file_ctx_write_meta_entry(bra_io_file_ctx_t* ctx, const bra_attr_t attributes, const char* fn)
{
    assert_bra_io_file_cxt_t(ctx);

    // Processing & Writing data
    switch (BRA_ATTR_TYPE(attributes))
    {
    case BRA_ATTR_TYPE_FILE:
    {
        if (!_bra_io_file_ctx_write_meta_entry_process_write_file(ctx, attributes, fn))
            goto BRA_IO_WRITE_ERR;
    }
    break;
    case BRA_ATTR_TYPE_DIR:
        if (!_bra_io_file_ctx_write_meta_entry_process_write_dir_subdir(ctx, attributes, fn))
            goto BRA_IO_WRITE_ERR;
        break;
    case BRA_ATTR_TYPE_SUBDIR:
    {
        if (!_bra_io_file_ctx_write_meta_entry_process_write_dir_subdir(ctx, attributes, fn))
            goto BRA_IO_WRITE_ERR;
    }
    break;
    case BRA_ATTR_TYPE_SYM:
        bra_log_critical("SYMLINK NOT IMPLEMENTED YET");
        // goto BRA_IO_WRITE_ERR; // fallthrough
    default:
        goto BRA_IO_WRITE_ERR;
    }

    return true;

BRA_IO_WRITE_ERR:
    bra_io_file_write_error(&ctx->f);
    return false;
}

bool bra_io_file_ctx_encode_and_write_to_disk(bra_io_file_ctx_t* ctx, const char* fn)
{
    assert_bra_io_file_cxt_t(ctx);
    assert(fn != NULL);
    assert(ctx->last_dir != NULL);

    // get entry attributes
    bra_attr_t attributes;
    if (!bra_fs_file_attributes(ctx->last_dir, fn, &attributes))
    {
        bra_log_error("%s has unknown attribute", fn);
        bra_io_file_close(&ctx->f);
        return false;
    }

    bra_log_printf("Archiving %-7s:  ", g_attr_type_names[BRA_ATTR_TYPE(attributes)]);
    _bra_print_string_max_length(fn, strlen(fn), BRA_PRINTF_FMT_FILENAME_MAX_LENGTH);

    if (!bra_io_file_ctx_write_meta_entry(ctx, attributes, fn))
        return false;    // f closed already

    bra_log_printf(" [  %-4.4s  ]\n", g_end_messages[0]);
    return true;
}

bool bra_io_file_ctx_decode_and_write_to_disk(bra_io_file_ctx_t* ctx, bra_fs_overwrite_policy_e* overwrite_policy)
{
    assert_bra_io_file_cxt_t(ctx);
    assert(overwrite_policy != NULL);

    const char*      end_msg;    // 'OK  ' | 'SKIP'
    char*            fn = NULL;
    bra_meta_entry_t me;
    if (!bra_meta_entry_init(&me, 0, NULL, 0))
        return false;

    if (!bra_io_file_ctx_read_meta_entry(ctx, &me))
        goto BRA_IO_DECODE_ERR;

    // the full path name
    fn = _bra_io_file_ctx_reconstruct_meta_entry_name(ctx, &me, NULL);
    if (fn == NULL)
        goto BRA_IO_DECODE_ERR;

    if (!_bra_validate_filename(fn, strlen(fn)))
        goto BRA_IO_DECODE_ERR;

    // 4. read and write in chunk data
    // NOTE: nothing to extract for a directory, but only to create it
    switch (BRA_ATTR_TYPE(me.attributes))
    {
    case BRA_ATTR_TYPE_FILE:
    {
        const bra_meta_entry_file_t* mef = (const bra_meta_entry_file_t*) me.entry_data;
        assert(mef != NULL);
        const uint64_t ds = mef->data_size;
        if (!bra_fs_file_exists_ask_overwrite(fn, overwrite_policy, false))
        {
            end_msg = g_end_messages[1];
            bra_log_printf("Skipping file:   " BRA_PRINTF_FMT_FILENAME, fn);
            bra_meta_entry_free(&me);
            if (!bra_io_file_skip_data(&ctx->f, ds))
            {
                bra_io_file_seek_error(&ctx->f);
                goto BRA_IO_DECODE_ERR;
            }
        }
        else
        {
            bra_io_file_t f2;
            end_msg = g_end_messages[0];
            bra_log_printf("Extracting file: " BRA_PRINTF_FMT_FILENAME, fn);
            // NOTE: the directory must have been created in the previous entry,
            //       otherwise this will fail to create the file.
            //       The archive ensures the last used directory is created first,
            //       and then its files follow.
            //       So, no need to create the parent directory for each file each time.
            if (!bra_io_file_open(&f2, fn, "wb"))
                goto BRA_IO_DECODE_ERR;

            bra_meta_entry_free(&me);
            if (!bra_io_file_copy_file_chunks(&f2, &ctx->f, ds))
                goto BRA_IO_DECODE_ERR;

            bra_io_file_close(&f2);
        }
    }
    break;
    case BRA_ATTR_TYPE_DIR:
    // [[fallthrough]];
    case BRA_ATTR_TYPE_SUBDIR:
    {
        if (bra_fs_dir_exists(fn))
        {
            end_msg = g_end_messages[1];
            bra_log_printf("Dir exists:   " BRA_PRINTF_FMT_FILENAME, fn);
        }
        else
        {
            end_msg = g_end_messages[0];
            bra_log_printf("Creating dir: " BRA_PRINTF_FMT_FILENAME, fn);

            if (!bra_fs_dir_make(fn))
                goto BRA_IO_DECODE_ERR;
        }

        bra_meta_entry_free(&me);
    }
    break;
    case BRA_ATTR_TYPE_SYM:
        bra_log_critical("SYMLINK NOT IMPLEMENTED YET");
        // fallthrough
    default:
        goto BRA_IO_DECODE_ERR;
        break;
    }

    free(fn);
    bra_log_printf(" [  %-4.4s  ]\n", end_msg);
    return true;

BRA_IO_DECODE_ERR:
    if (fn != NULL)
        free(fn);

    bra_meta_entry_free(&me);
    bra_io_file_error(&ctx->f, "decode");
    return false;
}

bool bra_io_file_ctx_print_meta_entry(bra_io_file_ctx_t* ctx)
{
    assert(ctx != NULL);
    assert_bra_io_file_t(&ctx->f);

    char             bytes[BRA_PRINTF_FMT_BYTES_BUF_SIZE];
    bra_meta_entry_t me;
    if (!bra_meta_entry_init(&me, 0, NULL, 0))
        return false;

    if (!bra_io_file_ctx_read_meta_entry(ctx, &me))
        goto BRA_IO_FILE_CTX_PRINT_META_ENTRY_ERR;

    const uint64_t ds   = BRA_ATTR_TYPE(me.attributes) == BRA_ATTR_TYPE_FILE ? ((bra_meta_entry_file_t*) me.entry_data)->data_size : 0;
    const char     attr = bra_format_meta_attributes(me.attributes);
    size_t         len  = 0;
    char*          fn   = _bra_io_file_ctx_reconstruct_meta_entry_name(ctx, &me, &len);
    if (fn == NULL)
        goto BRA_IO_FILE_CTX_PRINT_META_ENTRY_ERR;

    bra_format_bytes(ds, bytes);
    bra_log_printf("|   %c  | %s | ", attr, bytes);
    _bra_print_string_max_length(fn, len, BRA_PRINTF_FMT_FILENAME_MAX_LENGTH);
    free(fn);

    bra_log_printf("|\n");
    bra_meta_entry_free(&me);
    // skip data content
    if (!bra_io_file_skip_data(&ctx->f, ds))
    {
        bra_io_file_seek_error(&ctx->f);
        goto BRA_IO_FILE_CTX_PRINT_META_ENTRY_ERR;
    }

    return true;

BRA_IO_FILE_CTX_PRINT_META_ENTRY_ERR:
    bra_meta_entry_free(&me);
    return false;
}
