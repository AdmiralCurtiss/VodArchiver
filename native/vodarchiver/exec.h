#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace VodArchiver {
int RunProgram(const std::string& programName,
               const std::vector<std::string>& args,
               const std::function<void(std::string_view)>& stdOutRedirect,
               const std::function<void(std::string_view)>& stdErrRedirect);
} // namespace VodArchiver
