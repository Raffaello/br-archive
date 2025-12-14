#include "lib_bra_io_file_meta_entries.h"

#include <lib_bra_private.h>
#include <lib_bra_defs.h>

#include <io/lib_bra_io_file_chunks.h>
#include <utils/lib_bra_crc32c.h>

#include <log/bra_log.h>

#include <stdint.h>
#include <assert.h>
#include <string.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////

bool bra_io_file_meta_entry_write_common_header(bra_io_file_t* f, const bra_attr_t attr, const char* filename, const uint8_t filename_size)
{
    assert_bra_io_file_t(f);
    assert(filename != NULL);
    assert(filename_size > 0);

    // 1. attributes
    if (!bra_io_file_write(f, &attr, sizeof(bra_attr_t)))
        return false;

    // 2. filename size
    if (!bra_io_file_write(f, &filename_size, sizeof(uint8_t)))
        return false;

    // 3. filename
    if (!bra_io_file_write(f, filename, filename_size))
        return false;

    return true;
}

bool bra_io_file_meta_entry_read_file_entry(bra_io_file_t* f, bra_meta_entry_t* me)
{
    assert_bra_io_file_t(f);
    assert(me != NULL);
    assert(me->entry_data != NULL);

    if (BRA_ATTR_TYPE(me->attributes) != BRA_ATTR_TYPE_FILE)
        return false;

    bra_meta_entry_file_t* mef = me->entry_data;
    return bra_io_file_read(f, &mef->data_size, sizeof(uint64_t));
}

bool bra_io_file_meta_entry_write_file_entry(bra_io_file_t* f, const bra_meta_entry_t* me)
{
    assert_bra_io_file_t(f);
    assert(me != NULL);
    assert(me->entry_data != NULL);

    if (BRA_ATTR_TYPE(me->attributes) != BRA_ATTR_TYPE_FILE)
        return false;

    bra_meta_entry_file_t* mef = me->entry_data;
    return bra_io_file_write(f, &mef->data_size, sizeof(uint64_t));
}

bool bra_io_file_meta_entry_read_subdir_entry(bra_io_file_t* f, bra_meta_entry_t* me)
{
    assert_bra_io_file_t(f);
    assert(me != NULL);
    assert(me->entry_data != NULL);

    if (BRA_ATTR_TYPE(me->attributes) != BRA_ATTR_TYPE_SUBDIR)
        return false;

    // read parent index
    bra_meta_entry_subdir_t* mes = me->entry_data;
    return bra_io_file_read(f, &mes->parent_index, sizeof(bra_meta_entry_subdir_t));
}

// bool bra_io_file_meta_entry_write_subdir_entry(bra_io_file_t* f, const bra_meta_entry_t* me)
// {
//     assert_bra_io_file_t(f);
//     assert(me != NULL);
//     assert(me->entry_data != NULL);

// if (BRA_ATTR_TYPE(me->attributes) != BRA_ATTR_TYPE_SUBDIR)
//     return false;

// // read parent index
// bra_meta_entry_subdir_t* mes = me->entry_data;
// return bra_io_file_write(f, &mes->parent_index, sizeof(uint32_t));
// }

bool bra_io_file_meta_entry_flush_entry_file(bra_io_file_t* f, bra_meta_entry_t* me, const char* filename, const size_t filename_len)
{
    assert_bra_io_file_t(f);
    assert(me != NULL);
    assert(filename != NULL);

    if (BRA_ATTR_TYPE(me->attributes) != BRA_ATTR_TYPE_FILE)
        return false;

    // compute crc32 up to here
    const bra_meta_entry_file_t* mef = (const bra_meta_entry_file_t*) me->entry_data;
    if (!_bra_compute_header_crc32(filename_len, filename, me))
        return false;

    // write common meta data (attribute, filename, filename_size)
    const bra_attr_t attr_orig = me->attributes;
    const int64_t    me_pos    = bra_io_file_tell(f);
    if (me_pos <= 0)
        return false;

    if (!bra_io_file_meta_entry_write_common_header(f, me->attributes, me->name, me->name_size))
        return false;

    // 3. file size
    assert(mef != NULL);
    // me->crc32 = bra_crc32c(&mef->data_size, sizeof(uint64_t), me->crc32);
    // if (fwrite(&mef->data_size, sizeof(uint64_t), 1, ctx->f.f) != 1)
    //     return false;

    // file content
    bra_io_file_t f2;
    memset(&f2, 0, sizeof(bra_io_file_t));
    if (!bra_io_file_open(&f2, filename, "rb"))
        return false;

    switch (BRA_ATTR_COMP(me->attributes))
    {
    case BRA_ATTR_COMP_STORED:
        me->crc32 = bra_crc32c(&mef->data_size, sizeof(uint64_t), me->crc32);
        if (!bra_io_file_meta_entry_write_file_entry(f, me))
            return false;
        if (!bra_io_file_chunks_copy_file(f, &f2, mef->data_size, me))
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
                    return false;

                // TODO: this is a quick fix after changed the metadata attribute
                //       later on refactor to avoid a recursive call.
                bra_io_file_close(&f2);
                // --ctx->cur_files;
                return bra_io_file_meta_entry_flush_entry_file(f, me, filename, filename_len);
            }
            else
                return false;
        }
        break;
    default:
        bra_log_critical("invalid compression type for file: %u", BRA_ATTR_COMP(me->attributes));
        return false;
    }

    bra_io_file_close(&f2);
    return true;
}

bool bra_io_file_meta_entry_flush_entry_dir(bra_io_file_t* f, const bra_meta_entry_t* me)
{
    assert_bra_io_file_t(f);
    assert(me != NULL);
    // assert(ctx->last_dir_node != NULL);
    // assert(ctx->last_dir_node->parent != NULL);    // not root

    if (BRA_ATTR_TYPE(me->attributes) != BRA_ATTR_TYPE_DIR &&
        BRA_ATTR_TYPE(me->attributes) != BRA_ATTR_TYPE_SUBDIR)
        return false;

    if (!bra_io_file_meta_entry_write_common_header(f, me->attributes, me->name, me->name_size))
        return false;

    return true;
}

bool bra_io_file_meta_entry_flush_entry_subdir(bra_io_file_t* f, const bra_meta_entry_t* me)
{
    assert_bra_io_file_t(f);
    assert(me != NULL);

    if (BRA_ATTR_TYPE(me->attributes) != BRA_ATTR_TYPE_SUBDIR)
        return false;

    if (!bra_io_file_meta_entry_flush_entry_dir(f, me))
        return false;

    bra_meta_entry_subdir_t* mes = (bra_meta_entry_subdir_t*) me->entry_data;
    return bra_io_file_write(f, mes, sizeof(bra_meta_entry_subdir_t));
}
