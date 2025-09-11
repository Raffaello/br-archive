#pragma once

#include <assert.h>    // for assert TODO split in a different .h

#define assert_bra_io_file_t(x) assert((x) != NULL && (x)->f != NULL && (x)->fn != NULL)

void _bra_print_string_max_length(const char* buf, const int buf_length, const int max_length);
