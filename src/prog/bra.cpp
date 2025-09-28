#include <lib_bra.h>
#include <io/lib_bra_io_file.h>
#include <io/lib_bra_io_file_ctx.h>

#include <log/bra_log.h>
#include <fs/bra_fs.hpp>
#include <version.h>

#include <BraProgram.hpp>

#include <filesystem>
#include <string>
#include <set>
#include <algorithm>
#include <limits>
#include <map>

#include <cstdint>
#include <cstdio>


using namespace std;

namespace fs = std::filesystem;

//////////////////////////////////////////////////////////////////////////

class Bra : public BraProgram
{
private:
    Bra(const Bra&)            = delete;
    Bra& operator=(const Bra&) = delete;
    Bra(Bra&&)                 = delete;
    Bra& operator=(Bra&&)      = delete;

    std::set<fs::path>                     m_files;
    std::map<fs::path, std::set<fs::path>> m_tree;
    fs::path                               m_out_filename;
    uint32_t                               m_tot_files         = 0;
    uint32_t                               m_written_num_files = 0;
    int64_t                                m_header_offset     = -1;
    bool                                   m_sfx               = false;
    bool                                   m_recursive         = false;
    bool                                   m_compress          = false;
    int                                    m_progress_width    = 0;


protected:
    void help_usage() const override
    {
        bra_log_printf("  %s [-s] [-r] -o <output_file> <input_file1> [<input_file2> ...]\n", fs::path(m_argv0).filename().string().c_str());
        bra_log_printf("The <output_file> will have %s (or %s with --sfx)\n", BRA_FILE_EXT, BRA_SFX_FILE_EXT);
    };

    void help_example() const override
    {
        bra_log_printf("  bra -o test test.txt\n");
        bra_log_printf("  bra -o test *.txt\n");
        bra_log_printf("\n");
        bra_log_printf("<input_file>      : path to an existing file or a wildcard pattern\n");
        bra_log_printf("                    Use shell wildcards like 'dir/*' to include files from directories;\n");
        bra_log_printf("                    bare directory arguments are ignored without recursion.\n");
    };

    void help_options() const override
    {
        bra_log_printf("--sfx        | -s : generate a self-extracting archive\n");
        bra_log_printf("--recursive  | -r : recursively scan files and directories. \n");
        // bra_log_printf("--update     | -u : update an existing archive with missing files from input.\n");
        // bra_log_printf("--test       | -t : test an existing archive.\n");
        bra_log_printf("--out        | -o : <output_filename> it takes the path of the output file.\n");
        bra_log_printf("                    If the extension %s is missing it will be automatically added.\n", BRA_FILE_EXT);
        bra_log_printf("-c                : compress files (alpha version)\n");
    };

    int parseArgs_minArgc() const override { return 2; }

    std::optional<bool> parseArgs_option(const int argc, const char* const argv[], int& i, const std::string& s) override
    {
        if (s == "--out" || s == "-o")
        {
            // next arg is output file
            ++i;
            if (i >= argc)
            {
                bra_log_error("%s missing argument <output_filename>", s.c_str());
                return false;
            }

            m_out_filename = argv[i];
        }
        else if (s == "--sfx" || s == "-s")
            m_sfx = true;
        else if (s == "--recursive" || s == "-r")
        {
            m_recursive = true;
        }
        else if (s == "-c")
            m_compress = true;
        else
            return nullopt;

        return true;
    }

    bool parseArgs_file(const std::filesystem::path& p) override
    {
        if (!m_files.insert(p).second)
        {
            bra_log_warn("duplicate file given in input: %s\n", p.string().c_str());
            return true;
        }

        return true;
    };

    std::optional<bool> parseArgs_dir(const std::filesystem::path& p) override
    {
        if (m_recursive)
        {
            if (!m_files.insert(p).second)
                bra_log_warn("duplicate dir given in input: %s\n", p.string().c_str());
            else if (!bra::fs::search_wildcard(p / "*", m_files, m_recursive))
            {
                bra_log_error("path not valid: %s", p.string().c_str());

                return false;
            }
        }

        return true;
    };

    std::optional<bool> parseArgs_wildcards(const std::filesystem::path& p) override
    {
        return bra::fs::search_wildcard(p, m_files, m_recursive);
    }

    bool validateArgs() override
    {
        if (m_files.empty())
        {
            bra_log_error("no input file provided");
            return false;
        }

        if (!bra::fs::file_set_add_dirs(m_files))
            return false;

#ifndef NDEBUG
        bra_log_verbose("Detected files:");
        for (const auto& file : m_files)
            bra_log_verbose("- %s", file.string().c_str());
#endif

        // TODO: Here could also start encoding the filenames
        //       and use them in compressed format to save on disk
        //       only if it less space (but it might be not).
        //       Need to test eventually later on

        if (m_out_filename.empty())
        {
            bra_log_error("no output file provided");
            return false;
        }

        // adjust input file extension
        if (m_sfx)
        {
            m_out_filename = bra::fs::filename_sfx_adjust(m_out_filename, true);
            // locate sfx bin
            if (!bra::fs::file_exists(BRA_SFX_FILENAME))
            {
                bra_log_error("unable to find %s-SFX module", BRA_NAME);
                return false;
            }

            // check if SFX_TMP exists...
            const auto overwrite = bra::fs::file_exists_ask_overwrite(m_out_filename, m_overwrite_policy, true);
            if (overwrite)
            {
                if (!*overwrite)
                    return false;

                bra_log_printf("Overwriting file: %s\n", m_out_filename.string().c_str());
            }
        }
        else
        {
            m_out_filename = bra::fs::filename_archive_adjust(m_out_filename);
        }

        fs::path p = m_out_filename;
        if (m_sfx)
            p = p.replace_extension(BRA_SFX_FILE_EXT);

        // TODO: this might not be ok
        if (auto res = bra::fs::file_exists_ask_overwrite(p, m_overwrite_policy, true))
        {
            if (!*res)
                return false;

            bra_log_printf("Overwriting file: %s\n", p.string().c_str());

            // check it is not present in the input files
            m_files.erase(p);
            if (p != m_out_filename)
                m_files.erase(m_out_filename);
        }
        else    // the output directory might not exists...
        {
            // create the parent directory if needed.
            const fs::path pof = m_out_filename.parent_path();
            if (!pof.empty())
            {
                if (!bra::fs::dir_make(pof))
                    return false;
            }
        }

// build tree from the set file list.
#ifndef NDEBUG
        bra_log_verbose("Building Tree from %zu files...", m_files.size());
#endif

        m_tree.clear();
        size_t tot_files = 0;
        if (!bra::fs::make_tree(m_files, m_tree, tot_files))
            return false;

#ifndef NDEBUG
        if (m_files.size() != tot_files)
            bra_log_debug("set<->tree size mismatch: %zu <-> %zu", m_files.size(), tot_files);
#endif

        if (tot_files == 0)
        {
            bra_log_error("no input entries to archive");
            return false;
        }
        else if (tot_files > std::numeric_limits<uint32_t>::max())
        {
            bra_log_critical("Too many entries to archive: %zu", tot_files);
            return false;
        }

        m_tot_files = static_cast<uint32_t>(tot_files);

#ifndef NDEBUG
        bra_log_verbose("Built Tree : [TOT_FILES: %u]", m_tot_files);
        for (const auto& [dir, files] : m_tree)
        {
            if (m_recursive)
                bra_log_verbose("- [%s]", dir.string().c_str());
            for (const auto& file : files)
                bra_log_verbose("- [%s]/%s", dir.string().c_str(), file.string().c_str());
        }
#endif

        m_files.clear();
        m_progress_width = snprintf(nullptr, 0, "%u", m_tot_files);
        if (m_progress_width < 0)
        {
            bra_log_critical("internal error");
            return false;
        }

        return true;
    };

    bool run_encode(const std::filesystem::path& p)
    {
        auto path = p;

        // write Progress (+1 because it is the file that is going to be written now)
        // TODO: add progress bar?
        bra_log_printf("[%*u/%u] ", m_progress_width, m_written_num_files + 1, m_tot_files);

        // no need to sanitize it again, but it won't hurt neither
        if (!bra::fs::try_sanitize(path))
        {
            bra_log_error("invalid path: %s", p.string().c_str());
            return false;
        }

        const string fn = path.generic_string();
        // TODO: add the argument for compression/store
        // TODO: also do the automatic decision based on file type/size?
        //       (e.g. do not compress very small files or already compressed files)
        //       or if the compression is bigger than the original size.
        //       This will be anyway part of the compression function.
        //       By default should be compression enabled.
        if (!bra_io_file_ctx_encode_and_write_to_disk(&m_ctx, fn.c_str(), m_compress))
            return false;

        return true;
    }

    int run_prog() override
    {
        string   out_fn = m_out_filename.generic_string();
        fs::path sfx_path;

        if (m_sfx)
        {
            sfx_path = out_fn;    // here is with BRA_SFX_TMP_FILE_EXT
            sfx_path.replace_extension(BRA_SFX_FILE_EXT);

            bra_log_printf("Creating Self-Extracting Archive: %s\n", sfx_path.string().c_str());
            if (!fs::copy_file(BRA_SFX_FILENAME, out_fn, fs::copy_options::overwrite_existing))
            {
            BRA_SFX_IO_ERROR:
                bra_log_error("unable to create a %s-SFX file", BRA_NAME);
                return 2;
            }

            // header
            if (!bra_io_file_ctx_sfx_open(&m_ctx, out_fn.c_str(), "rb+"))
                goto BRA_SFX_IO_ERROR;

            // save the start of the payload for later...
            m_header_offset = bra_io_file_tell(&m_ctx.f);
            if (m_header_offset < 0L)
            {
                bra_io_file_error(&m_ctx.f, "tell");
                return 2;
            }
        }
        else
        {
            bra_log_printf("Archiving Into: %s\n", out_fn.c_str());

            // header
            if (!bra_io_file_ctx_open(&m_ctx, out_fn.c_str(), "wb"))
                return 1;
        }

        if (!bra_io_file_ctx_write_header(&m_ctx, static_cast<uint32_t>(m_tot_files)))
            return 1;

        m_written_num_files = 0;
        for (const auto& [dir, files] : m_tree)
        {
            // write dir first...
            if (!dir.empty())
            {
                fs::path p = dir;
                if (!run_encode(p))
                    return 3;

                ++m_written_num_files;
            }

            // ...then all its files
            for (const auto& fn_ : files)
            {
                fs::path p = dir / fn_;
                if (!run_encode(p))
                    return 3;

                ++m_written_num_files;
            }
        }

#ifndef NDEBUG
        if (m_written_num_files != m_tot_files)
            bra_log_warn("written entries (%u) != header count (%u)", m_written_num_files, m_tot_files);
#endif

        if (m_sfx)
        {
            if (!bra_io_file_write_footer(&m_ctx.f, m_header_offset))
                return 2;

            if (!bra_io_file_ctx_close(&m_ctx))
                return 1;

            if (!bra::fs::file_rename(out_fn, sfx_path))
                goto BRA_SFX_IO_ERROR;

            // add executable permission (on UNIX)
            if (!bra::fs::file_permissions(sfx_path, fs::perms::owner_exec, fs::perm_options::add))
                bra_log_warn("unable to set executable bit on %s", sfx_path.string().c_str());
        }
        else
        {
            if (!bra_io_file_ctx_close(&m_ctx))
                return 1;
        }

        return 0;
    }

public:
    Bra()          = default;
    virtual ~Bra() = default;
};

/////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
    Bra bra_prog;

    return bra_prog.run(argc, argv);
}
