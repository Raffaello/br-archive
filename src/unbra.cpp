#include <lib_bra.h>
#include <bra_log.h>
#include <bra_fs.hpp>
#include <version.h>

#include <BraProgram.hpp>

#include <format>
#include <iostream>
#include <filesystem>
#include <string>
#include <list>
#include <algorithm>
#include <limits>

#include <cstdint>
#include <cstdio>


using namespace std;


namespace fs = std::filesystem;

// TODO: unbra must be able to read sfx too
// TODO: add output path as parameter

//////////////////////////////////////////////////////////////////////////


// std::string format_bytes(const size_t bytes)
// {
//     constexpr size_t KB = 1024;
//     constexpr size_t MB = KB * 1024;
//     constexpr size_t GB = MB * 1024;

// if (bytes >= GB)
//     return format("{:>6.1f} GB", static_cast<double>(bytes) / GB);
// else if (bytes >= MB)
//     return format("{:>6.1f} MB", static_cast<double>(bytes) / MB);
// else if (bytes >= KB)
//     return format("{:>6.1f} KB", static_cast<double>(bytes) / KB);
// else
//     return format("{:>6}  B", bytes);
// }

class Unbra : public BraProgram
{
private:
    fs::path g_bra_file;
    bool     g_listContent = false;

protected:
    virtual void help_usage()
    {
        bra_log_printf("  unbra (input_file)%s\n", BRA_FILE_EXT);
    };

    virtual void help_example()
    {
        bra_log_printf("  unbra test.BRa\n");
        bra_log_printf("\n");
        bra_log_printf("(input_file) : %s archive to extract.\n", BRA_NAME);
    };

    virtual void help_options()
    {
        bra_log_printf("--list   | -l : view archive content.\n");
    };

    std::optional<bool> parseArgs_option([[maybe_unused]] const int argc, [[maybe_unused]] const char* const argv[], [[maybe_unused]] int& i, const std::string_view& s) override
    {
        if (s == "--list" || s == "-l")
        {
            // list content
            g_listContent = true;
        }
        else
            return nullopt;

        return true;
    }

    void parseArgs_adjustFilename(std::filesystem::path& p) override
    {
        p = bra::fs::filename_archive_adjust(p);
    }

    bool parseArgs_file(const std::filesystem::path& p)
    {
        g_bra_file = p;
        return true;
    }

    bool parseArgs_dir([[maybe_unused]] const std::filesystem::path& p)
    {
        // TODO not implemented yet
        // it should create the dir and extract in that dir
        return false;
    }

    bool parseArgs_wildcard([[maybe_unused]] const std::filesystem::path& p)
    {
        // Not supported.
        // TODO: or for filtering what to extract from the archive?
        return false;
    }

    bool validateArgs()
    {
        if (g_bra_file.empty())
        {
            bra_log_error("no input file provided");
            return false;
        }

        return true;
    }

    int run_prog()
    {
        // forcing to work only on BRA_FILE_EXT
        if (g_bra_file.extension() != BRA_FILE_EXT)
        {
            bra_log_critical("unexpected %s", g_bra_file.string().c_str());
            return 99;
        }

        // header
        bra_io_header_t bh{};
        bra_io_file_t   f{};
        if (!bra_io_open(&f, g_bra_file.string().c_str(), "rb"))
            return 1;

        if (!bra_io_read_header(&f, &bh))
            return 1;

        bra_log_printf("%s contains num files: %u\n", BRA_NAME, bh.num_files);
        if (g_listContent)
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
};

//////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
    Unbra unbra;

    return unbra.run(argc, argv);
}
