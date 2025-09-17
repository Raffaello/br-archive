#include <io/lib_bra_io_file_ctx.h>
#include <lib_bra_defs.h>
#include <lib_bra_private.h>
#include <io/lib_bra_io_file.h>
#include <log/bra_log.h>
#include <fs/bra_fs_c.h>
#include <lib_bra.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static const char  g_end_messages[][5] = {" OK ", "SKIP"};
static const char* g_attr_type_names[] = {"file", "dir", "symlink", "subdir"};

///////////////////////////////////////////////////////////////////////////////////////////////////////

static bool _bra_io_file_ctx_write_meta_file_common(bra_io_file_ctx_t* ctx, const bra_attr_t attr, const char* filename, const uint8_t filename_size)
{
    assert_bra_io_file_cxt_t(ctx);
    assert(filename != NULL);

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
        if (!_bra_io_file_ctx_write_meta_file_common(ctx, ctx->last_dir_attr, ctx->last_dir, ctx->last_dir_size))
            return false;
    }

    return true;
}

static bool _bra_io_file_ctx_write_meta_file_process_write_file(bra_io_file_ctx_t* ctx, const bra_meta_file_t* mf, char* filename, uint8_t* filename_size)
{
    assert_bra_io_file_cxt_t(ctx);
    assert(mf != NULL);
    assert(filename != NULL);
    assert(filename_size != NULL);

    if (ctx->last_dir_size >= mf->name_size)
    {
        // In this case is a parent/sibling folder.
        // but it should have read a dir instead.
        // error because two consecutive files in different
        // folders are been submitted.
        return false;
    }

    if (strncmp(mf->name, ctx->last_dir, ctx->last_dir_size) != 0)    // ctx->last_dir doesn't have '/'
        return false;

    size_t l = ctx->last_dir_size;    // strnlen(g_last_dir, BRA_MAX_PATH_LENGTH);
    if (mf->name[l] == '/')           // or when l == 0
        ++l;                          // skip also '/'
    else if (ctx->last_dir_size > 0)
    {
        bra_log_critical("mf->name: %s -- last_dir: %s", mf->name, ctx->last_dir);
        return false;
    }

    *filename_size = mf->name_size - l;
    assert(*filename_size > 0);    // check edge case that means an error somewhere else
    memcpy(filename, &mf->name[l], *filename_size);
    filename[*filename_size] = '\0';

    // flush dir
    if (!_bra_io_file_ctx_flush_dir(ctx))
        return false;

    // write common meta data (attribute, filename, filename_size)
    if (!_bra_io_file_ctx_write_meta_file_common(ctx, mf->attributes.attr, filename, *filename_size))
        return false;

    // 3. data size
    // NOTE: for directory makes sense to be zero, but it could be used for something else.
    //       actually for directory would be better not saving it at all if it is always zero.
    if (fwrite(&mf->data_size, sizeof(uint64_t), 1, ctx->f.f) != 1)
        return false;

    // 4. data (this should be paired with data_size to avoid confusion
    // instead is in the meta file write function
    // NOTE: that data_size is written only for file in there)
    bra_io_file_t f2;
    memset(&f2, 0, sizeof(bra_io_file_t));
    if (!bra_io_file_open(&f2, mf->name, "rb"))
        return false;

    if (!bra_io_file_copy_file_chunks(&ctx->f, &f2, mf->data_size))
        return false;    // f, f2 closed already

    bra_io_file_close(&f2);
    return true;
}

static bool _bra_io_file_ctx_write_meta_file_process_write_dir(bra_io_file_ctx_t* ctx, bra_meta_file_t* mf, char* dirname, uint8_t* dirname_size)
{
    assert_bra_io_file_cxt_t(ctx);
    assert(mf != NULL);
    assert(dirname != NULL);
    assert(dirname_size != NULL);

    // As a safe-guard, but pointless otherwise
    if (mf->data_size != 0)
        return false;

    *dirname_size = mf->name_size;
    memcpy(dirname, mf->name, *dirname_size);
    dirname[*dirname_size] = '\0';

    if (ctx->last_dir_not_flushed)
    {
        // TODO: this will be in conflict with the new proposed tree dir structure.
        //       need to be done as a 2nd pass at the end as an update.
        //       as at this point can't know if it is better or not.
        //       as it might be honestly.
        //       the tree dir structure might be postponed for now.
        //       besides the consolidating 1st sub-dir if parent empty is not so good.
        //       the lazy write on directory could be kept though, not useful neither at the very end.
        //       i am just playing around here....
        //
        // TODO: the only scenario were is really useful is parent dir empty and just 1 subdir
        //       that in that case can save 1 entry to be stored.
        //       also especially if the parent dir as a long name
        //       but here at maximum is a save of 256 bytes.. doesn't really make sense. well better than nothing, but..
        // TODO: tree instead might occupy more if the parent directory name are very short, 1-3 chars
        //       in this case though it could revert to a normal directory as it was since the beginning
        //       using the consolidate dir as well
        const bool replacing_dir = mf->attributes.bra_meta_file_attr_t.type == BRA_ATTR_TYPE_SUB_DIR;    // bra_fs_dir_is_sub_dir(ctx->last_dir, dirname);
        if (replacing_dir)
        {
            bra_log_debug("parent dir %s is empty, replacing it with %s", ctx->last_dir, dirname);
            // mf->attributes.attr &= ~BRA_ATTR_TYPE(0xFF);           // clear the bits first
            // mf->attributes.attr |= BRA_ATTR_TYPE(BRA_ATTR_TYPE_DIR);    // in this case it becomes a regular dir
            mf->attributes.bra_meta_file_attr_t.type = BRA_ATTR_TYPE_DIR;
        }
        else
        {
            // not a subdir, need to save as empty dir, otherwise wasn't given as input
            if (!_bra_io_file_ctx_flush_dir(ctx))
                return false;
        }
    }

    ctx->last_dir_not_flushed = true;
    ctx->last_dir_attr        = mf->attributes.attr;

    memcpy(ctx->last_dir, dirname, *dirname_size);
    ctx->last_dir[*dirname_size] = '\0';
    ctx->last_dir_size           = *dirname_size;
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
    return bra_io_file_open(&ctx->f, fn, mode);
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

bool bra_io_file_ctx_read_meta_file(bra_io_file_ctx_t* ctx, bra_meta_file_t* mf)
{
    assert_bra_io_file_cxt_t(ctx);
    assert(mf != NULL);

    char     buf[BRA_MAX_PATH_LENGTH];
    unsigned buf_size = 0;

    mf->name      = NULL;
    mf->name_size = 0;
    mf->data_size = 0;

    // 1. attributes
    if (fread(&mf->attributes.attr, sizeof(bra_attr_t), 1, ctx->f.f) != 1)
    {
    BRA_IO_READ_ERR:
        bra_io_file_read_error(&ctx->f);
        return false;
    }

    // TODO: this block can be removed as it bit masked now, no other 4 values possible at all.
    // #ifndef NDEBUG
    //     switch (mf->attributes.bra_meta_file_attr_t.type)
    //     {
    //     case BRA_ATTR_TYPE_FILE:
    //         // [[fallthrough]];
    //     case BRA_ATTR_TYPE_DIR:
    //     case BRA_ATTR_TYPE_SYM:
    //     case BRA_ATTR_TYPE_SUB_DIR:
    //         break;
    //     default:
    //         goto BRA_IO_READ_ERR;
    //     }
    // #endif

    // 2. filename size
    if (fread(&buf_size, sizeof(uint8_t), 1, ctx->f.f) != 1)
        goto BRA_IO_READ_ERR;
    if (buf_size == 0 || buf_size >= BRA_MAX_PATH_LENGTH)
        goto BRA_IO_READ_ERR;

    // 3. filename
    if (fread(buf, sizeof(char), buf_size, ctx->f.f) != buf_size)
        goto BRA_IO_READ_ERR;

    buf[buf_size] = '\0';

    // 4. data size
    switch (mf->attributes.bra_meta_file_attr_t.type)
    {
    case BRA_ATTR_TYPE_SUB_DIR:
        // TODO: for now as normal BRA_ATTR_TYPE_DIR.
    case BRA_ATTR_TYPE_DIR:
    {
        // NOTE: for directory doesn't have data-size nor data,
        // NOTE: here if it is a sub-dir
        //       it could cut some extra chars, and be constructed from the other dir
        //       but the file won't be able to reconstruct its full relative path.
        //   SO: I can't optimize sub-dir length with this struct
        //       I must replicate the parent-dir too
        // TODO: unless dir attribute has a 2nd bit to tell sub-dir or dir
        //       but then must track the sub-dir (postponed for now until recursive)
        memcpy(ctx->last_dir, buf, buf_size);
        ctx->last_dir_size      = buf_size;
        ctx->last_dir[buf_size] = '\0';

        mf->name_size = (uint8_t) buf_size;
        mf->name      = malloc(sizeof(char) * (buf_size + 1));    // !< one extra for '\0'
        if (mf->name == NULL)
            goto BRA_IO_READ_ERR;

        memcpy(mf->name, buf, buf_size);
        mf->name[buf_size] = '\0';
    }
    break;
    case BRA_ATTR_TYPE_FILE:
    {
        if (fread(&mf->data_size, sizeof(uint64_t), 1, ctx->f.f) != 1)
            goto BRA_IO_READ_ERR;

        const size_t total_size =
            ctx->last_dir_size + buf_size + (ctx->last_dir_size > 0);    // one extra for '/'
        if (total_size == 0 || total_size > UINT8_MAX)
            goto BRA_IO_READ_ERR;

        mf->name_size = (uint8_t) total_size;
        mf->name      = malloc(sizeof(char) * (mf->name_size + 1));    // !< one extra for '\0'
        if (mf->name == NULL)
            goto BRA_IO_READ_ERR;

        char* b = NULL;
        if (ctx->last_dir_size > 0)
        {
            memcpy(mf->name, ctx->last_dir, ctx->last_dir_size);
            mf->name[ctx->last_dir_size] = '/';
            b                            = &mf->name[ctx->last_dir_size + 1];
        }
        else
        {
            b = mf->name;
        }

        memcpy(b, buf, buf_size);
        b[buf_size] = '\0';
    }
    break;
    case BRA_ATTR_TYPE_SYM:
        bra_log_critical("SYMLINK NOT IMPLEMENTED YET");
        return false;
        break;
    default:
        return false;
    }

    mf->name[mf->name_size] = '\0';
    return true;
}

bool bra_io_file_ctx_write_meta_file(bra_io_file_ctx_t* ctx, bra_meta_file_t* mf)
{
    assert_bra_io_file_cxt_t(ctx);
    assert(mf != NULL);
    assert(mf->name != NULL);

    char    buf[BRA_MAX_PATH_LENGTH];
    uint8_t buf_size;

    const size_t len = strnlen(mf->name, BRA_MAX_PATH_LENGTH);
    if (len != mf->name_size || len == 0 || len >= BRA_MAX_PATH_LENGTH)
    {
    BRA_IO_WRITE_ERR:
        bra_io_file_write_error(&ctx->f);
        return false;
    }

    // Processing & Writing data
    switch (mf->attributes.attr)
    {
    case BRA_ATTR_TYPE_FILE:
    {
        if (!_bra_io_file_ctx_write_meta_file_process_write_file(ctx, mf, buf, &buf_size))
            goto BRA_IO_WRITE_ERR;
        break;
    }
    case BRA_ATTR_TYPE_SUB_DIR:
        // TODO: for now same as dir
    case BRA_ATTR_TYPE_DIR:
    {
        if (!_bra_io_file_ctx_write_meta_file_process_write_dir(ctx, mf, buf, &buf_size))
            goto BRA_IO_WRITE_ERR;
        break;
    }
    break;
    case BRA_ATTR_TYPE_SYM:
        bra_log_critical("SYMLINK NOT IMPLEMENTED YET");
        // goto BRA_IO_WRITE_ERR; // fallthrough
    default:
        goto BRA_IO_WRITE_ERR;
    }

    return true;
}

bool bra_io_file_ctx_encode_and_write_to_disk(bra_io_file_ctx_t* ctx, const char* fn)
{
    assert_bra_io_file_cxt_t(ctx);
    assert(fn != NULL);

    // 1. attributes
    bra_meta_file_attr_u attributes;
    if (!bra_fs_file_attributes(fn, ctx->last_dir, &attributes.attr))
    {
        bra_log_error("%s has unknown attribute", fn);
    BRA_IO_WRITE_CLOSE_ERROR:
        bra_io_file_close(&ctx->f);
        return false;
    }

    bra_log_printf("Archiving %-7s:  ", g_attr_type_names[attributes.bra_meta_file_attr_t.type]);
    _bra_print_string_max_length(fn, strlen(fn), BRA_PRINTF_FMT_FILENAME_MAX_LENGTH);

    // 2. file name length
    const size_t fn_len = strnlen(fn, BRA_MAX_PATH_LENGTH);
    // NOTE: fn_len >= BRA_MAX_PATH_LENGTH is redundant but as a safeguard
    //       in case changing BRA_MAX_PATH_LENGTH to be UINT8_MAX
    //       there is a static assert for that now.
    if (fn_len > UINT8_MAX /*|| fn_len >= BRA_MAX_PATH_LENGTH*/)
    {
        bra_log_error("filename too long: %s", fn);
        goto BRA_IO_WRITE_CLOSE_ERROR;
    }

    if (fn_len == 0)
    {
        bra_log_error("filename missing: %s", fn);
        goto BRA_IO_WRITE_CLOSE_ERROR;
    }

    const uint8_t fn_size = (uint8_t) fn_len;
    uint64_t      ds;
    if (!bra_fs_file_size(fn, &ds))
        goto BRA_IO_WRITE_CLOSE_ERROR;

    bra_meta_file_t mf;
    mf.attributes = attributes;
    mf.name_size  = fn_size;
    mf.data_size  = ds;
    mf.name       = _bra_strdup(fn);
    if (mf.name == NULL)
        goto BRA_IO_WRITE_CLOSE_ERROR;

    const bool res = bra_io_file_ctx_write_meta_file(ctx, &mf);
    bra_meta_file_free(&mf);
    if (!res)
        return false;    // f closed already

    bra_log_printf(" [  %-4.4s  ]\n", g_end_messages[0]);
    return true;
}

bool bra_io_file_ctx_decode_and_write_to_disk(bra_io_file_ctx_t* ctx, bra_fs_overwrite_policy_e* overwrite_policy)
{
    assert_bra_io_file_cxt_t(ctx);
    assert(overwrite_policy != NULL);

    const char*     end_msg;    // 'OK  ' | 'SKIP'
    bra_meta_file_t mf;
    if (!bra_io_file_ctx_read_meta_file(ctx, &mf))
        return false;

    if (!_bra_validate_meta_filename(&mf))
    {
    BRA_IO_DECODE_ERR:
        bra_meta_file_free(&mf);
        bra_io_file_error(&ctx->f, "decode");
        return false;
    }

    // 4. read and write in chunk data
    // NOTE: nothing to extract for a directory, but only to create it
    switch (mf.attributes.attr)
    {
    case BRA_ATTR_TYPE_FILE:
    {
        const uint64_t ds = mf.data_size;
        if (!bra_fs_file_exists_ask_overwrite(mf.name, overwrite_policy, false))
        {
            end_msg = g_end_messages[1];
            bra_log_printf("Skipping file:   " BRA_PRINTF_FMT_FILENAME, mf.name);
            bra_meta_file_free(&mf);
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
            bra_log_printf("Extracting file: " BRA_PRINTF_FMT_FILENAME, mf.name);
            // NOTE: the directory must have been created in the previous entry,
            //       otherwise this will fail to create the file.
            //       The archive ensures the last used directory is created first,
            //       and then its files follow.
            //       So, no need to create the parent directory for each file each time.
            if (!bra_io_file_open(&f2, mf.name, "wb"))
                goto BRA_IO_DECODE_ERR;

            bra_meta_file_free(&mf);
            if (!bra_io_file_copy_file_chunks(&f2, &ctx->f, ds))
                return false;

            bra_io_file_close(&f2);
        }
    }
    break;
    case BRA_ATTR_TYPE_SUB_DIR:
        // TODO: for now like dir
    case BRA_ATTR_TYPE_DIR:
    {
        if (bra_fs_dir_exists(mf.name))
        {
            end_msg = g_end_messages[1];
            bra_log_printf("Dir exists:   " BRA_PRINTF_FMT_FILENAME, mf.name);
        }
        else
        {
            end_msg = g_end_messages[0];
            bra_log_printf("Creating dir: " BRA_PRINTF_FMT_FILENAME, mf.name);

            if (!bra_fs_dir_make(mf.name))
                goto BRA_IO_DECODE_ERR;
        }

        bra_meta_file_free(&mf);
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
