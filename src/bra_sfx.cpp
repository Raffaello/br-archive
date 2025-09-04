#include <lib_bra.h>
#include <bra_log.h>
#include <version.h>

#include <iostream>
#include <filesystem>
#include <format>

#include <cstdio>


using namespace std;

namespace fs = std::filesystem;

// fs::path g_bra_file;
static bool g_listContent = false;
// static bool g_always_yes  = false;
static bra_fs_overwrite_policy_e g_overwrite_policy = BRA_OVERWRITE_ASK;

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

void help()
{
    cout << format("BR-Archive Utility Version: {}", VERSION) << endl;
    cout << endl;
    cout << endl;
    cout << format("Options:") << endl;
    cout << format("--help   | -h : display this page.") << endl;
    cout << format("--list   | -l : view archive content.") << endl;
    cout << format("--yes    | -y : force a 'yes' response to all the user questions.") << endl;
    cout << format("--no     | -n : force 'no' to all prompts (skip overwrites).") << endl;
    cout << format("--update | -u : update an existing archive with missing files from input.") << endl;
    cout << endl;
}

bool parse_args(int argc, char* argv[])
{
    // Supporting only EXE and ELF file type for now
    if (bra_isElf(argv[0]))
    {
        bra_log_info("ELF file detected");
    }
    else if (bra_isPE(argv[0]))
    {
        bra_log_info("PE file detected");
    }
    else
    {
        bra_log_error("unsupported file detected: %s", argv[0]);
        return false;
    }

    g_listContent      = false;
    g_overwrite_policy = BRA_OVERWRITE_ASK;
    for (int i = 1; i < argc; i++)
    {
        string s = argv[i];
        if ((s == "--help") || (s == "-h"))
        {
            help();
            exit(0);    // this is required to terminate the program with a return 0, if returning false, will return != 0
        }
        else if (s == "--list" || s == "-l")
        {
            // list content
            g_listContent = true;
        }
        else if (s == "--yes" || s == "-y")
        {
            if (g_overwrite_policy != BRA_OVERWRITE_ASK)
            {
                bra_log_error("can't set %s option, another mutual exclusive option already used.", s.c_str());
                return false;
            }

            g_overwrite_policy = BRA_OVERWRITE_ALWAYS_YES;
        }
        else if (s == "--no" || s == "-n")
        {
            if (g_overwrite_policy != BRA_OVERWRITE_ASK)
            {
                bra_log_error("can't set %s option, another mutual exclusive option already used.", s.c_str());
                return false;
            }

            g_overwrite_policy = BRA_OVERWRITE_ALWAYS_NO;
        }
        else
        {
            bra_log_error("unknown argument: %s", s.c_str());
            return false;
        }
    }

    return true;
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

int main(int argc, char* argv[])
{
    // TODO: add output directory where to decode
    // TODO: ask to overwrite files, etc..
    // TODO: all these functionalities are common among the utilities

    if (!parse_args(argc, argv))
        return 1;

    // The idea of the SFX is to have a footer at the end of the file
    // The footer contain the location where the embedded data is
    // so it can be extracted / dumped into a temporary file
    // and decoded

    // TODO: when extracting should check to do not auto-overwrite itself.

    bra_io_header_t bh;
    bra_io_file_t   f{};
    if (!bra_file_open_and_read_footer_header(argv[0], &bh, &f))
        return 1;

    // extract payload, encoded data
    for (uint32_t i = 0; i < bh.num_files; ++i)
    {
        if (g_listContent)
        {
            bra_log_critical("Not implemented yet.");
            // if (!unbra_list_meta_file(f))
            return 2;
        }
        else
        {
            if (!bra_io_decode_and_write_to_disk(&f, &g_overwrite_policy))
                return 1;
        }
    }

    bra_io_close(&f);
    return 0;
}
