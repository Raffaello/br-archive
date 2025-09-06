#include <lib_bra.h>
#include <bra_log.h>
#include <bra_fs.hpp>
#include <version.h>

#include <BraProgram.hpp>

#include <filesystem>
#include <string>
#include <set>
#include <algorithm>

#include <cstdint>


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

    std::set<fs::path> m_files;
    fs::path           m_out_filename;
    bool               m_sfx = false;

    bra_io_file_t m_f2{};

protected:
    void help_usage() const override
    {
        bra_log_printf("  %s [-s] -o <output_file> <input_file1> [<input_file2> ...]\n", fs::path(m_argv0).filename().string().c_str());
        bra_log_printf("The [output_file] will have %s (or %s with --sfx)\n", BRA_FILE_EXT, BRA_SFX_FILE_EXT);
    };

    void help_example() const override
    {
        bra_log_printf("  bra -o test test.txt\n");
        bra_log_printf("  bra -o test *.txt\n");
        bra_log_printf("\n");
        bra_log_printf("(input_file): path to an existing file or a wildcard pattern\n");
        bra_log_printf("              Directories expand to <dir/*> (non-recursive); empty directories are ignored.\n");
    };

    void help_options() const override
    {
        bra_log_printf("--sfx    | -s : generate a self-extracting archive\n");
        bra_log_printf("--update | -u : update an existing archive with missing files from input.\n");
        bra_log_printf("--out    | -o : <output_filename> it takes the path of the output file.\n");
        bra_log_printf("                If the extension %s is missing it will be automatically added.\n", BRA_FILE_EXT);
    };

    int parseArgs_minArgc() const override { return 2; }

    std::optional<bool> parseArgs_option(const int argc, const char* const argv[], int& i, std::string_view s) override
    {
        if (s == "--out" || s == "-o")
        {
            // next arg is output file
            ++i;
            if (i >= argc)
            {
                bra_log_error("%.*s missing argument <output_filename>", static_cast<int>(s.size()), s.data());
                return false;
            }

            m_out_filename = argv[i];
        }
        else if (s == "--sfx" || s == "-s")
            m_sfx = true;
        else
            return nullopt;

        return true;
    }

    void parseArgs_adjustFilename([[maybe_unused]] std::filesystem::path& p) override {};

    bool parseArgs_file(const std::filesystem::path& p) override
    {
        if (!m_files.insert(p).second)
        {
            bra_log_warn("duplicate file given in input: %s\n", p.string().c_str());
            return true;
        }

        return true;
    };

    bool parseArgs_dir(const std::filesystem::path& p) override
    {
        // Match exactly the directory:
        // append `/*` to include all files in it (non-recursive).
        if (!bra::fs::search_wildcard(p / "*", m_files))
        {
            bra_log_error("path not valid: %s", p.string().c_str());
            return false;
        }
        return true;
    };

    bool validateArgs() override
    {
        if (m_files.empty())
        {
            bra_log_error("no input file provided");
            return false;
        }

        if (m_files.size() > std::numeric_limits<uint32_t>::max())
        {
            bra_log_critical("Too many files to be archived\n");
            return false;
        }

        if (!bra::fs::file_set_add_dir(m_files))
            return false;

#ifndef NDEBUG
        bra_log_debug("Detected files:");
        for (const auto& m_f : m_files)
            bra_log_debug("- %s", m_f.string().c_str());
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

        return true;
    };

    int run_prog() override
    {
        string out_fn = m_out_filename.generic_string();

        // header
        if (!bra_io_open(&m_f, out_fn.c_str(), "wb"))
            return 1;

        bra_log_printf("Archiving into %s\n", out_fn.c_str());
        if (!bra_io_write_header(&m_f, static_cast<uint32_t>(m_files.size())))
            return 1;

        uint32_t written_num_files = 0;
        for (const auto& fn_ : m_files)
        {
            fs::path p = fn_;

// TODO: write Progress bar?
#if 0
            bra_log_printf("[%u/%u] ", written_num_files, static_cast<uint32_t>(m_files.size()));
#endif
            // no need to sanitize it again, but it won't hurt neither
            if (!bra::fs::try_sanitize(p))
            {
                bra_log_error("invalid path: %s", fn_.string().c_str());
                return 1;
            }

            const string fn = p.generic_string();
            if (!bra_io_encode_and_write_to_disk(&m_f, fn.c_str()))
                return 1;
            else
                ++written_num_files;
        }

        bra_io_close(&m_f);

        // TODO: doing an SFX should be in the lib_bra
        if (m_sfx)
        {
            fs::path sfx_path = m_out_filename;
            sfx_path.replace_extension(BRA_SFX_FILE_EXT);

            bra_log_printf("creating Self-extracting archive %s...", sfx_path.string().c_str());

            if (!fs::copy_file(BRA_SFX_FILENAME, sfx_path, fs::copy_options::overwrite_existing))
            {
            BRA_SFX_IO_ERROR:
                bra_log_error("unable to create a %s-SFX file", BRA_NAME);
                return 2;
            }

            if (!bra_io_open(&m_f, sfx_path.string().c_str(), "rb+"))
                goto BRA_SFX_IO_ERROR;

            if (!bra_io_seek(&m_f, 0, SEEK_END))
            {
                bra_io_file_seek_error(&m_f);
                return 2;
            }

            // save the start of the payload for later...
            const int64_t header_offset = bra_io_tell(&m_f);
            if (header_offset < 0L)
            {
                bra_io_file_error(&m_f, "tell");
                return 2;
            }

            // append bra file
            if (!bra_io_open(&m_f2, out_fn.c_str(), "rb"))
                return 2;

            auto file_size = bra::fs::file_size(out_fn);
            if (!file_size)
                return 2;

            if (!bra_io_copy_file_chunks(&m_f, &m_f2, *file_size))
                return 2;

            bra_io_close(&m_f2);
            // write footer
            if (!bra_io_write_footer(&m_f, header_offset))
            {
                bra_io_file_write_error(&m_f);
                return 2;
            }

            bra_io_close(&m_f);

            // add executable permission on UNIX
            // #if defined(__unix__) || defined(__APPLE__)
            if (!bra::fs::file_permissions(sfx_path,
                                           fs::perms::owner_exec | fs::perms::owner_read | fs::perms::owner_write |
                                               fs::perms::group_exec | fs::perms::group_read |
                                               fs::perms::others_exec | fs::perms::others_read,
                                           fs::perm_options::add))
            {
                bra_log_warn("unable to set executable bit on %s", sfx_path.string().c_str());
            }
            // #endif

            // remove TMP SFX FILE
            // TODO: better starting with the SFX file then append the file
            //       it will save disk space as in this way requires twice the archive size
            //       to do an SFX

            if (!bra::fs::file_remove(m_out_filename))
                bra_log_warn("unable to delete SFX_TMP_FILE");

            bra_log_printf("OK\n");
        }

        return 0;
    }

public:
    Bra() = default;

    virtual ~Bra()
    {
        bra_io_close(&m_f2);
    }
};

/////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
    Bra bra_prog;

    return bra_prog.run(argc, argv);
}
