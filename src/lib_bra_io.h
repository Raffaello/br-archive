#pragma once


#include "lib_bra_types.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#pragma pack(push, 1)

/**
 * @brief BR-Archive File Header.
 */
typedef struct bra_io_header_t
{
    uint32_t magic;    //!< 'BR-a'

    // uint8_t  version;    //!< Archive format version
    // compression type ? (archive, best, fast, ... ??)
    // crc32 / md5 ?
    uint32_t num_files;

} bra_io_header_t;

/**
 * @brief BR-Sfx File Footer.
 */
typedef struct bra_io_footer_t
{
    uint32_t magic;            //!< 'BR-x'
    int64_t  header_offset;    //!< absolute offset of the header chunk from file start.
} bra_io_footer_t;

#pragma pack(pop)

/**
 * @brief Type used to perform I/O from the disk.
 *        It is just a simple wrapper around @c FILE,
 *        but it carries on the filename @p fn associated with it.
 */
typedef struct bra_io_file_t
{
    FILE* f;     //!< File Pointer representing a file on the disk.
    char* fn;    //!< the filename of the file on disk.
} bra_io_file_t;

/**
 * @brief Print a meta file @p f using @ref bra_log_printf to display attributes, size and filename.
 *
 * @param f
 * @param bh
 * @return true
 * @return false
 */
bool bra_io_print_meta_file(bra_io_file_t* f);

/**
 * @brief Print an error message and close the file.
 *
 * @param bf
 * @param verb a string to complete the error message: "unable to %s".
 */
void bra_io_file_error(bra_io_file_t* bf, const char* verb);

/**
 * @brief Print an error message and eventually close the file
 *
 * @see bra_io_file_error
 *
 * @param bf
 */
void bra_io_file_open_error(bra_io_file_t* bf);

/**
 * @brief Print an error message and close the file.
 *
 * @see bra_io_file_error
 *
 * @param bf
 */
void bra_io_file_read_error(bra_io_file_t* bf);

/**
 * @brief Print an error message and close the file.
 *
 * @see bra_io_file_error
 *
 * @param bf
 */
void bra_io_file_seek_error(bra_io_file_t* bf);

/**
 * @brief Print an error message and close the file.
 *
 * @see bra_io_file_error
 *
 * @param bf
 */
void bra_io_file_write_error(bra_io_file_t* bf);

/**
 * @brief Detect if the given filename @p fn is an ELF.
 *
 * @see bra_io_is_pe_exe
 * @see bra_io_is_sfx
 *
 * @param fn
 * @return true if ELF magic is detected.
 * @return false otherwise.
 */
bool bra_io_is_elf(const char* fn);

/**
 * @brief Detect if the given filename @p fn is a PE/EXE.
 *
 * @see bra_io_is_elf
 * @see bra_io_is_sfx
 *
 * @param fn
 * @return true if a valid PE signature is detected.
 * @return false otherwise.
 */
bool bra_io_is_pe_exe(const char* fn);

/**
 * @brief Detect if the given filename @p fn is a possible SFX archive.
 *
 * @todo consider to open the file once instead of 2 times.
 *
 * @see bra_io_is_elf
 * @see bra_io_is_pe_exe
 *
 * @param fn
 * @return true if a file appear to be an ELF or PE/EXE (SFX wrapper).
 * @return false otherwise.
 */
bool bra_io_is_sfx(const char* fn);

/**
 * @brief Open the file @p fn in the @p mode.
 *        On failure there is no need to call @ref bra_io_close
 *
 * @param bf
 * @param fn
 * @param mode @c fopen modes
 * @return true on success
 * @return false on error and close @p bf via @ref bra_io_close.
 */
bool bra_io_open(bra_io_file_t* bf, const char* fn, const char* mode);

/**
 * @brief Close the file, free the internal memory and set the fields to NULL.
 *
 * @param bf
 */
void bra_io_close(bra_io_file_t* bf);

/**
 * @brief Open the file @p fn in @p mode and seek to the beginning of the SFX footer (EOF - sizeof(bra_io_footer_t)).
 *        On failure there is no need to call @ref bra_io_close.
 *
 * @param f
 * @param fn
 * @param mode @c fopen modes
 * @return true on success (file positioned at start of footer; ready for bra_io_read_footer)
 * @return false on error (seek errors close @p f via @ref bra_io_close)
 */
bool bra_io_sfx_open(bra_io_file_t* f, const char* fn, const char* mode);

/**
 * @brief Seek file at position @p offs.
 *
 * @param f
 * @param offs
 * @param origin SEEK_SET, SEEK_CUR, SEEK_END
 * @return true
 * @return false
 */
bool bra_io_seek(bra_io_file_t* f, const int64_t offs, const int origin);

/**
 * @brief Tell the file position.
 *        On error returns -1
 *
 * @param f
 * @return int64_t
 */
int64_t bra_io_tell(bra_io_file_t* f);

/**
 * @brief Read the header from the give @p bf file.
 *        The file must be positioned at the beginning of the header.
 *        On error returns false and closes the file via @ref bra_io_close.
 *
 * @param bf
 * @param out_bh
 * @return true on success
 * @return false on error
 */
bool bra_io_read_header(bra_io_file_t* bf, bra_io_header_t* out_bh);

/**
 * @brief Write the bra header into @p bf with @p num_files.
 *        On error closes @p f via @ref bra_io_close.
 *
 * @param f
 * @param num_files
 * @return true
 * @return false
 */
bool bra_io_write_header(bra_io_file_t* f, const uint32_t num_files);

/**
 * @brief Read the bra footer into @p bf_out.
 *        On error closes @p f via @ref bra_io_close.
 *
 * @param f
 * @param bf_out
 * @return true
 * @return false
 */
bool bra_io_read_footer(bra_io_file_t* f, bra_io_footer_t* bf_out);

/**
 * @brief Write the footer into the file @p f.
 *        On error closes @p f via @ref bra_io_close.
 *
 * @param f
 * @param header_offset
 * @return true
 * @return false
 */
bool bra_io_write_footer(bra_io_file_t* f, const int64_t header_offset);

/**
 * @brief Open @p fn in read-binary mode, read the SFX footer to obtain the header offset,
 *        seek to it, and read the header.
 *
 * @param fn
 * @param out_bh
 * @param f
 * @return true on success (file positioned immediately after the header, at first entry)
 * @return false on error (errors during read/seek close @p f via @ref bra_io_close)
 */
bool bra_io_sfx_open_and_read_footer_header(const char* fn, bra_io_header_t* out_bh, bra_io_file_t* f);

/**
 * @brief Read the filename meta data information that is pointing in @p f and store it on @p mf.
 *
 * @param f
 * @param mf
 * @return true on success @p mf must be explicitly free via @ref bra_meta_file_free.
 * @return false on error closes @p f via @ref bra_io_close.
 */
bool bra_io_read_meta_file(bra_io_file_t* f, bra_meta_file_t* mf);

/**
 * @brief Write the filename meta data information given from @p mf in @p f.
 *
 * @param f
 * @param mf
 * @return true on success
 * @return false on error closes @p f via @ref bra_io_close.
 */
bool bra_io_write_meta_file(bra_io_file_t* f, const bra_meta_file_t* mf);

/**
 * @brief Copy from @p src to @p dst in chunks size of #BRA_MAX_CHUNK_SIZE for @p data_size bytes
 *        the files must be positioned at the correct read/write offsets.
 *        On failure closes both @p dst and @p src via @ref bra_io_close
 *
 * @param dst
 * @param src
 * @param data_size
 * @return true
 * @return false
 */
bool bra_io_copy_file_chunks(bra_io_file_t* dst, bra_io_file_t* src, const uint64_t data_size);

/**
 * @brief Move @p f forward of @p data_size bytes.
 *
 * @param f
 * @param data_size
 * @return true
 * @return false
 */
bool bra_io_skip_data(bra_io_file_t* f, const uint64_t data_size);

/**
 * @brief Encode a file or directory @p fn and append it to the open archive @p f.
 *        On error closes @p f via @ref bra_io_close.
 *
 * @param f Open archive handle.
 * @param fn NULL-terminated path to file or directory.
 * @return true on success
 * @return false on error (archive handle is closed)
 */
bool bra_io_encode_and_write_to_disk(bra_io_file_t* f, const char* fn);

/**
 * @brief Decode the current pointed internal file contained in @p f and write it to its relative path on disk.
 *        On error closes @p f via @ref bra_io_close.
 *
 * @pre  @p overwrite_policy != NULL.
 *
 * @note @p overwrite_policy is in/out: the callee may update it if the user selects
 *       a global choice (e.g., “[A]ll” / “N[o]ne”).
 *
 * @todo better split into decode and write to disk ?
 *
 * @param f
 * @param overwrite_policy[in/out]
 * @return true on success
 * @return false on error
 */
bool bra_io_decode_and_write_to_disk(bra_io_file_t* f, bra_fs_overwrite_policy_e* overwrite_policy);