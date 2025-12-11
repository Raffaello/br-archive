#include <lib_bra.h>
#include <io/lib_bra_io_file.h>
#include <io/lib_bra_io_file_ctx.h>

#include <log/bra_log.h>
#include <version.h>
#include <BraProgram.hpp>

#include <filesystem>
#include <string>

#include <cstdio>
#include <cstdint>

/// \cond DO_NOT_DOCUMENT

using namespace std;

namespace fs = std::filesystem;

/////////////////////////////////////////////////////////////////////////

/**
 * @brief BraSfx program
 */
class BraSfx : public BraProgram
{
private:
    BraSfx(const BraSfx&)            = delete;
    BraSfx& operator=(const BraSfx&) = delete;
    BraSfx(BraSfx&&)                 = delete;
    BraSfx& operator=(BraSfx&&)      = delete;

    fs::path m_output_path;

protected:
    virtual void help_usage() const override
    {
        const fs::path p(m_argv0);

        bra_log_printf("  %s       : self-extract the embedded archive.\n", p.filename().string().c_str());
    };

    virtual void help_example() const override
    {
        bra_log_printf("  %s\n", fs::path(m_argv0).filename().string().c_str());
    };

    // same as unbra
    virtual void help_options() const override
    {
        bra_log_printf("--output | -o : output path must be a directory relative to the current one (default: current directory).\n");
    };

    int parseArgs_minArgc() const override { return 1; }

    // same as unbra
    std::optional<bool> parseArgs_option([[maybe_unused]] const int argc, [[maybe_unused]] const char* const argv[], [[maybe_unused]] int& i, [[maybe_unused]] const std::string& s) override
    {
        if (s == "--output" || s == "-o")
        {
            if (i + 1 >= argc)
            {
                bra_log_error("missing argument for --output");
                return false;
            }

            m_output_path = fs::path(argv[++i]);
        }
        else
        {
            return nullopt;
        }

        return true;
    }

    bool parseArgs_file([[maybe_unused]] const std::filesystem::path& p) override
    {
        return false;
    }

    bool validateArgs() override
    {
        if (!bra_io_file_can_be_sfx(m_argv0.c_str()))
        {
            bra_log_error("unsupported file detected: %s", m_argv0.c_str());
            return false;
        }

        if (m_output_path.empty())
        {
            std::error_code ec;
            m_output_path = fs::current_path(ec);
            if (ec)
            {
                bra_log_error("unable to get current directory");
                return false;
            }

            if (!bra::fs::try_sanitize(m_output_path))
            {
                bra_log_error("invalid current path: '%s'", m_output_path.string().c_str());
                return false;
            }
        }
        else
        {
            if (!bra::fs::try_sanitize(m_output_path) || m_output_path.empty())
            {
                bra_log_error("invalid output path: '%s'", m_output_path.string().c_str());
                return false;
            }

            if (bra::fs::dir_exists(m_output_path))
            {
                bra_log_warn("output path %s already exists.", m_output_path.string().c_str());
            }
        }

#ifndef NDEBUG
        bra_log_debug("output path: %s", m_output_path.string().c_str());
#endif

        return true;
    }

    int run_prog() override
    {
        bra_io_header_t bh{};

        // this is the only difference from unbra (read the footer)
        // and do not force extension checking to unbra
        // plus file detection, as it would be required to open the SFX
        // just for the supported exe/elf formats avoid for all the rest.
        if (!bra_io_file_ctx_sfx_open_and_read_footer_header(m_argv0.c_str(), &bh, &m_ctx))
            return 1;

        // extract payload, encoded data
        bra_log_printf("%s contains num files: %u\n", BRA_NAME, bh.num_files);
        std::error_code ec;
        if (!bra::fs::dir_exists(m_output_path))
        {
            bra_log_printf("Creating output path: %s\n", m_output_path.string().c_str());
            if (!bra::fs::dir_make(m_output_path))
                return 1;
        }
        const fs::path cur_path = fs::current_path(ec);
        if (ec)
        {
            bra_log_error("unable to get current directory");
            return 1;
        }

        fs::current_path(m_output_path, ec);
        if (ec)
        {
            bra_log_error("unable to change current directory to: %s", m_output_path.string().c_str());
            return 1;
        }

        for (uint32_t i = 0; i < bh.num_files; i++)
        {
            if (!bra_io_file_ctx_decode_and_write_to_disk(&m_ctx, &m_overwrite_policy))
                return 1;
        }

        fs::current_path(cur_path, ec);
        if (ec)
            bra_log_warn("unable to change current directory to: %s", cur_path.string().c_str());

        if (!bra_io_file_ctx_close(&m_ctx))
            return 1;

        return 0;
    }

public:
    BraSfx()          = default;
    virtual ~BraSfx() = default;
};

////////////////////////////////////////////////////////////////////////


int main(int argc, char* argv[])
{
    // The idea of the SFX is to have a footer at the end of the file
    // The footer contain the location where the embedded data is
    // so it can be extracted / dumped into a temporary file
    // and decoded

    // TODO: when extracting should check to do not auto-overwrite itself.

    // BraSfx bra_sfx(argc, argv);
    BraSfx bra_sfx;
    return bra_sfx.run(argc, argv);
}

/// \endcond DO_NOT_DOCUMENT
