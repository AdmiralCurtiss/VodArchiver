#include "exec.h"

#include <format>
#include <string>
#include <vector>

#include "util/scope.h"
#include "util/text.h"

#ifdef BUILD_FOR_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

namespace VodArchiver {
#ifdef BUILD_FOR_WINDOWS
std::string BuildArgsString(const std::string& programName, const std::vector<std::string>& args) {
    std::string s = std::format("\"{}\"", programName);
    for (const std::string& arg : args) {
        s.push_back(' ');
        s.append(std::format("\"{}\"", HyoutaUtils::TextUtils::Replace(arg, "\"", "\\\"")));
    }
    return s;
}

int RunProgram(const std::string& programName, const std::vector<std::string>& args) {
    auto wideProgName =
        HyoutaUtils::TextUtils::Utf8ToWString(programName.data(), programName.size());
    if (!wideProgName.has_value()) {
        return -1;
    }
    std::string argsString = BuildArgsString(programName, args);
    auto wideArgsString =
        HyoutaUtils::TextUtils::Utf8ToWString(argsString.data(), argsString.size());
    if (!wideArgsString.has_value()) {
        return -1;
    }

    STARTUPINFO startupinfo{};
    startupinfo.cb = sizeof(STARTUPINFO);
    startupinfo.dwFlags = STARTF_FORCEOFFFEEDBACK;
    PROCESS_INFORMATION processinfo{};
    if (!CreateProcessW(wideProgName->c_str(),
                        wideArgsString->data(),
                        nullptr,
                        nullptr,
                        FALSE,
                        CREATE_UNICODE_ENVIRONMENT,
                        nullptr,
                        nullptr,
                        &startupinfo,
                        &processinfo)) {
        return -1;
    }
    auto closeHandles = HyoutaUtils::MakeScopeGuard([&]() {
        CloseHandle(processinfo.hThread);
        CloseHandle(processinfo.hProcess);
    });
    WaitForSingleObject(processinfo.hProcess, INFINITE);
    DWORD rv = 0;
    if (!GetExitCodeProcess(processinfo.hProcess, &rv)) {
        return -1;
    }
    return static_cast<int>(rv);
}
#else
int RunProgram(const std::string& programName, const std::vector<std::string>& args) {
    return -1;
}
#endif
} // namespace VodArchiver
