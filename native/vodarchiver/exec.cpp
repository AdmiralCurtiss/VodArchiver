#include "exec.h"

#include <format>
#include <string>
#include <thread>
#include <vector>

#include "util/scope.h"
#include "util/text.h"
#include "util/thread.h"

#ifdef BUILD_FOR_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <array>
#include <fcntl.h>
#include <poll.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace VodArchiver {
#ifdef BUILD_FOR_WINDOWS
void AppendArgEscaped(std::string& s, std::string_view arg) {
    s.push_back('"');
    size_t i = 0;
    while (i < arg.size()) {
        char c = arg[i];
        if (c == '\\') {
            // this is the weird one. we must now read ahead and count the number of backslashes,
            // then check if the next character after is a quote or not
            size_t numberOfBackslashes = 1;
            ++i;
            while (i < arg.size() && arg[i] == '\\') {
                ++numberOfBackslashes;
                ++i;
            }
            const bool nextIsQuote = (i < arg.size() && arg[i] == '"');
            if (nextIsQuote) {
                // append (numberOfBackslashes*2)+1 backslashes, then the quote
                for (size_t j = 0; j < numberOfBackslashes; ++j) {
                    s.push_back('\\');
                    s.push_back('\\');
                }
                s.push_back('\\');
                s.push_back(arg[i]);
            } else if (i < arg.size()) {
                // just append the backslashes and next character as-is
                for (size_t j = 0; j < numberOfBackslashes; ++j) {
                    s.push_back('\\');
                }
                s.push_back(arg[i]);
            } else {
                // end of the argument
                // append (numberOfBackslashes*2) backslashes, then the closing quote
                for (size_t j = 0; j < numberOfBackslashes; ++j) {
                    s.push_back('\\');
                    s.push_back('\\');
                }
                break;
            }
        } else if (c == '"') {
            // this only happens if the quote wasn't preceeded by backslashes,
            // so we can safely escape this with one backslash
            s.push_back('\\');
            s.push_back(c);
        } else {
            s.push_back(c);
        }
        ++i;
    }
    s.push_back('"');
}

std::string BuildArgsString(const std::string& programName, const std::vector<std::string>& args) {
    std::string s;

    // the program name is part of the arg string but has different escaping rules. as far as I can
    // tell it's impossible to pass a quote in this string, doing so just toggles the in-quote state
    // of the argument parser, so we just drop them. everything else is passed verbatim.
    s.push_back('"');
    for (size_t i = 0; i < programName.size(); ++i) {
        char c = programName[i];
        if (c != '"') {
            s.push_back(c);
        }
    }
    s.push_back('"');

    for (const std::string& arg : args) {
        s.push_back(' ');
        AppendArgEscaped(s, arg);
    }

    return s;
}

// from https://devblogs.microsoft.com/oldnewthing/20111216-00/?p=8873
static BOOL CreateProcessWithExplicitHandles(__in_opt LPCWSTR lpApplicationName,
                                             __inout_opt LPWSTR lpCommandLine,
                                             __in_opt LPSECURITY_ATTRIBUTES lpProcessAttributes,
                                             __in_opt LPSECURITY_ATTRIBUTES lpThreadAttributes,
                                             __in BOOL bInheritHandles,
                                             __in DWORD dwCreationFlags,
                                             __in_opt LPVOID lpEnvironment,
                                             __in_opt LPCWSTR lpCurrentDirectory,
                                             __in LPSTARTUPINFO lpStartupInfo,
                                             __out LPPROCESS_INFORMATION lpProcessInformation,
                                             // here is the new stuff
                                             __in DWORD cHandlesToInherit,
                                             __in_ecount(cHandlesToInherit)
                                                 HANDLE* rgHandlesToInherit) {
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
        STARTUPINFOEX info{};
        info.StartupInfo = *lpStartupInfo;
        info.StartupInfo.cb = sizeof(info);
        info.lpAttributeList = lpAttributeList;
        fSuccess = CreateProcessW(lpApplicationName,
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
                                          CREATE_UNICODE_ENVIRONMENT | IDLE_PRIORITY_CLASS
                                              | CREATE_NO_WINDOW,
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

    std::thread stdOutThread([&]() {
        HyoutaUtils::SetThreadName("stdoutThread");
        RedirectOutput(handleStdOutRead, stdOutRedirect);
    });
    auto joinStdOutThread = HyoutaUtils::MakeScopeGuard([&]() {
        if (stdOutThread.joinable()) {
            stdOutThread.join();
        }
    });
    std::thread stdErrThread([&]() {
        HyoutaUtils::SetThreadName("stderrThread");
        RedirectOutput(handleStdErrRead, stdErrRedirect);
    });
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
    pid_t child_pid = 0;
    {
        std::array<int, 2> stdout_pipe{};
        std::array<int, 2> stderr_pipe{};
        bool stdout_pipe_initialized = false;
        bool stderr_pipe_initialized = false;
        auto stdout_pipe_0_guard = HyoutaUtils::MakeDisposableScopeGuard([&]() {
            if (stdout_pipe_initialized) {
                close(stdout_pipe[0]);
            }
        });
        auto stderr_pipe_0_guard = HyoutaUtils::MakeDisposableScopeGuard([&]() {
            if (stderr_pipe_initialized) {
                close(stderr_pipe[0]);
            }
        });

        {
            // posix_spawnp() needs non-const char pointers for some reason, set that up...
            std::vector<std::string> mutable_args;
            mutable_args.push_back(programName);
            mutable_args.insert(mutable_args.end(), args.begin(), args.end());
            std::vector<char*> arg_pointers;
            for (std::string& arg : mutable_args) {
                arg_pointers.push_back(arg.data());
            }
            arg_pointers.push_back(nullptr);
            {
                // make two pipes that redirect the stdout and stderr of the child process to us
                if (pipe2(stdout_pipe.data(), O_NONBLOCK | O_CLOEXEC) != 0) {
                    return -1;
                }
                stdout_pipe_initialized = true;
                auto stdout_pipe_1_guard =
                    HyoutaUtils::MakeScopeGuard([&]() { close(stdout_pipe[1]); });
                if (pipe2(stderr_pipe.data(), O_NONBLOCK | O_CLOEXEC) != 0) {
                    return -1;
                }
                stderr_pipe_initialized = true;
                auto stderr_pipe_1_guard =
                    HyoutaUtils::MakeScopeGuard([&]() { close(stderr_pipe[1]); });

                // set up the write end of the pipes as the child's stdout and stderr
                posix_spawn_file_actions_t file_actions;
                if (posix_spawn_file_actions_init(&file_actions) != 0) {
                    return -1;
                }
                auto file_actions_guard = HyoutaUtils::MakeScopeGuard(
                    [&]() { posix_spawn_file_actions_destroy(&file_actions); });
                if (posix_spawn_file_actions_adddup2(&file_actions, stdout_pipe[1], STDOUT_FILENO)
                    != 0) {
                    return -1;
                }
                if (posix_spawn_file_actions_adddup2(&file_actions, stderr_pipe[1], STDERR_FILENO)
                    != 0) {
                    return -1;
                }

                // spawn the child process
                if (posix_spawnp(&child_pid,
                                 programName.c_str(),
                                 &file_actions,
                                 nullptr,
                                 arg_pointers.data(),
                                 nullptr)
                    != 0) {
                    return -1;
                }

                // we leave the read end of our redirect pipes open, but close the write end
                // (happens automatically via scope guards)
            }
        }

        // now poll and read the redirected stdout/stderr until they're closed from the other side
        std::array<struct pollfd, 2> pollfds{};
        pollfds[0].fd = stdout_pipe[0];
        pollfds[0].events = POLLIN | POLLRDHUP;
        pollfds[1].fd = stderr_pipe[0];
        pollfds[1].events = POLLIN | POLLRDHUP;
        static constexpr size_t bufferSize = 4096;
        std::array<char, bufferSize> buffer;
        struct pollfd* fds_to_poll = pollfds.data();
        nfds_t number_of_poll_fds = static_cast<nfds_t>(2);
        bool stdout_hung_up = false;
        bool stderr_hung_up = false;
        while (true) {
            const int p = poll(fds_to_poll, number_of_poll_fds, -1);
            if (p <= 0) {
                break;
            }
            if (!stdout_hung_up) {
                if (pollfds[0].revents & POLLIN) {
                    ssize_t bytes_read = 0;
                    while (true) {
                        bytes_read = read(stdout_pipe[0], buffer.data(), bufferSize);
                        if (bytes_read <= 0) {
                            break;
                        }
                        std::string_view str(buffer.data(), static_cast<size_t>(bytes_read));
                        stdOutRedirect(str);
                        if (static_cast<size_t>(bytes_read) < bufferSize) {
                            break;
                        }
                    }
                }
                if (pollfds[0].revents & POLLHUP) {
                    stdout_hung_up = true;
                    if (!stderr_hung_up) {
                        // only poll stderr from now
                        fds_to_poll = &pollfds[1];
                        number_of_poll_fds = static_cast<nfds_t>(1);
                    }
                }
            }
            if (!stderr_hung_up) {
                if (pollfds[1].revents & POLLIN) {
                    ssize_t bytes_read = 0;
                    while (true) {
                        bytes_read = read(stderr_pipe[0], buffer.data(), bufferSize);
                        if (bytes_read <= 0) {
                            break;
                        }
                        std::string_view str(buffer.data(), static_cast<size_t>(bytes_read));
                        stdErrRedirect(str);
                        if (static_cast<size_t>(bytes_read) < bufferSize) {
                            break;
                        }
                    }
                }
                if (pollfds[1].revents & POLLHUP) {
                    stderr_hung_up = true;
                    if (!stdout_hung_up) {
                        // only poll stdout from now
                        fds_to_poll = pollfds.data();
                        number_of_poll_fds = static_cast<nfds_t>(1);
                    }
                }
            }
            if (stdout_hung_up && stderr_hung_up) {
                break;
            }
        }
    }

    // wait for process to exit and get its exit status
    int exit_status = -1;
    if (waitpid(child_pid, &exit_status, 0) == -1) {
        return -1;
    }
    if (WIFEXITED(exit_status) == 0) {
        return -1;
    }
    return static_cast<int8_t>(static_cast<uint8_t>(WEXITSTATUS(exit_status)));
}
#endif
} // namespace VodArchiver
