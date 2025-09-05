#include "BraProgram.hpp"
#include <bra_log.h>
#include <bra_fs.hpp>
#include <version.h>

#include <string>


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

void BraProgram::banner()
{
    bra_log_printf("BR-Archive Utility Version: %s\n", VERSION);
    bra_log_printf("\n");
}

void BraProgram::help_common_options()
{
    bra_log_printf("--help   | -h : display this page.\n");
    bra_log_printf("--yes    | -y : force a 'yes' response to all the user questions.\n");
    bra_log_printf("--no     | -n : force 'no' to all prompts (skip overwrites).\n");
}

void BraProgram::help()
{
    banner();
    bra_log_printf("Usage:\n");
    help_usage();
    bra_log_printf("Example:\n");
    help_example();
    bra_log_printf("\n");
    bra_log_printf("Options:\n");
    help_common_options();
    help_options();
    bra_log_printf("\n");
    bra_log_flush();
}

std::optional<bool> BraProgram::parseArgs(const int argc, const char* const argv[])
{
    if (argc < 2)
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
            // doing nothing here
            // it is simpler to read in this way,
            // else if parseArgs_option failed going to FS sub-section.
            if (!*o)
                return false;
        }
        else
        {
            // FS sub-section
            fs::path p = s;
            // check if it is file or a dir
            if (bra::fs::file_exists(p))
            {
                // check file path
                if (!bra::fs::try_sanitize(p))
                {
                    bra_log_error("path not valid: %s", p.string().c_str());
                    return false;
                }

                if (!parseArgs_file(p))
                    return false;
            }
            else if (bra::fs::dir_exists(p))
            {
                // This should match exactly the directory.
                // so need to be converted as a wildcard adding a `/*' at the end
                p /= "*";
                if (!parseArgs_dir(p))
                    return false;
            }
            // check if it is a wildcard
            else if (bra::fs::is_wildcard(p))
            {
                if (!parseArgs_wildcard(p))
                    return false;
            }
            else
            {
                bra_log_error("unknown argument/file doesn't exist: %s", s.c_str());
                return false;
            }
        }
    }

    return true;
}

int BraProgram::run(const int argc, const char* const argv[])
{
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
