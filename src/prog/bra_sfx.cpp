#include <lib_bra.h>
#include <io/lib_bra_io_file.h>

#include <log/bra_log.h>
#include <version.h>
#include <BraProgram.hpp>

#include <filesystem>
#include <string>

#include <cstdio>
#include <cstdint>


using namespace std;

namespace fs = std::filesystem;

/////////////////////////////////////////////////////////////////////////

class BraSfx : public BraProgram
{
private:
    BraSfx(const BraSfx&)            = delete;
    BraSfx& operator=(const BraSfx&) = delete;
    BraSfx(BraSfx&&)                 = delete;
    BraSfx& operator=(BraSfx&&)      = delete;

protected:
    virtual void help_usage() const override
    {
        const fs::path p(m_argv0);

        bra_log_printf("  %s       : self-extract thes embedded archive.\n", p.filename().string().c_str());
    };

    virtual void help_example() const override
    {
        bra_log_printf("  %s\n", fs::path(m_argv0).filename().string().c_str());
    };

    // same as unbra
    virtual void help_options() const override {};

    int parseArgs_minArgc() const override { return 1; }

    // same as unbra
    std::optional<bool> parseArgs_option([[maybe_unused]] const int argc, [[maybe_unused]] const char* const argv[], [[maybe_unused]] int& i, [[maybe_unused]] const std::string& s) override
    {
        return nullopt;
    }

    bool parseArgs_file([[maybe_unused]] const std::filesystem::path& p) override
    {
        return false;
    }

    bool validateArgs() override
    {
        if (!bra_io_file_is_sfx(m_argv0.c_str()))
        {
            bra_log_error("unsupported file detected: %s", m_argv0.c_str());
            return false;
        }

        return true;
    }

    int run_prog() override
    {
        bra_io_header_t bh{};

        // this is the only difference from unbra (read the footer)
        // and do not force extension checking to unbra
        // plus file detection, as it would be required to open the SFX
        // just for the supported exe/elf formats avoid for all the rest.
        if (!bra_io_file_sfx_open_and_read_footer_header(m_argv0.c_str(), &bh, &m_f))
            return 1;

        // extract payload, encoded data
        bra_log_printf("%s contains num files: %u\n", BRA_NAME, bh.num_files);
        for (uint32_t i = 0; i < bh.num_files; i++)
        {
            if (!bra_io_file_decode_and_write_to_disk(&m_f, &m_overwrite_policy))
                return 1;
        }

        bra_io_file_close(&m_f);
        return 0;
    }

public:
    BraSfx()          = default;
    virtual ~BraSfx() = default;
};

////////////////////////////////////////////////////////////////////////


int main(int argc, char* argv[])
{
    // TODO: add output directory where to decode

    // The idea of the SFX is to have a footer at the end of the file
    // The footer contain the location where the embedded data is
    // so it can be extracted / dumped into a temporary file
    // and decoded

    // TODO: when extracting should check to do not auto-overwrite itself.

    // BraSfx bra_sfx(argc, argv);
    BraSfx bra_sfx;
    return bra_sfx.run(argc, argv);
}
