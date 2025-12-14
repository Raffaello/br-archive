#include "lib_bra_io_file_meta_entries.h"

#include <lib_bra_private.h>
#include <lib_bra_defs.h>

#include <stdint.h>
#include <assert.h>

bool bra_io_file_read_meta_entry_file(bra_io_file_t* f, bra_meta_entry_t* me)
{
    assert_bra_io_file_t(f);
    assert(me != NULL);
    assert(me->entry_data != NULL);

    if (BRA_ATTR_TYPE(me->attributes) != BRA_ATTR_TYPE_FILE)
        return false;

    bra_meta_entry_file_t* mef = me->entry_data;
    return bra_io_file_read(f, &mef->data_size, sizeof(uint64_t));
}

bool bra_io_file_write_meta_entry_file(bra_io_file_t* f, const bra_meta_entry_t* me)
{
    assert_bra_io_file_t(f);
    assert(me != NULL);
    assert(me->entry_data != NULL);

    if (BRA_ATTR_TYPE(me->attributes) != BRA_ATTR_TYPE_FILE)
        return false;

    bra_meta_entry_file_t* mef = me->entry_data;
    return bra_io_file_write(f, &mef->data_size, sizeof(uint64_t));
}

bool bra_io_file_read_meta_entry_subdir(bra_io_file_t* f, bra_meta_entry_t* me)
{
    assert_bra_io_file_t(f);
    assert(me != NULL);
    assert(me->entry_data != NULL);

    if (BRA_ATTR_TYPE(me->attributes) != BRA_ATTR_TYPE_SUBDIR)
        return false;

    // read parent index
    bra_meta_entry_subdir_t* mes = me->entry_data;
    return bra_io_file_read(f, &mes->parent_index, sizeof(uint32_t));
}
