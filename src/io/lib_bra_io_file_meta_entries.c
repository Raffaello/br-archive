#include "lib_bra_io_file_meta_entries.h"

#include <lib_bra_private.h>
#include <lib_bra_defs.h>
#include <lib_bra.h>

#include <io/lib_bra_io_file_chunks.h>
#include <utils/lib_bra_crc32c.h>

#include <log/bra_log.h>

#include <stdint.h>
#include <assert.h>
#include <string.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////

bool bra_io_file_meta_entry_read_common_header(bra_io_file_t* f, bra_meta_entry_t* me)
{
    assert_bra_io_file_t(f);
    assert(me != NULL);

    char       buf[BRA_MAX_PATH_LENGTH];
    uint8_t    buf_size = 0;
    bra_attr_t attr;

    // 1. attributes
    if (!bra_io_file_read(f, &attr, sizeof(bra_attr_t)))
    {
    BRA_IO_FILE_META_ENTRY_READ_COMMON_HEADER_ERR:
        bra_io_file_close(f);
        return false;
    }

    // 2. filename size
    if (!bra_io_file_read(f, &buf_size, sizeof(uint8_t)))
        goto BRA_IO_FILE_META_ENTRY_READ_COMMON_HEADER_ERR;

    if (buf_size == 0)
        goto BRA_IO_FILE_META_ENTRY_READ_COMMON_HEADER_ERR;

    // 3. filename
    if (!bra_io_file_read(f, buf, buf_size))
        goto BRA_IO_FILE_META_ENTRY_READ_COMMON_HEADER_ERR;

    buf[buf_size] = '\0';
    if (!bra_meta_entry_init(me, attr, buf, buf_size))
    {
        bra_log_error("unable to init meta entry for %s", buf);
        goto BRA_IO_FILE_META_ENTRY_READ_COMMON_HEADER_ERR;
    }

    return true;
}

bool bra_io_file_meta_entry_write_common_header(bra_io_file_t* f, const bra_meta_entry_t* me)
{
    assert_bra_io_file_t(f);
    assert(me != NULL);

    // 1. attributes
    if (!bra_io_file_write(f, &me->attributes, sizeof(bra_attr_t)))
        return false;

    // 2. filename size
    if (!bra_io_file_write(f, &me->name_size, sizeof(uint8_t)))
        return false;

    // 3. filename
    if (!bra_io_file_write(f, me->name, me->name_size))
        return false;

    return true;
}

bool bra_io_file_meta_entry_read_file_entry(bra_io_file_t* f, bra_meta_entry_t* me)
{
    assert_bra_io_file_t(f);
    assert(me != NULL);
    assert(me->entry_data != NULL);

    if (!bra_meta_entry_file_set(me, 0))
    {
        bra_io_file_close(f);
        return false;
    }

    bra_meta_entry_file_t* mef = me->entry_data;
    return bra_io_file_read(f, &mef->data_size, sizeof(uint64_t));
}

bool bra_io_file_meta_entry_write_file_entry(bra_io_file_t* f, const bra_meta_entry_t* me)
{
    assert_bra_io_file_t(f);
    assert(me != NULL);
    assert(me->entry_data != NULL);

    if (BRA_ATTR_TYPE(me->attributes) != BRA_ATTR_TYPE_FILE)
    {
        bra_io_file_close(f);
        return false;
    }

    bra_meta_entry_file_t* mef = me->entry_data;
    return bra_io_file_write(f, &mef->data_size, sizeof(uint64_t));
}

bool bra_io_file_meta_entry_read_subdir_entry(bra_io_file_t* f, bra_meta_entry_t* me)
{
    assert_bra_io_file_t(f);
    assert(me != NULL);
    assert(me->entry_data != NULL);

    if (BRA_ATTR_TYPE(me->attributes) != BRA_ATTR_TYPE_SUBDIR)
    {
    BRA_IO_FILE_META_ENTRY_READ_SUBDIR_ENTRY_ERR:
        bra_io_file_close(f);
        return false;
    }

    if (!bra_meta_entry_subdir_set(me, 0))
        goto BRA_IO_FILE_META_ENTRY_READ_SUBDIR_ENTRY_ERR;

    bra_meta_entry_subdir_t* mes = me->entry_data;
    return bra_io_file_read(f, &mes->parent_index, sizeof(uint32_t));
}

bool bra_io_file_meta_entry_write_subdir_entry(bra_io_file_t* f, const bra_meta_entry_t* me)
{
    assert_bra_io_file_t(f);
    assert(me != NULL);
    assert(me->entry_data != NULL);

    if (BRA_ATTR_TYPE(me->attributes) != BRA_ATTR_TYPE_SUBDIR)
    {
        bra_io_file_close(f);
        return false;
    }

    bra_meta_entry_subdir_t* mes = me->entry_data;
    return bra_io_file_write(f, &mes->parent_index, sizeof(uint32_t));
}

bool bra_io_file_meta_entry_flush_entry_file(bra_io_file_t* f, bra_meta_entry_t* me, const char* filename, const size_t filename_len)
{
    assert_bra_io_file_t(f);
    assert(me != NULL);
    assert(filename != NULL);

    if (BRA_ATTR_TYPE(me->attributes) != BRA_ATTR_TYPE_FILE)
    {
    BRA_IO_FILE_META_ENTRY_FLUSH_ENTRY_FILE_ERR:
        bra_io_file_close(f);
        return false;
    }

    // compute crc32 up to here
    const bra_meta_entry_file_t* mef = (const bra_meta_entry_file_t*) me->entry_data;
    if (!_bra_compute_header_crc32(filename_len, filename, me))
        goto BRA_IO_FILE_META_ENTRY_FLUSH_ENTRY_FILE_ERR;

    // write common meta data (attribute, filename, filename_size)
    const bra_attr_t attr_orig = me->attributes;
    const int64_t    me_pos    = bra_io_file_tell(f);
    if (me_pos <= 0)
        goto BRA_IO_FILE_META_ENTRY_FLUSH_ENTRY_FILE_ERR;

    if (!bra_io_file_meta_entry_write_common_header(f, me))
        return false;

    // 3. file size
    assert(mef != NULL);

    // file content
    bra_io_file_t f2;
    memset(&f2, 0, sizeof(bra_io_file_t));
    if (!bra_io_file_open(&f2, filename, "rb"))
        goto BRA_IO_FILE_META_ENTRY_FLUSH_ENTRY_FILE_ERR;

    switch (BRA_ATTR_COMP(me->attributes))
    {
    case BRA_ATTR_COMP_STORED:
        me->crc32 = bra_crc32c(&mef->data_size, sizeof(uint64_t), me->crc32);
        if (!bra_io_file_meta_entry_write_file_entry(f, me))
            goto BRA_IO_FILE_META_ENTRY_FLUSH_ENTRY_FILE_ERROR;

        if (!bra_io_file_chunks_copy_file(f, &f2, mef->data_size, me, true))
            return false;
        break;
    case BRA_ATTR_COMP_COMPRESSED:
        if (!bra_io_file_chunks_compress_file(f, &f2, mef->data_size, me))
        {
            // check if it has failed do it to invalidate file compression rather than error
            if (attr_orig != me->attributes)
            {
                // In this case it must be re-done fully due to the crc32
                // The file hasn't be stored.
                if (!bra_io_file_seek(f, me_pos, SEEK_SET))
                    goto BRA_IO_FILE_META_ENTRY_FLUSH_ENTRY_FILE_ERROR;

                // TODO: this is a quick fix after changed the metadata attribute
                //       later on refactor to avoid a recursive call.
                bra_io_file_close(&f2);
                return bra_io_file_meta_entry_flush_entry_file(f, me, filename, filename_len);
            }
            else
                goto BRA_IO_FILE_META_ENTRY_FLUSH_ENTRY_FILE_ERROR;
        }
        break;
    default:
        bra_log_critical("invalid compression type for file: %u", BRA_ATTR_COMP(me->attributes));
        goto BRA_IO_FILE_META_ENTRY_FLUSH_ENTRY_FILE_ERROR;
    }

    bra_io_file_close(&f2);
    return true;

BRA_IO_FILE_META_ENTRY_FLUSH_ENTRY_FILE_ERROR:
    bra_io_file_close(f);
    bra_io_file_close(&f2);
    return false;
}

bool bra_io_file_meta_entry_flush_entry_dir(bra_io_file_t* f, const bra_meta_entry_t* me)
{
    assert_bra_io_file_t(f);
    assert(me != NULL);
    // assert(ctx->last_dir_node != NULL);
    // assert(ctx->last_dir_node->parent != NULL);    // not root

    if (BRA_ATTR_TYPE(me->attributes) != BRA_ATTR_TYPE_DIR &&
        BRA_ATTR_TYPE(me->attributes) != BRA_ATTR_TYPE_SUBDIR)
    {
        bra_io_file_close(f);
        return false;
    }

    if (!bra_io_file_meta_entry_write_common_header(f, me))
        return false;

    return true;
}

bool bra_io_file_meta_entry_flush_entry_subdir(bra_io_file_t* f, const bra_meta_entry_t* me)
{
    assert_bra_io_file_t(f);
    assert(me != NULL);

    if (!bra_io_file_meta_entry_flush_entry_dir(f, me))
        return false;

    return bra_io_file_meta_entry_write_subdir_entry(f, me);
}
