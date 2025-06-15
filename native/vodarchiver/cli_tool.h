#pragma once

namespace VodArchiver {
using CliToolFunctionT = int (*)(int argc, char** argv);
struct CliTool {
    const char* Name;
    const char* ShortDescription;
    CliToolFunctionT Function;
    bool Hidden = false;
};
} // namespace VodArchiver
