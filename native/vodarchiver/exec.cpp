#include "exec.h"

#include <format>
#include <string>
#include <thread>
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

// from https://devblogs.microsoft.com/oldnewthing/20111216-00/?p=8873
BOOL CreateProcessWithExplicitHandles(__in_opt LPCTSTR lpApplicationName,
                                      __inout_opt LPTSTR lpCommandLine,
                                      __in_opt LPSECURITY_ATTRIBUTES lpProcessAttributes,
                                      __in_opt LPSECURITY_ATTRIBUTES lpThreadAttributes,
                                      __in BOOL bInheritHandles,
                                      __in DWORD dwCreationFlags,
                                      __in_opt LPVOID lpEnvironment,
                                      __in_opt LPCTSTR lpCurrentDirectory,
                                      __in LPSTARTUPINFO lpStartupInfo,
                                      __out LPPROCESS_INFORMATION lpProcessInformation,
                                      // here is the new stuff
                                      __in DWORD cHandlesToInherit,
                                      __in_ecount(cHandlesToInherit) HANDLE* rgHandlesToInherit) {
    BOOL fSuccess;
    BOOL fInitialized = FALSE;
    SIZE_T size = 0;
    LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList = NULL;
    fSuccess = cHandlesToInherit < 0xFFFFFFFF / sizeof(HANDLE)
               && lpStartupInfo->cb == sizeof(*lpStartupInfo);
    if (!fSuccess) {
        SetLastError(ERROR_INVALID_PARAMETER);
    }
    if (fSuccess) {
        fSuccess = InitializeProcThreadAttributeList(NULL, 1, 0, &size)
                   || GetLastError() == ERROR_INSUFFICIENT_BUFFER;
    }
    if (fSuccess) {
        lpAttributeList =
            reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(HeapAlloc(GetProcessHeap(), 0, size));
        fSuccess = lpAttributeList != NULL;
    }
    if (fSuccess) {
        fSuccess = InitializeProcThreadAttributeList(lpAttributeList, 1, 0, &size);
    }
    if (fSuccess) {
        fInitialized = TRUE;
        fSuccess = UpdateProcThreadAttribute(lpAttributeList,
                                             0,
                                             PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
                                             rgHandlesToInherit,
                                             cHandlesToInherit * sizeof(HANDLE),
                                             NULL,
                                             NULL);
    }
    if (fSuccess) {
        STARTUPINFOEX info;
        ZeroMemory(&info, sizeof(info));
        info.StartupInfo = *lpStartupInfo;
        info.StartupInfo.cb = sizeof(info);
        info.lpAttributeList = lpAttributeList;
        fSuccess = CreateProcess(lpApplicationName,
                                 lpCommandLine,
                                 lpProcessAttributes,
                                 lpThreadAttributes,
                                 bInheritHandles,
                                 dwCreationFlags | EXTENDED_STARTUPINFO_PRESENT,
                                 lpEnvironment,
                                 lpCurrentDirectory,
                                 &info.StartupInfo,
                                 lpProcessInformation);
    }
    if (fInitialized)
        DeleteProcThreadAttributeList(lpAttributeList);
    if (lpAttributeList)
        HeapFree(GetProcessHeap(), 0, lpAttributeList);
    return fSuccess;
}

void RedirectOutput(HANDLE handle, const std::function<void(std::string_view)>& target) {
    // TODO: This seems to work fine, though I'm not actually sure how. I would have expected a
    // text encoding to appear *somewhere* -- or in other words, how do I know what encoding my
    // child process is writing as?

    static constexpr DWORD bufferSize = 4096;
    std::array<char, bufferSize> buffer;
    while (true) {
        DWORD bytesRead = 0;
        if (!ReadFile(handle, buffer.data(), bufferSize, &bytesRead, nullptr)) {
            break;
        }
        if (bytesRead == 0) {
            break;
        }

        if (target) {
            try {
                target(std::string_view(buffer.data(), bytesRead));
            } catch (...) {
            }
        }
    }
}

int RunProgram(const std::string& programName,
               const std::vector<std::string>& args,
               const std::function<void(std::string_view)>& stdOutRedirect,
               const std::function<void(std::string_view)>& stdErrRedirect) {
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

    SECURITY_ATTRIBUTES securityattributes{};
    securityattributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    securityattributes.bInheritHandle = TRUE;

    HANDLE handleStdOutRead = nullptr;
    HANDLE handleStdOutWrite = nullptr;
    if (!CreatePipe(&handleStdOutRead, &handleStdOutWrite, &securityattributes, 0)) {
        return -1;
    }
    auto handleStdOutScope = HyoutaUtils::MakeScopeGuard([&]() {
        if (handleStdOutRead) {
            CloseHandle(handleStdOutRead);
            handleStdOutRead = nullptr;
        }
        if (handleStdOutWrite) {
            CloseHandle(handleStdOutWrite);
            handleStdOutWrite = nullptr;
        }
    });
    if (!SetHandleInformation(handleStdOutRead, HANDLE_FLAG_INHERIT, 0)) {
        return -1;
    }

    HANDLE handleStdErrRead = nullptr;
    HANDLE handleStdErrWrite = nullptr;
    if (!CreatePipe(&handleStdErrRead, &handleStdErrWrite, &securityattributes, 0)) {
        return -1;
    }
    auto handleStdErrScope = HyoutaUtils::MakeScopeGuard([&]() {
        if (handleStdErrRead) {
            CloseHandle(handleStdErrRead);
            handleStdErrRead = nullptr;
        }
        if (handleStdErrWrite) {
            CloseHandle(handleStdErrWrite);
            handleStdErrWrite = nullptr;
        }
    });
    if (!SetHandleInformation(handleStdErrRead, HANDLE_FLAG_INHERIT, 0)) {
        return -1;
    }

    std::array<HANDLE, 2> handlesToInherit{};
    handlesToInherit[0] = handleStdOutWrite;
    handlesToInherit[1] = handleStdErrWrite;

    STARTUPINFO startupinfo{};
    startupinfo.cb = sizeof(STARTUPINFO);
    startupinfo.hStdOutput = handleStdOutWrite;
    startupinfo.hStdError = handleStdErrWrite;
    startupinfo.hStdInput = INVALID_HANDLE_VALUE;
    startupinfo.dwFlags = STARTF_FORCEOFFFEEDBACK | STARTF_USESTDHANDLES;
    PROCESS_INFORMATION processinfo{};
    if (!CreateProcessWithExplicitHandles(wideProgName->c_str(),
                                          wideArgsString->data(),
                                          nullptr,
                                          nullptr,
                                          TRUE,
                                          CREATE_UNICODE_ENVIRONMENT | IDLE_PRIORITY_CLASS,
                                          nullptr,
                                          nullptr,
                                          &startupinfo,
                                          &processinfo,
                                          2,
                                          handlesToInherit.data())) {
        return -1;
    }
    auto closeHandles = HyoutaUtils::MakeScopeGuard([&]() {
        CloseHandle(processinfo.hThread);
        CloseHandle(processinfo.hProcess);
    });
    CloseHandle(handleStdOutWrite);
    handleStdOutWrite = nullptr;
    CloseHandle(handleStdErrWrite);
    handleStdErrWrite = nullptr;

    std::thread stdOutThread([&]() { RedirectOutput(handleStdOutRead, stdOutRedirect); });
    auto joinStdOutThread = HyoutaUtils::MakeScopeGuard([&]() {
        if (stdOutThread.joinable()) {
            stdOutThread.join();
        }
    });
    std::thread stdErrThread([&]() { RedirectOutput(handleStdErrRead, stdErrRedirect); });
    auto joinStdErrThread = HyoutaUtils::MakeScopeGuard([&]() {
        if (stdErrThread.joinable()) {
            stdErrThread.join();
        }
    });

    WaitForSingleObject(processinfo.hProcess, INFINITE);
    DWORD rv = 0;
    if (!GetExitCodeProcess(processinfo.hProcess, &rv)) {
        return -1;
    }
    return static_cast<int>(rv);
}
#else
int RunProgram(const std::string& programName,
               const std::vector<std::string>& args,
               const std::function<void(std::string_view)>& stdOutRedirect,
               const std::function<void(std::string_view)>& stdErrRedirect) {
    return -1;
}
#endif
} // namespace VodArchiver
