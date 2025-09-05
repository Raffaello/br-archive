#pragma once

#include <lib_bra_defs.h>
#include <lib_bra_types.h>
#include <lib_bra.h>
#include <bra_fs.hpp>

#include <string>
#include <string_view>
#include <filesystem>
#include <optional>

/**
 * @brief Abstract Class for generating a program.
 *
 * @details Contains the help, parseArgs, validateArgs and run elements to execute a CLI command
 *          specific to BR-archive
 */
class BraProgram
{
private:
protected:
    bra_io_file_t             m_f{};
    bra_fs_overwrite_policy_e m_overwrite_policy = BRA_OVERWRITE_ASK;

    bool set_overwrite_policy(const bra_fs_overwrite_policy_e op, const std::string& s);

    void banner();

    void help();

    void         help_common_options();
    virtual void help_usage()   = 0;
    virtual void help_example() = 0;
    virtual void help_options() = 0;

    std::optional<bool> parseArgs(const int argc, const char* const argv[]);

    virtual int                 parseArgs_minArgc() const                                                                     = 0;
    virtual std::optional<bool> parseArgs_option(const int argc, const char* const argv[], int& i, const std::string_view& s) = 0;
    virtual void                parseArgs_adjustFilename(std::filesystem::path& p)                                            = 0;
    virtual bool                parseArgs_file(const std::filesystem::path& p)                                                = 0;
    virtual bool                parseArgs_dir(const std::filesystem::path& p)                                                 = 0;
    virtual bool                parseArgs_wildcard(const std::filesystem::path& p)                                            = 0;

    virtual bool validateArgs() = 0;

    virtual int run_prog() = 0;

public:
    virtual ~BraProgram();

    int run(const int argc, const char* const argv[]);
};
