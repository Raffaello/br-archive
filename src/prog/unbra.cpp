#include <lib_bra.h>
#include <io/lib_bra_io_file.h>
#include <io/lib_bra_io_file_ctx.h>

#include <log/bra_log.h>
#include <fs/bra_fs.hpp>
#include <version.h>

#include <BraProgramOutputArgTrait.hpp>

#include <filesystem>
#include <string>
#include <list>
#include <algorithm>

#include <cstdint>

/// \cond DO_NOT_DOCUMENT

using namespace std;


namespace fs = std::filesystem;

//////////////////////////////////////////////////////////////////////////

/**
 * @brief Unbra program
 */
class Unbra : public BraProgramOutputArgTrait
{
private:
    Unbra(const Unbra&)            = delete;
    Unbra& operator=(const Unbra&) = delete;
    Unbra(Unbra&&)                 = delete;
    Unbra& operator=(Unbra&&)      = delete;

    fs::path m_bra_file;
    bool     m_listContent = false;
    bool     m_testContent = false;
    bool     m_sfx         = false;

protected:
    virtual void help_usage() const override
    {
        bra_log_printf("  %s <input_file>%s[%s|%s]\n", fs::path(m_argv0).filename().string().c_str(), BRA_FILE_EXT, BRA_SFX_FILE_EXT_LIN, BRA_SFX_FILE_EXT_WIN);
    };

    virtual void help_example() const override
    {
        bra_log_printf("  unbra test.BRa\n");
        bra_log_printf("\n");
        bra_log_printf("<input_file>[%s]          : %s archive to extract.\n", BRA_FILE_EXT, BRA_NAME);
        bra_log_printf("<input_file>%s[%s|%s] : %s self-extracting archive to extract.\n", BRA_FILE_EXT, BRA_SFX_FILE_EXT_LIN, BRA_SFX_FILE_EXT_WIN, BRA_NAME);
    };

    virtual void help_options() const override
    {
        bra_log_printf("--list       | -l : view archive content.\n");
        bra_log_printf("--test       | -t : test archive integrity (implies --list).\n");
        BraProgramOutputArgTrait::help_options();
    };

    int parseArgs_minArgc() const override { return 2; }

    std::optional<bool> parseArgs_option(const int argc, const char* const argv[], int& i, const std::string& s) override
    {
        if (s == "--list" || s == "-l")
        {
            // list content
            m_listContent = true;
        }
        else if (s == "--test" || s == "-t")
        {
            // test content
            m_testContent = true;
            m_listContent = true;
        }
        else
        {
            return BraProgramOutputArgTrait::parseArgs_option(argc, argv, i, s);
        }

        return true;
    }

    void parseArgs_adjustFilename(std::filesystem::path& p) override
    {
        // current file
        const auto path = p;

        // file .BRa
        p = bra::fs::filename_archive_adjust(p);
        if (bra::fs::file_exists(p))
            return;

        const fs::path p_  = p;
        p                 += BRA_SFX_FILE_EXT_LIN;
        if (bra::fs::file_exists(p))
            return;

        p  = p_;
        p += BRA_SFX_FILE_EXT_WIN;
        if (bra::fs::file_exists(p))
            return;

        p = path;
    };

    bool parseArgs_file(const std::filesystem::path& p) override
    {
        if (p.extension() == BRA_SFX_FILE_EXT_LIN || p.extension() == BRA_SFX_FILE_EXT_WIN)
            m_sfx = true;

        m_bra_file = p;

        return true;
    }

    bool validateArgs() override
    {
        if (m_bra_file.empty())
        {
            bra_log_error("no input file provided");
            return false;
        }

        return BraProgramOutputArgTrait::validateArgs();
    }

    int run_prog() override
    {
        // header
        bra_io_header_t bh{};

        if (m_sfx)
        {
            if (!bra_io_file_ctx_sfx_open_and_read_footer_header(m_bra_file.string().c_str(), &bh, &m_ctx))
                return 1;
        }
        else
        {
            if (!bra_io_file_ctx_open(&m_ctx, m_bra_file.string().c_str(), "rb"))
                return 1;

            if (!bra_io_file_ctx_read_header(&m_ctx, &bh))
                return 1;
        }

        bra_log_printf("%s contains num files: %u\n", m_ctx.f.fn, bh.num_files);
        if (m_listContent || m_testContent)
        {
            bra_log_printf("| ATTR |   SIZE    | " BRA_PRINTF_FMT_FILENAME "| CRC32  |\n", "FILENAME");
            bra_log_printf("|------|-----------|-");
            for (int i = 0; i < BRA_PRINTF_FMT_FILENAME_MAX_LENGTH; i++)
                bra_log_printf("-");
            bra_log_printf("|--------|\n");
            for (uint32_t i = 0; i < bh.num_files; i++)
            {
                if (!bra_io_file_ctx_print_meta_entry(&m_ctx, m_testContent))
                    return 2;
            }

            if (m_testContent)
                bra_log_printf("* All %u entr%s verified successfully.\n", bh.num_files, bh.num_files == 1 ? "y" : "ies");
        }
        else
        {
            const int ret = BraProgramOutputArgTrait::run_prog();
            if (ret != 0)
                return ret;

            for (uint32_t i = 0; i < bh.num_files; i++)
            {
                if (!bra_io_file_ctx_decode_and_write_to_disk(&m_ctx, &m_overwrite_policy))
                    return 1;
            }

            BraProgramOutputArgTrait::run_prog_end();
        }

        bra_io_file_ctx_close(&m_ctx);

        return 0;
    }

public:
    Unbra()          = default;
    virtual ~Unbra() = default;
};

//////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
    Unbra unbra;

    return unbra.run(argc, argv);
}

/// \endcond DO_NOT_DOCUMENT
