#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <lib_bra_types.h>
#include <stdbool.h>

/**
 * @brief Open the file @p fn in @p mode. Clear the @p ctx state.
 *        On failure there is no need to call @ref bra_io_file_ctx_close.
 *
 * @param ctx[out]
 * @param fn
 * @param mode
 * @retval true
 * @retval false
 */
bool bra_io_file_ctx_open(bra_io_file_ctx_t* ctx, const char* fn, const char* mode);

/**
 * @brief Open the file @p fn in @p mode and seek to the beginning of the SFX footer (EOF - sizeof(bra_io_footer_t)).
 *        On failure there is no need to call @ref bra_io_file_ctx_close.
 *
 * @param ctx
 * @param fn
 * @param mode   @c fopen modes
 * @retval true  on success (file positioned at start of footer; ready for bra_io_file_read_footer)
 * @retval false on error (seek errors close @p ctx->f via @ref bra_io_file_close)
 */
bool bra_io_file_ctx_sfx_open(bra_io_file_ctx_t* ctx, const char* fn, const char* mode);

/**
 * @brief
 *
 * @param ctx
 * @retval true
 * @retval false
 */
bool bra_io_file_ctx_close(bra_io_file_ctx_t* ctx);

/**
 * @brief Read the header from the file associated to @p ctx file.
 *        The file must be positioned at the beginning of the header.
 *        On error returns false and closes the file via @ref bra_io_file_close.
 *
 * @param ctx[in]
 * @param out_bh
 * @retval true on success
 * @retval false on error
 */
bool bra_io_file_ctx_read_header(bra_io_file_ctx_t* ctx, bra_io_header_t* out_bh);

/**
 * @brief Write the bra header into @p ctx->f with @p num_files.
 *        On error closes @p ctx->f via @ref bra_io_file_close.
 *
 * @param ctx[in,out]
 * @param num_files
 * @retval true
 * @retval false
 */
bool bra_io_file_ctx_write_header(bra_io_file_ctx_t* ctx, const uint32_t num_files);

/**
 * @brief Open @p fn in read-binary mode, read the SFX footer to obtain the header offset,
 *        seek to it, and read the header.
 *
 * @param fn
 * @param out_bh
 * @param ctx[out]
 * @retval true on success (file positioned immediately after the header, at first entry)
 * @retval false on error (errors during read/seek close @p ctx->f via @ref bra_io_file_close)
 */
bool bra_io_file_ctx_sfx_open_and_read_footer_header(const char* fn, bra_io_header_t* out_bh, bra_io_file_ctx_t* ctx);

/**
 * @brief Read the filename meta data information that is pointing in @p ctx->f and store it on @p mf.
 *        @p mf must be free via @ref bra_meta_file_free.
 *
 * @param ctx[in,out]
 * @param mf
 * @retval true on success @p mf must be explicitly free via @ref bra_meta_file_free.
 * @retval false on error closes @p ctx->f via @ref bra_io_file_close.
 */
bool bra_io_file_ctx_read_meta_file(bra_io_file_ctx_t* ctx, bra_meta_file_t* mf);

/**
 * @brief Write the filename meta data information given from @p mf in @p ctx->f.
 *
 * @param ctx[in,out]
 * @param mf[in,out]
 * @retval true on success
 * @retval false on error closes @p ctx->f via @ref bra_io_file_close.
 */
bool bra_io_file_ctx_write_meta_file(bra_io_file_ctx_t* ctx, bra_meta_file_t* mf);

/**
 * @brief Encode a file or directory @p fn and append it to the open archive @p ctx->f.
 *        On error closes @p ctx->f via @ref bra_io_file_close.
 *
 * @param ctx[in,out]
 * @param fn NULL-terminated path to file or directory.
 * @retval true on success
 * @retval false on error (archive handle is closed)
 */
bool bra_io_file_ctx_encode_and_write_to_disk(bra_io_file_ctx_t* ctx, const char* fn);

/**
 * @brief Decode the current pointed internal file contained in @p ctx->f and write it to its relative path on disk.
 *        On error closes @p ctx->f via @ref bra_io_file_close.
 *
 * @pre  @p overwrite_policy != NULL.
 *
 * @note @p overwrite_policy is in/out: the callee may update it if the user selects
 *       a global choice (e.g., “[A]ll” / “N[o]ne”).
 *
 * @todo better split into decode and write to disk ?
 *
 * @param ctx[in,out]
 * @param overwrite_policy[in/out]
 * @retval true on success
 * @retval false on error
 */
bool bra_io_file_ctx_decode_and_write_to_disk(bra_io_file_ctx_t* ctx, bra_fs_overwrite_policy_e* overwrite_policy);

#ifdef __cplusplus
}
#endif