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

#ifndef NDEBUG
static void _bra_tree_node_print(bra_tree_node_t* node)
{
    while (node != NULL)
    {
        bra_log_debug("tree: [%u] %s", node->index, node->dirname);

        _bra_tree_node_print(node->firstChild);

        node = node->next;
    }
}

#endif

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

static bool _bra_io_file_ctx_flush_dir(bra_io_file_ctx_t* ctx)
{
    if (ctx->last_dir_not_flushed)
    {
        bra_log_debug("flushing dir: %s", ctx->last_dir);
        ctx->last_dir_not_flushed = false;
        if (!_bra_io_file_ctx_write_meta_entry_common(ctx, ctx->last_dir_attr, ctx->last_dir, ctx->last_dir_size))
            return false;

        switch (BRA_ATTR_TYPE(ctx->last_dir_attr))
        {
        case BRA_ATTR_TYPE_SUBDIR:
        {
            assert(ctx->last_dir_node != NULL);

            bra_meta_entry_subdir_t me_subdir;
            me_subdir.parent_index = ctx->last_dir_node->index;
            if (fwrite(&me_subdir.parent_index, sizeof(uint32_t), 1, ctx->f.f) != 1)
                return false;
        }
        break;

        default:
            break;
        }
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

    const size_t total_size =
        ctx->last_dir_size + buf_size + (ctx->last_dir_size > 0);    // one extra for '/'
    if (total_size == 0 || total_size > UINT8_MAX)
        return false;

    me->name_size = (uint8_t) total_size;
    me->name      = malloc(sizeof(char) * (me->name_size + 1));    // !< one extra for '\0'
    if (me->name == NULL)
        return false;

    char* b = NULL;
    if (ctx->last_dir_size > 0)
    {
        memcpy(me->name, ctx->last_dir, ctx->last_dir_size);
        me->name[ctx->last_dir_size] = '/';
        b                            = &me->name[ctx->last_dir_size + 1];
    }
    else
    {
        b = me->name;
    }

    memcpy(b, buf, buf_size);
    b[buf_size] = '\0';
    return true;
}

static bool _bra_io_file_ctx_read_meta_entry_read_dir(bra_io_file_ctx_t* ctx, bra_meta_entry_t* me, const char* buf, const uint8_t buf_size)
{
    assert_bra_io_file_cxt_t(ctx);
    assert(me != NULL);
    assert(buf_size < UINT8_MAX);

    memcpy(ctx->last_dir, buf, buf_size);
    ctx->last_dir_size      = buf_size;
    ctx->last_dir[buf_size] = '\0';

    me->name_size = buf_size;
    me->name      = malloc(sizeof(char) * (me->name_size + 1));    // !< one extra for '\0'
    if (me->name == NULL)
        return false;

    memcpy(me->name, buf, me->name_size);
    me->name[me->name_size] = '\0';
    return true;
}

static bool _bra_io_file_ctx_read_meta_entry_read_subdir(bra_io_file_ctx_t* ctx, bra_meta_entry_t* me, const char* buf, const uint8_t buf_size)
{
    if (!_bra_io_file_ctx_read_meta_entry_read_dir(ctx, me, buf, buf_size))
        return false;

    // read parent index
    bra_meta_entry_subdir_t* mes = me->entry_data;
    if (fread(&mes->parent_index, sizeof(uint32_t), 1, ctx->f.f) != 1)
        return false;

    return true;
}

static bool _bra_io_file_ctx_write_meta_entry_process_write_file(bra_io_file_ctx_t* ctx, const bra_meta_entry_t* me, char* filename, uint8_t* filename_size)
{
    assert_bra_io_file_cxt_t(ctx);
    assert(me != NULL);
    assert(filename != NULL);
    assert(filename_size != NULL);

    if (BRA_ATTR_TYPE(me->attributes) != BRA_ATTR_TYPE_FILE)
        return false;

    if (ctx->last_dir_size >= me->name_size)
    {
        // In this case is a parent/sibling folder.
        // but it should have read a dir instead.
        // error because two consecutive files in different
        // folders are been submitted.
        return false;
    }

    if (strncmp(me->name, ctx->last_dir, ctx->last_dir_size) != 0)    // ctx->last_dir doesn't have '/'
        return false;

    size_t l = ctx->last_dir_size;    // strnlen(g_last_dir, BRA_MAX_PATH_LENGTH);
    if (me->name[l] == '/')           // or when l == 0
        ++l;                          // skip also '/'
    else if (ctx->last_dir_size > 0)
    {
        bra_log_critical("me->name: %s -- last_dir: %s", me->name, ctx->last_dir);
        return false;
    }

    *filename_size = me->name_size - l;
    assert(*filename_size > 0);    // check edge case that means an error somewhere else
    memcpy(filename, &me->name[l], *filename_size);
    filename[*filename_size] = '\0';

    // flush dir
    if (!_bra_io_file_ctx_flush_dir(ctx))
        return false;

    // write common meta data (attribute, filename, filename_size)
    if (!_bra_io_file_ctx_write_meta_entry_common(ctx, me->attributes, filename, *filename_size))
        return false;

    // 3. file size
    const bra_meta_entry_file_t* mef = (const bra_meta_entry_file_t*) me->entry_data;
    assert(mef != NULL);
    if (fwrite(&mef->data_size, sizeof(uint64_t), 1, ctx->f.f) != 1)
        return false;

    // 4. file content
    bra_io_file_t f2;
    memset(&f2, 0, sizeof(bra_io_file_t));
    if (!bra_io_file_open(&f2, me->name, "rb"))
        return false;

    if (!bra_io_file_copy_file_chunks(&ctx->f, &f2, mef->data_size))
        return false;    // f, f2 closed already (but not ctx as not required due to error terminating program)

    bra_io_file_close(&f2);
    return true;
}

static bool _bra_io_file_ctx_write_meta_entry_process_write_dir(bra_io_file_ctx_t* ctx, bra_meta_entry_t* me, char* dirname, uint8_t* dirname_size)
{
    assert_bra_io_file_cxt_t(ctx);
    assert(me != NULL);
    assert(dirname != NULL);
    assert(dirname_size != NULL);

    if (BRA_ATTR_TYPE(me->attributes) != BRA_ATTR_TYPE_DIR)
        return false;

    *dirname_size = me->name_size;
    memcpy(dirname, me->name, *dirname_size);
    dirname[*dirname_size] = '\0';

    bra_tree_node_t* node = bra_tree_dir_add(ctx->tree, dirname);
    if (node == NULL)
    {
        bra_log_error("unable to add dir %s to bra_tree", dirname);
        return false;
    }

    if (!_bra_io_file_ctx_flush_dir(ctx))
        return false;

    ctx->last_dir_not_flushed = true;
    ctx->last_dir_attr        = me->attributes;
    ctx->last_dir_node        = node;

    memcpy(ctx->last_dir, dirname, *dirname_size);
    ctx->last_dir[*dirname_size] = '\0';
    ctx->last_dir_size           = *dirname_size;
    return true;
}

static bool _bra_io_file_ctx_write_meta_entry_process_write_subdir(bra_io_file_ctx_t* ctx, bra_meta_entry_t* me, char* dirname, uint8_t* dirname_size)
{
    assert_bra_io_file_cxt_t(ctx);
    assert(me != NULL);
    assert(dirname != NULL);
    assert(dirname_size != NULL);

    if (BRA_ATTR_TYPE(me->attributes) != BRA_ATTR_TYPE_SUBDIR)
        return false;

    bra_tree_node_t* node = bra_tree_dir_add(ctx->tree, me->name);
    if (node == NULL)
    {
        bra_log_error("unable to add dir %s to bra_tree", me->name);
        return false;
    }

    if (ctx->last_dir_not_flushed)
    {
        // TODO: need to be reviewed with tree dir

        const bool replacing_dir = BRA_ATTR_TYPE(me->attributes) == BRA_ATTR_TYPE_SUBDIR;    // bra_fs_dir_is_sub_dir(ctx->last_dir, dirname);
        if (replacing_dir)
        {
            // TODO: this must recompose a subdir to a dir
            //       when subdir is present the last dir is partial.
            //       so need to recover from the parent index the parent dir to be merged.
            bra_log_debug("parent dir %s is empty, replacing it with %s", ctx->last_dir, dirname);
            me->attributes = BRA_ATTR_SET_TYPE(me->attributes, BRA_ATTR_TYPE_DIR);
        }
        else
        {
            // not a subdir(?), need to save as empty dir, otherwise wasn't given as input
            if (!_bra_io_file_ctx_flush_dir(ctx))
                return false;
        }
    }

    *dirname_size = me->name_size;
    memcpy(dirname, me->name, *dirname_size);
    dirname[*dirname_size] = '\0';

    ctx->last_dir_not_flushed = true;
    ctx->last_dir_attr        = me->attributes;
    ctx->last_dir_node        = node;

    // const size_t node_len = strnlen(node->dirname, BRA_MAX_PATH_LENGTH);
    memcpy(ctx->last_dir, dirname, *dirname_size);
    ctx->last_dir[*dirname_size] = '\0';
    ctx->last_dir_size           = (uint8_t) *dirname_size;
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

bool bra_io_file_ctx_open(bra_io_file_ctx_t* ctx, const char* fn, const char* mode)
{
    assert(ctx != NULL);
    assert(fn != NULL);
    assert(mode != NULL);

    memset(ctx, 0, sizeof(bra_io_file_ctx_t));
    ctx->isWritable = (strchr(mode, 'w') != NULL || strchr(mode, 'a') != NULL || strchr(mode, '+') != NULL);
    ctx->tree       = bra_tree_dir_create();
    if (ctx->tree == NULL)
        return false;
    const bool res = bra_io_file_open(&ctx->f, fn, mode);
    if (!res)
    {
        bra_tree_dir_destroy(&ctx->tree);
        ctx->tree = NULL;
    }
    else
        ctx->last_dir_node = ctx->tree->root;    // root dir

    return res;
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
        _bra_tree_node_print(ctx->tree->root);
#endif

        bra_tree_dir_destroy(&ctx->tree);
        ctx->tree = NULL;
    }

    if (ctx->isWritable)
    {
        if (!_bra_io_file_ctx_flush_dir(ctx))
            return false;

        if (ctx->num_files != ctx->cur_files)
        {
            bra_log_debug("Consolidated dirs results: entries: %u - original: %u", ctx->cur_files, ctx->num_files);
            ctx->num_files = ctx->cur_files;
            if (fflush(ctx->f.f) != 0)
            {
            BRA_IO_FILE_CTX_CLOSE_ERR:
                bra_log_error("unable to update header in %s", ctx->f.fn);
                res = false;
                goto BRA_IO_FILE_CTX_CLOSE;
            }
            // change header num files.
            if (!bra_io_file_seek(&ctx->f, sizeof(uint32_t), SEEK_SET))
                goto BRA_IO_FILE_CTX_CLOSE_ERR;

            if (fwrite(&ctx->num_files, sizeof(ctx->num_files), 1, ctx->f.f) != 1)
                goto BRA_IO_FILE_CTX_CLOSE_ERR;
        }
    }

BRA_IO_FILE_CTX_CLOSE:
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
    case BRA_ATTR_TYPE_SUBDIR:
    {
        if (!bra_meta_entry_subdir_init(me, 0))
            goto BRA_IO_READ_ERR;
        if (!_bra_io_file_ctx_read_meta_entry_read_subdir(ctx, me, buf, (uint8_t) buf_size))
            goto BRA_IO_READ_ERR;
    }
    break;
    case BRA_ATTR_TYPE_DIR:
    {
        if (!_bra_io_file_ctx_read_meta_entry_read_dir(ctx, me, buf, (uint8_t) buf_size))
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

    me->name[me->name_size] = '\0';
    return true;
}

bool bra_io_file_ctx_write_meta_entry(bra_io_file_ctx_t* ctx, const bra_attr_t attributes, const char* fn)
{
    assert_bra_io_file_cxt_t(ctx);

    bra_meta_entry_t me;
    char             buf[BRA_MAX_PATH_LENGTH];
    uint8_t          buf_size;

    const size_t len = strnlen(fn, BRA_MAX_PATH_LENGTH);
    if (len == 0 || len >= BRA_MAX_PATH_LENGTH)
    {
    BRA_IO_WRITE_ERR:
        bra_io_file_write_error(&ctx->f);
        return false;
    }

    if (!bra_meta_entry_init(&me, attributes, fn, len))
        goto BRA_IO_WRITE_ERR;

    // Processing & Writing data
    switch (BRA_ATTR_TYPE(me.attributes))
    {
    case BRA_ATTR_TYPE_FILE:
    {
        uint64_t ds;
        if (!bra_fs_file_size(fn, &ds))
            goto BRA_IO_WRITE_ERR;

        if (!bra_meta_entry_file_init(&me, ds))
            goto BRA_IO_WRITE_ERR;

        if (!_bra_io_file_ctx_write_meta_entry_process_write_file(ctx, &me, buf, &buf_size))
            goto BRA_IO_WRITE_ERR;
    }
    break;
    case BRA_ATTR_TYPE_DIR:
    {
        if (!_bra_io_file_ctx_write_meta_entry_process_write_dir(ctx, &me, buf, &buf_size))
            goto BRA_IO_WRITE_ERR;
    }
    break;
    case BRA_ATTR_TYPE_SUBDIR:
    {
        assert(ctx->last_dir_node != NULL);

        if (!bra_meta_entry_subdir_init(&me, ctx->last_dir_node->index))
            goto BRA_IO_WRITE_ERR;

        if (!_bra_io_file_ctx_write_meta_entry_process_write_subdir(ctx, &me, buf, &buf_size))
            goto BRA_IO_WRITE_ERR;
    }
    break;
    case BRA_ATTR_TYPE_SYM:
        bra_log_critical("SYMLINK NOT IMPLEMENTED YET");
        // goto BRA_IO_WRITE_ERR; // fallthrough
    default:
        goto BRA_IO_WRITE_ERR;
    }

    bra_meta_entry_free(&me);
    return true;
}

bool bra_io_file_ctx_encode_and_write_to_disk(bra_io_file_ctx_t* ctx, const char* fn)
{
    assert_bra_io_file_cxt_t(ctx);
    assert(fn != NULL);

    // get entry attributes
    // TODO replace ctx->last dir with tree_dir reconstructed path
    //      (the small challenge now is that it can be greater than 255 chars)
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
    bra_meta_entry_t me;
    if (!bra_meta_entry_init(&me, 0, NULL, 0))
        return false;

    if (!bra_io_file_ctx_read_meta_entry(ctx, &me))
        goto BRA_IO_DECODE_ERR;

    if (!_bra_validate_meta_name(&me))
    {
    BRA_IO_DECODE_ERR:
        bra_meta_entry_free(&me);
        bra_io_file_error(&ctx->f, "decode");
        return false;
    }

    // TODO: this part should resolve the meta name with the tree dir
    //       now the meta name is complete, later on will be partial
    //       so need to reconstruct the full path using the tree dir

    // 4. read and write in chunk data
    // NOTE: nothing to extract for a directory, but only to create it
    switch (BRA_ATTR_TYPE(me.attributes))
    {
    case BRA_ATTR_TYPE_FILE:
    {
        const bra_meta_entry_file_t* mef = (const bra_meta_entry_file_t*) me.entry_data;
        assert(mef != NULL);
        const uint64_t ds = mef->data_size;
        if (!bra_fs_file_exists_ask_overwrite(me.name, overwrite_policy, false))
        {
            end_msg = g_end_messages[1];
            bra_log_printf("Skipping file:   " BRA_PRINTF_FMT_FILENAME, me.name);
            bra_meta_entry_free(&me);
            if (!bra_io_file_skip_data(&ctx->f, ds))
            {
                bra_io_file_seek_error(&ctx->f);
                return false;
            }
        }
        else
        {
            bra_io_file_t f2;

            end_msg = g_end_messages[0];
            bra_log_printf("Extracting file: " BRA_PRINTF_FMT_FILENAME, me.name);
            // NOTE: the directory must have been created in the previous entry,
            //       otherwise this will fail to create the file.
            //       The archive ensures the last used directory is created first,
            //       and then its files follow.
            //       So, no need to create the parent directory for each file each time.
            if (!bra_io_file_open(&f2, me.name, "wb"))
                goto BRA_IO_DECODE_ERR;

            bra_meta_entry_free(&me);
            if (!bra_io_file_copy_file_chunks(&f2, &ctx->f, ds))
                return false;

            bra_io_file_close(&f2);
        }
    }
    break;
    case BRA_ATTR_TYPE_SUBDIR:
        // at this point the sub-dir name should have been already resolved
        // so it can fallthrough as a normal dir
        // [[fallthrough]];
    case BRA_ATTR_TYPE_DIR:
    {
        if (bra_fs_dir_exists(me.name))
        {
            end_msg = g_end_messages[1];
            bra_log_printf("Dir exists:   " BRA_PRINTF_FMT_FILENAME, me.name);
        }
        else
        {
            end_msg = g_end_messages[0];
            bra_log_printf("Creating dir: " BRA_PRINTF_FMT_FILENAME, me.name);

            if (!bra_fs_dir_make(me.name))
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

    bra_log_printf(" [  %-4.4s  ]\n", end_msg);
    return true;
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
        return false;


    const uint64_t ds   = BRA_ATTR_TYPE(me.attributes) == BRA_ATTR_TYPE_FILE ? ((bra_meta_entry_file_t*) me.entry_data)->data_size : 0;
    const char     attr = bra_format_meta_attributes(me.attributes);
    bra_format_bytes(ds, bytes);

    bra_log_printf("|   %c  | %s | ", attr, bytes);
    _bra_print_string_max_length(me.name, me.name_size, BRA_PRINTF_FMT_FILENAME_MAX_LENGTH);

    // print last dir to understand internal structure
#ifndef NDEBUG
    bra_log_printf("| %s [DEBUG]", ctx->last_dir);
#endif

    bra_log_printf("|\n");
    bra_meta_entry_free(&me);
    // skip data content
    if (!bra_io_file_skip_data(&ctx->f, ds))
    {
        bra_io_file_seek_error(&ctx->f);
        return false;
    }

    return true;
}
