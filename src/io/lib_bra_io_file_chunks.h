#pragma once

#include <lib_bra_types.h>

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/**
 * @brief Read a chunk header from the file. This is only present in compressed chunks.
 *        The chunk header is present in every chunk.
 *
 * @param src Source file wrapper (must not be @c NULL and file must be open)
 * @param chunk_header the chunk header to be populated with the data read from disk.
 * @retval true On successful read of all requested bytes
 * @retval false On read error, EOF before reading all bytes, or I/O error
 *
 * @note On error, source file is automatically closed.
 * @note Partial reads are considered errors.
 */
bool bra_io_file_chunks_read_header(bra_io_file_t* src, bra_io_chunk_header_t* chunk_header);

/**
 * @brief Write a chunk header to the file. This is only present in compressed chunks.
 *        The chunk header is present in every chunk.
 *
 * @param dst  Destination file wrapper (must be not @c NULL and file must be open)
 * @param chunk_header the chunk header to be written into @p dst
 * @retval true On successful write.
 * @retval false On write error
 *
 * @note On error, @p dst is automatically closed.
 */
bool bra_io_file_chunks_write_header(bra_io_file_t* dst, const bra_io_chunk_header_t* chunk_header);

/**
 * @brief Read file data in chunks and update CRC32.
 *
 * Reads the specified amount of data from the current file position in
 * #BRA_MAX_CHUNK_SIZE chunks, updating the CRC32 checksum in the metadata
 * entry. Used for processing large files efficiently while maintaining
 * data integrity verification.
 *
 * @param src Source file wrapper positioned at start of data (must not be @c NULL)
 * @param data_size Total number of bytes to read
 * @param me Metadata entry to update with CRC32 (must not be @c NULL)
 * @retval true On successful read of all data with CRC32 updated
 * @retval false On read error, EOF, or I/O failure
 *
 * @note File position advances by data_size bytes on success.
 * @note On error, source file is automatically closed via @ref bra_io_file_read_error().
 * @note CRC32 is calculated incrementally for memory efficiency.
 * @note After success, file is positioned at trailing CRC32 location.
 *
 * @see bra_io_file_chunks_copy_file
 */
bool bra_io_file_chunks_read_file(bra_io_file_t* src, const uint64_t data_size, bra_meta_entry_t* me);

/**
 * @brief Copy data between files in chunks without CRC32 calculation.
 *
 * Efficiently copies data from source to destination in #BRA_MAX_CHUNK_SIZE
 * chunks. Both files must be positioned
 * at the correct read/write offsets before calling.
 *
 * @param dst Destination file wrapper positioned for writing (must not be @c NULL)
 * @param src Source file wrapper positioned for reading (must not be @c NULL)
 * @param data_size Total number of bytes to copy
 * @param me meta entry
 * @param compute_crc32 if true compute the crc32 while copying the @p src to @p dst.
 *
 * @retval true On successful copy of all data
 * @retval false On read/write error or I/O failure
 *
 * @note Both files advance by data_size bytes on success.
 * @note On error, both files are automatically closed via @ref bra_io_file_close().
 * @note Memory usage is limited to #BRA_MAX_CHUNK_SIZE regardless of data_size.
 *
 * @see bra_io_file_chunks_read_file
 * @see bra_io_file_chunks_compress_file
 */
bool bra_io_file_chunks_copy_file(bra_io_file_t* dst, bra_io_file_t* src, const uint64_t data_size, bra_meta_entry_t* me, const bool compute_crc32);

/**
 * @brief Compress and copy file data in chunks.
 *
 * Reads data from source file, compresses it using the configured compression
 * algorithm, and writes the compressed data to the destination file. Updates
 * metadata entry with compression information and CRC32 of original data.
 *
 * @param dst Destination file for compressed data (must not be @c NULL)
 * @param src Source file for original data (must not be @c NULL)
 * @param data_size Size of original data to compress
 * @param me Metadata entry to update with compression info (must not be @c NULL)
 * @retval true On successful compression and write
 * @retval false On compression error, read/write failure, or insufficient memory
 *
 * @note Both files must be positioned correctly before calling.
 * @note Compression ratio and method are stored in metadata entry.
 * @note CRC32 is calculated on original (uncompressed) data.
 *
 * @see bra_io_file_chunks_decompress_file
 * @see bra_io_file_chunks_copy_file
 */
bool bra_io_file_chunks_compress_file(bra_io_file_t* dst, bra_io_file_t* src, const uint64_t data_size, bra_meta_entry_t* me);

/**
 * @brief Decompress file data in chunks.
 *
 * Reads compressed data from source file, decompresses it using the algorithm
 * specified in the metadata entry, and writes the original data to the
 * destination file. Verifies data integrity using stored CRC32.
 *
 * @param dst Destination file for decompressed data (must not be @c NULL)
 * @param src Source file containing compressed data (must not be @c NULL)
 * @param data_size Size of compressed data to read
 * @param me Metadata entry with compression info and CRC32 (must not be NULL)
 * @retval true On successful decompression and CRC32 verification
 * @retval false On decompression error, CRC32 mismatch, or I/O failure
 *
 * @note Both files must be positioned correctly before calling.
 * @note Decompression method is determined from metadata entry.
 * @note CRC32 verification ensures data integrity after decompression.
 *
 * @see bra_io_file_chunks_compress_file
 * @see bra_io_file_chunks_copy_file
 */
bool bra_io_file_chunks_decompress_file(bra_io_file_t* dst, bra_io_file_t* src, const uint64_t data_size, bra_meta_entry_t* me);
