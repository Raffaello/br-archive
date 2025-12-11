#include "BraProgramOutputArgTrait.hpp"

#include <log/bra_log.h>
#include <fs/bra_fs.hpp>

namespace fs = std::filesystem;

/// \cond DO_NOT_DOCUMENT

void BraProgramOutputArgTrait::help_options() const
{
    bra_log_printf("--output | -o : output path must be a directory relative to the current one (default: current directory).\n");
}

std::optional<bool> BraProgramOutputArgTrait::parseArgs_option(const int argc, const char* const argv[], int& i, const std::string& s)
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
        return std::nullopt;
    }

    return true;
}

bool BraProgramOutputArgTrait::validateArgs()
{
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

int BraProgramOutputArgTrait::run_prog()
{
    std::error_code ec;
    if (!bra::fs::dir_exists(m_output_path))
    {
        bra_log_printf("Creating output path: %s\n", m_output_path.string().c_str());
        if (!bra::fs::dir_make(m_output_path))
            return 1;
    }
    m_cur_path = fs::current_path(ec);
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

    return 0;
}

void BraProgramOutputArgTrait::run_prog_end()
{
    std::error_code ec;
    fs::current_path(m_cur_path, ec);
    if (ec)
        bra_log_warn("unable to change current directory to: %s", m_cur_path.string().c_str());
}

/// \endcond DO_NOT_DOCUMENT