#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <lib_bra_types.h>
#include <stdbool.h>

/**
 * @brief Log an error message and close the file.
 *
 * Logs a formatted error message using the provided verb and closes the file
 * wrapper to prevent resource leaks. Sets all file wrapper fields to safe values.
 *
 * @param f File wrapper to close (must not be NULL)
 * @param verb Action verb to complete the error message format "unable to %s"
 *
 * @note Idempotent - safe to call on already closed files.
 */
void bra_io_file_error(bra_io_file_t* f, const char* verb);

/**
 * @brief Log a file open error and close the file.
 *
 * Convenience wrapper for bra_io_file_error() specifically for open failures.
 * Logs "unable to open file" error message.
 *
 * @param f File wrapper to close (must not be NULL)
 *
 * @see bra_io_file_error
 */
void bra_io_file_open_error(bra_io_file_t* f);

/**
 * @brief Log a file read error and close the file.
 *
 * Convenience wrapper for bra_io_file_error() specifically for read failures.
 * Logs "unable to read from file" error message.
 *
 * @param f File wrapper to close (must not be NULL)
 *
 * @see bra_io_file_error
 */
void bra_io_file_read_error(bra_io_file_t* f);

/**
 * @brief Log a file seek error and close the file.
 *
 * Convenience wrapper for bra_io_file_error() specifically for seek failures.
 * Logs "unable to seek in file" error message.
 *
 * @param f File wrapper to close (must not be NULL)
 *
 * @see bra_io_file_error
 */
void bra_io_file_seek_error(bra_io_file_t* f);

/**
 * @brief Log a file write error and close the file.
 *
 * Convenience wrapper for bra_io_file_error() specifically for write failures.
 * Logs "unable to write to file" error message.
 *
 * @param f File wrapper to close (must not be NULL)
 *
 * @see bra_io_file_error
 */
void bra_io_file_write_error(bra_io_file_t* f);

/**
 * @brief Detect if the given file is an ELF executable.
 *
 * Opens the file and checks for the ELF magic signature (0x7F 'E' 'L' 'F').
 * Used to detect Linux/Unix executables that could contain SFX archives.
 *
 * @param fn Filename to check (must not be NULL)
 * @retval true If ELF magic signature is detected
 * @retval false If not ELF, file doesn't exist, or read error
 *
 * @note File is automatically closed after checking.
 *
 * @see bra_io_file_is_pe_exe
 * @see bra_io_file_can_be_sfx
 */
bool bra_io_file_is_elf(const char* fn);

/**
 * @brief Detect if the given file is a PE/EXE executable.
 *
 * Opens the file and checks for valid PE signature. Validates both DOS header
 * ("MZ") and PE signature. Used to detect Windows executables that could
 * contain SFX archives.
 *
 * @param fn Filename to check (must not be NULL)
 * @retval true If valid PE signature is detected
 * @retval false If not PE/EXE, file doesn't exist, or read error
 *
 * @note File is automatically closed after checking.
 *
 * @see bra_io_file_is_elf
 * @see bra_io_file_can_be_sfx
 */
bool bra_io_file_is_pe_exe(const char* fn);

/**
 * @brief Detect if the given file could be a self-extracting archive.
 *
 * Efficiently checks if the file is either ELF or PE/EXE format by opening
 * the file once and testing both signatures. Self-extracting archives are
 * typically wrapped in executable formats.
 *
 * @param fn Filename to check (must not be NULL)
 * @retval true If file appears to be ELF or PE/EXE (potential SFX wrapper)
 * @retval false If neither format detected, file doesn't exist, or read error
 *
 * @note More efficient than calling bra_io_file_is_elf() and bra_io_file_is_pe_exe() separately.
 * @note File is automatically closed after checking.
 *
 * @see bra_io_file_is_elf
 * @see bra_io_file_is_pe_exe
 */
bool bra_io_file_can_be_sfx(const char* fn);

/**
 * @brief Open a file with the specified mode.
 *
 * Opens the specified file and initializes the file wrapper. On failure,
 * no cleanup is required as the wrapper is left in a safe state.
 *
 * @param f File wrapper to initialize (must not be @c NULL)
 * @param fn Filename to open (must not be @c NULL)
 * @param mode File open mode (standard fopen modes: "r", "w", "rb", etc.)
 * @retval true On success - file is opened and wrapper is initialized
 * @retval false On error - wrapper is in safe state, no cleanup needed
 *
 * @note On failure, bra_io_file_close() is called automatically.
 * @note Common modes: "rb" (read binary), "wb" (write binary), "r+b" (read/write binary).
 *
 * @warning File wrapper must be uninitialized or previously closed.
 */
bool bra_io_file_open(bra_io_file_t* f, const char* fn, const char* mode);

/**
 * @brief Open a temporary file. It will be autodeleted when @ref bra_io_file_close.
 *
 * @param f  File wrapper to initialize (must not be NULL)
 * @retval true On success - file is opened and wrapper is initialized
 * @retval false On error - wrapper is in safe state, no cleanup needed
 */
bool bra_io_file_tmp_open(bra_io_file_t* f);

/**
 * @brief Close the file and reset the wrapper.
 *
 * Closes the underlying file handle, frees any allocated memory, and resets
 * all wrapper fields to safe values. Safe to call multiple times.
 *
 * @param f File wrapper to close (must not be @c NULL)
 *
 * @note Idempotent - safe to call on already closed or uninitialized wrappers.
 * @note Always succeeds - no return value needed.
 */
void bra_io_file_close(bra_io_file_t* f);

/**
 * @brief Seek to a specific position in the file.
 *
 * Changes the file position indicator using fseek semantics. Supports
 * seeking relative to start, current position, or end of file.
 *
 * @param f File wrapper (must not be NULL and file must be open)
 * @param offs Offset in bytes (can be negative for backward seeks)
 * @param origin Seek origin: SEEK_SET (start), SEEK_CUR (current), SEEK_END (end)
 * @retval true On successful seek
 * @retval false On seek error (invalid offset, closed file, etc.)
 *
 * @note For large files, use appropriate offset types to avoid overflow.
 *
 * @see bra_io_file_tell
 */
bool bra_io_file_seek(bra_io_file_t* f, const int64_t offs, const int origin);

/**
 * @brief Get the current file position.
 *
 * Returns the current position of the file pointer as a byte offset from
 * the beginning of the file.
 *
 * @param f File wrapper (must not be NULL and file must be open)
 * @return Current file position in bytes, or -1 on error
 *
 * @note Return value of -1 indicates error (file closed, invalid handle, etc.).
 *
 * @see bra_io_file_seek
 */
int64_t bra_io_file_tell(bra_io_file_t* f);

/**
 * @brief Read the archive footer from the file.
 *
 * Reads the BRA archive footer structure from the current file position.
 * The footer contains metadata about the archive structure and is typically
 * located at the end of archive files.
 *
 * @param f File wrapper positioned at footer location (must not be NULL)
 * @param bf_out Footer structure to populate (must not be NULL)
 * @retval true On successful footer read and validation
 * @retval false On read error, invalid footer, or validation failure
 *
 * @note On error, file is automatically closed via bra_io_file_close().
 * @note Footer structure is validated for correct magic and format.
 *
 * @see bra_io_file_write_footer
 */
bool bra_io_file_read_footer(bra_io_file_t* f, bra_io_footer_t* bf_out);

/**
 * @brief Write the archive footer to the file.
 *
 * Writes the BRA archive footer at the current file position. The footer
 * contains essential metadata including the header offset for archive parsing.
 *
 * @param f File wrapper positioned for footer write (must not be NULL)
 * @param header_offset Byte offset where archive header begins
 * @retval true On successful footer write
 * @retval false On write error
 *
 * @note On error, file is automatically closed via bra_io_file_close().
 * @note Footer includes magic signature, format version, and header offset.
 *
 * @see bra_io_file_read_footer
 */
bool bra_io_file_write_footer(bra_io_file_t* f, const int64_t header_offset);

/**
 * @brief Read a chunk of data from the file.
 *
 * Reads exactly buf_size bytes from the current file position into the
 * provided buffer. Used for reading fixed-size data blocks.
 *
 * @param src Source file wrapper (must not be NULL and file must be open)
 * @param buf Buffer to read data into (must not be NULL and have buf_size capacity)
 * @param buf_size Number of bytes to read (must be > 0)
 * @retval true On successful read of all requested bytes
 * @retval false On read error, EOF before reading all bytes, or I/O error
 *
 * @note On error, source file is automatically closed.
 * @note Partial reads are considered errors.
 *
 * @warning Buffer must have at least buf_size bytes available.
 */
bool bra_io_file_read_chunk(bra_io_file_t* src, void* buf, const size_t buf_size);

/**
 * @brief Read a chunk header. This is only present in compressed chunk.
 *
 * @param src
 * @param chunk_header
 * @retval true
 * @retval false
 */
bool bra_io_file_read_chunk_header(bra_io_file_t* src, bra_io_chunk_header_t* chunk_header);

/**
 * @brief Read file data in chunks and update CRC32.
 *
 * Reads the specified amount of data from the current file position in
 * BRA_MAX_CHUNK_SIZE chunks, updating the CRC32 checksum in the metadata
 * entry. Used for processing large files efficiently while maintaining
 * data integrity verification.
 *
 * @param src Source file wrapper positioned at start of data (must not be NULL)
 * @param data_size Total number of bytes to read
 * @param me Metadata entry to update with CRC32 (must not be NULL)
 * @retval true On successful read of all data with CRC32 updated
 * @retval false On read error, EOF, or I/O failure
 *
 * @note File position advances by data_size bytes on success.
 * @note On error, source file is automatically closed via bra_io_file_read_error().
 * @note CRC32 is calculated incrementally for memory efficiency.
 * @note After success, file is positioned at trailing CRC32 location.
 *
 * @see bra_io_file_copy_file_chunks
 */
bool bra_io_file_read_file_chunks(bra_io_file_t* src, const uint64_t data_size, bra_meta_entry_t* me);

/**
 * @brief Copy data between files in chunks with CRC32 calculation.
 *
 * Efficiently copies data from source to destination in BRA_MAX_CHUNK_SIZE
 * chunks while calculating CRC32 checksum. Both files must be positioned
 * at the correct read/write offsets before calling.
 *
 * @param dst Destination file wrapper positioned for writing (must not be NULL)
 * @param src Source file wrapper positioned for reading (must not be NULL)
 * @param data_size Total number of bytes to copy
 * @param me Metadata entry to update with CRC32 (must not be NULL)
 * @retval true On successful copy of all data with CRC32 updated
 * @retval false On read/write error or I/O failure
 *
 * @note Both files advance by data_size bytes on success.
 * @note On error, both files are automatically closed via bra_io_file_close().
 * @note CRC32 is calculated during the copy operation for efficiency.
 * @note Memory usage is limited to BRA_MAX_CHUNK_SIZE regardless of data_size.
 *
 * @see bra_io_file_read_file_chunks
 * @see bra_io_file_compress_file_chunks
 */
bool bra_io_file_copy_file_chunks(bra_io_file_t* dst, bra_io_file_t* src, const uint64_t data_size, bra_meta_entry_t* me);

/**
 * @brief Skip data by advancing file position.
 *
 * Advances the file position by the specified number of bytes without
 * reading the data. More efficient than reading and discarding data
 * when the content is not needed.
 *
 * @param f File wrapper (must not be NULL and file must be open)
 * @param data_size Number of bytes to skip
 * @retval true On successful position advance
 * @retval false On seek error or if skip would go past EOF
 *
 * @note Equivalent to bra_io_file_seek(f, data_size, SEEK_CUR).
 * @note More efficient than reading data into a temporary buffer.
 *
 * @see bra_io_file_seek
 */
bool bra_io_file_skip_data(bra_io_file_t* f, const uint64_t data_size);

/**
 * @brief Compress and copy file data in chunks.
 *
 * Reads data from source file, compresses it using the configured compression
 * algorithm, and writes the compressed data to the destination file. Updates
 * metadata entry with compression information and CRC32 of original data.
 *
 * @param dst Destination file for compressed data (must not be NULL)
 * @param src Source file for original data (must not be NULL)
 * @param data_size Size of original data to compress
 * @param me Metadata entry to update with compression info (must not be NULL)
 * @retval true On successful compression and write
 * @retval false On compression error, read/write failure, or insufficient memory
 *
 * @note Both files must be positioned correctly before calling.
 * @note Compression ratio and method are stored in metadata entry.
 * @note CRC32 is calculated on original (uncompressed) data.
 *
 * @see bra_io_file_decompress_file_chunks
 * @see bra_io_file_copy_file_chunks
 */
bool bra_io_file_compress_file_chunks(bra_io_file_t* dst, bra_io_file_t* src, const uint64_t data_size, bra_meta_entry_t* me);

/**
 * @brief Decompress file data in chunks.
 *
 * Reads compressed data from source file, decompresses it using the algorithm
 * specified in the metadata entry, and writes the original data to the
 * destination file. Verifies data integrity using stored CRC32.
 *
 * @param dst Destination file for decompressed data (must not be NULL)
 * @param src Source file containing compressed data (must not be NULL)
 * @param data_size Size of compressed data to reads
 * @param me Metadata entry with compression info and CRC32 (must not be NULL)
 * @retval true On successful decompression and CRC32 verification
 * @retval false On decompression error, CRC32 mismatch, or I/O failure
 *
 * @note Both files must be positioned correctly before calling.
 * @note Decompression method is determined from metadata entry.
 * @note CRC32 verification ensures data integrity after decompression.
 *
 * @see bra_io_file_compress_file_chunks
 * @see bra_io_file_copy_file_chunks
 */
bool bra_io_file_decompress_file_chunks(bra_io_file_t* dst, bra_io_file_t* src, const uint64_t data_size, bra_meta_entry_t* me);

#ifdef __cplusplus
}
#endif
