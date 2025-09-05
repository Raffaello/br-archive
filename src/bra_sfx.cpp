#include <lib_bra.h>
#include <bra_log.h>
#include <version.h>
#include <BraProgram.hpp>

#include <iostream>
#include <filesystem>


using namespace std;

namespace fs = std::filesystem;

bool bra_isElf(const char* fn)
{
    bra_io_file_t f;
    if (!bra_io_open(&f, fn, "rb"))
        return false;

    // 0x7F,'E','L','F'
    constexpr int MAGIC_SIZE = 4;
    char          magic[MAGIC_SIZE];
    if (fread(magic, sizeof(char), MAGIC_SIZE, f.f) != MAGIC_SIZE)
    {
        bra_io_file_read_error(&f);
        return false;
    }

    bra_io_close(&f);
    return magic[0] == 0x7F && magic[1] == 'E' && magic[2] == 'L' && magic[3] == 'F';
}

bool bra_isPE(const char* fn)
{
    bra_io_file_t f;
    if (!bra_io_open(&f, fn, "rb"))
        return false;

    // 'M' 'Z'
    constexpr int MAGIC_SIZE = 2;
    char          magic[MAGIC_SIZE];
    if (fread(magic, sizeof(char), MAGIC_SIZE, f.f) != MAGIC_SIZE)
    {
    BRA_IS_EXE_ERROR:
        bra_io_file_read_error(&f);
        return false;
    }

    if (magic[0] != 'M' || magic[1] != 'Z')
    {
        bra_io_close(&f);
        return false;
    }

    // Read the PE header offset from the DOS header
    if (!bra_io_seek(&f, 0x3C, SEEK_SET))
    {
    BRA_IS_EXE_SEEK_ERROR:
        bra_io_file_seek_error(&f);
        return false;
    }

    uint32_t pe_offset;
    if (fread(&pe_offset, sizeof(uint32_t), 1, f.f) != 1)
        goto BRA_IS_EXE_ERROR;

    if (!bra_io_seek(&f, pe_offset, SEEK_SET))
        goto BRA_IS_EXE_SEEK_ERROR;

    constexpr int PE_MAGIC_SIZE = 4;
    char          pe_magic[PE_MAGIC_SIZE];
    if (fread(pe_magic, sizeof(char), PE_MAGIC_SIZE, f.f) != PE_MAGIC_SIZE)
        goto BRA_IS_EXE_ERROR;

    bra_io_close(&f);
    return pe_magic[0] == 'P' && pe_magic[1] == 'E' && pe_magic[2] == '\0' && pe_magic[3] == '\0';
}

bool bra_file_open_and_read_footer_header(const char* fn, bra_io_header_t* out_bh, bra_io_file_t* f)
{
    if (!bra_io_open(f, fn, "rb"))
        return false;

    if (!bra_io_seek(f, -1L * static_cast<int64_t>(sizeof(bra_io_footer_t)), SEEK_END))
    {
    BRA_FILE_SEEK_ERROR:
        bra_io_file_seek_error(f);
        return false;
    }

    bra_io_footer_t bf{};
    if (!bra_io_read_footer(f, &bf))
        return false;

    // read header and check
    if (!bra_io_seek(f, bf.header_offset, SEEK_SET))
        goto BRA_FILE_SEEK_ERROR;

    if (!bra_io_read_header(f, out_bh))
        return false;

    return true;
}

/////////////////////////////////////////////////////////////////////////

class BraSfx : public BraProgram
{
private:
    bool m_listContent = false;

protected:
    virtual void help_usage()
    {
        fs::path p(m_argv0);

        bra_log_printf("  %s [-l]     : to start un-archiving or listing.\n", p.filename().string().c_str());
    };

    virtual void help_example() {
    };

    // same as unbra
    virtual void help_options()
    {
        bra_log_printf("--list   | -l : view archive content.\n");
    };

    int parseArgs_minArgc() const override { return 1; }

    // same as unbra
    std::optional<bool> parseArgs_option([[maybe_unused]] const int argc, [[maybe_unused]] const char* const argv[], [[maybe_unused]] int& i, const std::string_view& s) override
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

    void parseArgs_adjustFilename([[maybe_unused]] std::filesystem::path& p) override
    {
    }

    bool parseArgs_file([[maybe_unused]] const std::filesystem::path& p)
    {
        return false;
    }

    // same as unbra
    bool parseArgs_dir([[maybe_unused]] const std::filesystem::path& p)
    {
        // TODO not implemented yet
        // it should create the dir and extract in that dir
        return false;
    }

    // same as unbra
    bool parseArgs_wildcard([[maybe_unused]] const std::filesystem::path& p)
    {
        // Not supported.
        // TODO: or for filtering what to extract from the archive?
        return false;
    }

    bool validateArgs()
    {
        // Supporting only EXE and ELF file type for now
        if (bra_isElf(m_argv0))
        {
            bra_log_info("ELF file detected");
        }
        else if (bra_isPE(m_argv0))
        {
            bra_log_info("PE file detected");
        }
        else
        {
            bra_log_error("unsupported file detected: %s", m_argv0);
            return false;
        }

        return true;
    }

    int run_prog()
    {
        bra_io_header_t bh;
        bra_io_file_t   f{};

        // this is the only difference from unbra (read the  footer)
        // and do not force extension checking to unbra
        if (!bra_file_open_and_read_footer_header(m_argv0, &bh, &f))
            return 1;

        // extract payload, encoded data
        // same as un bra
        bra_log_printf("%s contains num files: %u\n", BRA_NAME, bh.num_files);
        if (m_listContent)
        {
            bra_log_printf("| ATTR |   SIZE    | FILENAME                                |\n");
            bra_log_printf("|------|-----------|-----------------------------------------|\n");
            for (uint32_t i = 0; i < bh.num_files; i++)
            {
                if (!bra_print_meta_file(&f))
                    return 2;
            }
        }
        else
        {
            for (uint32_t i = 0; i < bh.num_files; i++)
            {
                if (!bra_io_decode_and_write_to_disk(&f, &m_overwrite_policy))
                    return 1;
            }
        }

        bra_io_close(&f);
        return 0;
    }

public:
    const char* m_argv0;

    explicit BraSfx([[maybe_unused]] const int argc, const char* const argv[]) : m_argv0(argv[0]) {}
};

////////////////////////////////////////////////////////////////////////


int main(int argc, char* argv[])
{
    // TODO: add output directory where to decode
    // TODO: ask to overwrite files, etc..
    // TODO: all these functionalities are common among the utilities

    // The idea of the SFX is to have a footer at the end of the file
    // The footer contain the location where the embedded data is
    // so it can be extracted / dumped into a temporary file
    // and decoded

    // TODO: when extracting should check to do not auto-overwrite itself.

    BraSfx bra_sfx(argc, argv);
    return bra_sfx.run(argc, argv);
}
