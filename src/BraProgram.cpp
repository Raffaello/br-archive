#include "BraProgram.hpp"
#include <bra_log.h>
#include <bra_fs.hpp>
#include <version.h>

#include <string>

#if defined(_WIN32)
#if defined(__MINGW32__) || defined(__MINGW64__) || defined(__MSYS__)
// Enable wildcard expansion on Windows MSYS2
extern "C" {
int _dowildcard = -1;
}
#endif
#endif


using namespace std;

namespace fs = std::filesystem;

BraProgram::~BraProgram()
{
    bra_io_close(&m_f);
}

bool BraProgram::set_overwrite_policy(const bra_fs_overwrite_policy_e op, const string& s)
{
    if (m_overwrite_policy != BRA_OVERWRITE_ASK)
    {
        bra_log_error("can't set %s: another mutually exclusive option is already set.", s.c_str());
        return false;
    }

    m_overwrite_policy = op;
    return true;
}

void BraProgram::banner() const
{
    bra_log_printf("BR-Archive Utility %s Version: %s\n", fs::path(m_argv0).filename().string().c_str(), VERSION);
    bra_log_printf("\n");
}

void BraProgram::help() const
{
    banner();
    bra_log_printf("Usage:\n");
    help_usage();
    bra_log_printf("Examples:\n");
    help_example();
    bra_log_printf("\n");
    bra_log_printf("Options:\n");
    help_common_options();
    help_options();
    bra_log_printf("\n");
    bra_log_flush();
}

void BraProgram::help_common_options() const
{
    bra_log_printf("--help   | -h : display this page.\n");
    bra_log_printf("--yes    | -y : force a 'yes' response to all the user questions.\n");
    bra_log_printf("--no     | -n : force 'no' to all prompts (skip overwrites).\n");
}

std::optional<bool> BraProgram::parseArgs(const int argc, const char* const argv[])
{
    if (argc < parseArgs_minArgc())
    {
        help();
        return false;
    }

    m_overwrite_policy = BRA_OVERWRITE_ASK;
    for (int i = 1; i < argc; ++i)
    {
        // first part is about arguments sections
        string s = argv[i];

        if (s == "--help" || s == "-h")
        {
            help();
            return nullopt;
        }
        else if (s == "--yes" || s == "-y")
        {
            if (!set_overwrite_policy(BRA_OVERWRITE_ALWAYS_YES, s))
                return false;
        }
        else if (s == "--no" || s == "-n")
        {
            if (!set_overwrite_policy(BRA_OVERWRITE_ALWAYS_NO, s))
                return false;
        }
        else if (auto o = parseArgs_option(argc, argv, i, s); o)
        {
            // if returned false means error.
            if (!*o)
                return false;
            // otherwise ok and do nothing

            // if returned nullopt, there was nothing to parse for 's'
        }
        // unknown option
        else if (!s.empty() && s[0] == '-')
        {
            bra_log_error("unknown option: %s", s.c_str());
            return false;
        }
        else
        {
            // FS sub-section
            fs::path p = s;
            parseArgs_adjustFilename(p);

            // check file path
            if (!bra::fs::try_sanitize(p))
            {
                bra_log_error("path not valid: %s", p.string().c_str());
                return false;
            }

            // check if it is file or a dir
            if (bra::fs::file_exists(p))
            {
                if (!parseArgs_file(p))
                    return false;
            }
            else if (bra::fs::dir_exists(p))
            {
                if (!parseArgs_dir(p))
                    return false;
            }
            else
            {
                bra_log_error("path doesn't exist: %s", s.c_str());
                return false;
            }
        }
    }

    return true;
}

int BraProgram::run(const int argc, const char* const argv[])
{
    m_argv0 = argv[0];
    if (auto pa = parseArgs(argc, argv))
    {
        if (!*pa)
            return 1;
    }
    else
        return 0;

    if (!validateArgs())
        return 1;

    return run_prog();
}
