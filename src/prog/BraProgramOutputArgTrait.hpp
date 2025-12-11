#pragma once

#include <prog/BraProgram.hpp>

#include <filesystem>
#include <optional>
#include <string>

/// \cond DO_NOT_DOCUMENT

class BraProgramOutputArgTrait : public BraProgram
{
private:
    std::filesystem::path m_cur_path;

protected:
    std::filesystem::path m_output_path;

    void                help_options() const override;
    std::optional<bool> parseArgs_option(const int argc, const char* const argv[], int& i, const std::string& s) override;
    bool                validateArgs() override;
    int                 run_prog() override;

    void run_prog_end();

public:
};

/// \endcond DO_NOT_DOCUMENT