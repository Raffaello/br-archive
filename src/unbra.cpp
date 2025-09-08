#include <lib_bra.h>
#include <bra_log.h>
#include <bra_fs.hpp>
#include <version.h>

#include <BraProgram.hpp>

#include <filesystem>
#include <string>
#include <list>
#include <algorithm>

#include <cstdint>


using namespace std;


namespace fs = std::filesystem;

// TODO: unbra must be able to read sfx too
// TODO: add output path as parameter

//////////////////////////////////////////////////////////////////////////

class Unbra : public BraProgram
{
private:
    Unbra(const Unbra&)            = delete;
    Unbra& operator=(const Unbra&) = delete;
    Unbra(Unbra&&)                 = delete;
    Unbra& operator=(Unbra&&)      = delete;

    fs::path m_bra_file;
    bool     m_listContent = false;

protected:
    virtual void help_usage() const override
    {
        bra_log_printf("  %s <input_file>%s\n", fs::path(m_argv0).filename().string().c_str(), BRA_FILE_EXT);
    };

    virtual void help_example() const override
    {
        bra_log_printf("  unbra test.BRa\n");
        bra_log_printf("\n");
        bra_log_printf("<input_file%s> : %s archive to extract.\n", BRA_FILE_EXT, BRA_NAME);
    };

    virtual void help_options() const override
    {
        bra_log_printf("--list       | -l : view archive content.\n");
    };

    int parseArgs_minArgc() const override { return 2; }

    std::optional<bool> parseArgs_option([[maybe_unused]] const int argc, [[maybe_unused]] const char* const argv[], [[maybe_unused]] int& i, const std::string& s) override
    {
        if (s == "--list" || s == "-l")
        {
            // list content
            m_listContent = true;
        }
        else
            return nullopt;

        return true;
    }

    void parseArgs_adjustFilename(std::filesystem::path& p) override
    {
        p = bra::fs::filename_archive_adjust(p);
    }

    bool parseArgs_file(const std::filesystem::path& p) override
    {
        m_bra_file = p;
        return true;
    }

    bool parseArgs_dir([[maybe_unused]] const std::filesystem::path& p) override
    {
        // TODO not implemented yet
        // it should create the dir and extract in that dir
        return false;
    }

    bool parseArgs_wildcards([[maybe_unused]] const std::filesystem::path& p) override
    {
        return false;
    }

    bool validateArgs() override
    {
        if (m_bra_file.empty())
        {
            bra_log_error("no input file provided");
            return false;
        }

        return true;
    }

    int run_prog() override
    {
        // forcing to work only on BRA_FILE_EXT
        if (m_bra_file.extension() != BRA_FILE_EXT)
        {
            bra_log_critical("unexpected %s", m_bra_file.string().c_str());
            return 99;
        }

        // header
        bra_io_header_t bh{};
        if (!bra_io_open(&m_f, m_bra_file.string().c_str(), "rb"))
            return 1;

        if (!bra_io_read_header(&m_f, &bh))
            return 1;

        bra_log_printf("%s contains num files: %u\n", BRA_NAME, bh.num_files);
        if (m_listContent)
        {
            bra_log_printf("| ATTR |   SIZE    | " BRA_PRINTF_FMT_FILENAME "|\n", "FILENAME");
            bra_log_printf("|------|-----------|-");
            for (int i = 0; i < BRA_PRINTF_FMT_FILENAME_MAX_LENGTH; i++)
                bra_log_printf("-");
            bra_log_printf("|\n");
            for (uint32_t i = 0; i < bh.num_files; i++)
            {
                if (!bra_io_print_meta_file(&m_f))
                    return 2;
            }
        }
        else
        {
            for (uint32_t i = 0; i < bh.num_files; i++)
            {
                if (!bra_io_decode_and_write_to_disk(&m_f, &m_overwrite_policy))
                    return 1;
            }
        }

        bra_io_close(&m_f);
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
