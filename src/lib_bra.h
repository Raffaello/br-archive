#pragma once


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <lib_bra_defs.h>

#pragma pack(push, 1)

// typedef struct bra_meta_directory_t
// {
//     uint32_t num_subdirs;    //!< subdirectories, it can be 0 if none.
//     // bra_meta_directory_t dirs[];
//     uint32_t num_files;    //!< number of files, it must be at least 1.
//     // bra_meta_file_t files[];
// } bra_meta_directory_t;

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

    // TODO: put num_directories. 1 must always be present and then
    //       num_files inside the bra_meta_directory_t
    //       it means that is an internal directory containing files.
    //
    //       this leads to save chars on the filename string,
    //       for multiple files in the same directory
    //       also it will mostly save the slash.
    // TODO: ISSUE: how about nested sub-directories instead?
    //       so the directory should contain a num_directory.
    //       basically the header is just follow of a bra_directory_t instead.
    //   OR: the directory is a chunk containing the file
    //       and after a directory chunk, there could be an optional directory chunk etc
    //       but this leads to longer directory name for sub-dirs because are all directory relative to the root level e.g.:
    //       dir1,  dir1/subdir1
    //       to avoid directory name duplication when storying a subdirectory is required.
    //       the problem is when there are none 4 bytes to store a 0 is a waste of space.
    //       and in a case like e.g.:   dir1, dir2, dir3 dir4, dir5, there are 4*5 =20 bytes just to store a zero information, very annoying.
    //   SO: dir1, dir1/subdir1  should be stored as dir1 -> subdir1 as a "nested" chunk in dir1
    //       to know if there are nested chunks must be put in the chunk itself..
    //   OR: simply store even the directory as files, and check at runtime if it is,
    //       to decode a dir stored as files? it's content is a directory_meta_header ? how to recognize it?
    //       It should have attributes in the file and store the directory bit in it...
    //   SO: need to store file attributes first as well.
    //       and when file dir is an attribute, that file name became the prefix of the following files
    //       until another file-dir name comes up. so the order will be important.
} bra_io_header_t;

/**
 * @brief BR-Sfx File Footer.
 */
typedef struct bra_io_footer_t
{
    uint32_t magic;          //!< 'BR-x'
    int64_t  data_offset;    //!< where the data chunk start from the beginning of the file
} bra_io_footer_t;

#pragma pack(pop)

typedef struct bra_io_file_t
{
    FILE* f;
    char* fn;
} bra_io_file_t;

/**
 * @brief This is the metadata of each file stored in a BR-archive.
 *        The file data is just after for @p data_size bytes.
 */
typedef struct bra_meta_file_t
{
    // TODO: add CRC ... file permissions, file attributes, etc... ?
    uint8_t  attributes;    //!< file attributes: 0=regular file, 1=directory
    uint8_t  name_size;
    char*    name;
    uint64_t data_size;
} bra_meta_file_t;

/////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief strdup()
 * @todo remove when switching to C23
 *
 * @param str
 * @return char*
 */
char* bra_strdup(const char* str);

/**
 * @brief print error message and close file.
 *
 * @param bf
 */
void bra_io_read_error(bra_io_file_t* bf);

/**
 * @brief open the file @p fn in the @p mode
 *        the @p will be overwritten.
 *        On failure there is no need to call @ref bra_io_close
 *
 * @param bf
 * @param fn
 * @param mode
 * @return true on success
 * @return false on error
 */
bool bra_io_open(bra_io_file_t* bf, const char* fn, const char* mode);

/**
 * @brief close file, free internal memory and set fields to NULL.
 *
 * @param bf
 */
void bra_io_close(bra_io_file_t* bf);

/**
 * @brief seek file at position @p offs.
 *  *
 * @param f
 * @param offs
 * @param origin SEEK_SET, SEEK_CUR, SEEK_END
 * @return true
 * @return false
 */
bool bra_io_seek(bra_io_file_t* f, const int64_t offs, const int origin);

/**
 * @brief tell the file position.
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
 *
 * @param f
 * @param data_offset
 * @return true
 * @return false
 */
bool bra_io_write_footer(bra_io_file_t* f, const int64_t data_offset);

/**
 * @brief Read the filename meta data information that is pointing in @p f and store it on @p mf.
 *        On error closes @p f via @ref bra_io_close.
 *        On success @mf must be explicitly free via @ref bra_meta_file_free.
 *
 * @param f
 * @param mf
 * @return true On success
 * @return false On error
 */
bool bra_io_read_meta_file(bra_io_file_t* f, bra_meta_file_t* mf);

/**
 * @brief
 *
 * @param f
 * @param mf
 * @return true
 * @return false
 */
bool bra_io_write_meta_file(bra_io_file_t* f, const bra_meta_file_t* mf);

/**
 * @brief Free any eventual content on @p mf.
 *
 * @param mf
 */
void bra_meta_file_free(bra_meta_file_t* mf);

/**
 * @brief Copy from @p src to @p dst in chunks size of #MAX_CHUNK_SIZE for @p data_size bytes
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
 * @brief Decode the current pointed internal file contained in @p f and write it to its relative path on disk.
 *        On error calls @ref bra_io_close with param @p f closing it.
 *
 *
 * @param f
 * @return true on success
 * @return false on error
 */
bool bra_io_decode_and_write_to_disk(bra_io_file_t* f);

#ifdef __cplusplus
}
#endif
