#include <cstdio>
#include <exception>
#include <memory>
#include <string_view>

#include "vodarchiver_version.h"

#include "util/text.h"

#include "cli_tool.h"

#ifdef BUILD_FOR_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#include "gui/main_gui.h"

namespace VodArchiver {
static constexpr auto CliTools = {
    CliTool{.Name = "GUI",
            .ShortDescription = "Start the interactive GUI.",
            .Function = VodArchiver::RunGui},
};
} // namespace VodArchiver

static void PrintUsage() {
    printf("VodArchiver " VODARCHIVER_VERSION "\n");
    printf("Select one of the following tools via the first argument:\n");
    for (const auto& tool : VodArchiver::CliTools) {
        if (!tool.Hidden) {
            printf(" %-18s %s\n", tool.Name, tool.ShortDescription);
        }
    }
}

static int InternalMain(int argc, char** argvUtf8) {
    try {
        if (argc < 2) {
#ifdef BUILD_FOR_WINDOWS
            // https://devblogs.microsoft.com/oldnewthing/20160125-00/?p=92922
            // Check to see if the user launched this by double-clicking so we can tell them that
            // this is a command line app -- you won't believe how many times I've seen someone be
            // confused by the 'flashing window' in discord...
            DWORD tmp;
            if (GetConsoleProcessList(&tmp, 1) == 1) {
                FreeConsole(); // close the console window
                return VodArchiver::RunGui(argc, argvUtf8);
            } else {
                PrintUsage();
            }
#else
            PrintUsage();
#endif

            return -1;
        }

        const std::string_view name = argvUtf8[1];
        for (const auto& tool : VodArchiver::CliTools) {
            if (HyoutaUtils::TextUtils::CaseInsensitiveEquals(name, tool.Name)) {
                return tool.Function(argc - 1, argvUtf8 + 1);
            }
        }

        PrintUsage();
        return -1;
    } catch (const std::exception& ex) {
        printf("Exception occurred: %s\n", ex.what());
        return -3;
    } catch (...) {
        printf("Unknown error occurred.\n");
        return -4;
    }
}

#ifdef BUILD_FOR_WINDOWS
int wmain(int argc, wchar_t* argv[], wchar_t* envp[]) {
    // don't display any 'please insert a disk into drive x' textboxes
    // when accessing a drive with removable storage
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
    SetConsoleOutputCP(65001); // we use printf() with utf8 strings

    std::unique_ptr<const char*[]> argvUtf8Pts;
    std::unique_ptr<std::string[]> argvStorage;
    const size_t argcSizet = static_cast<size_t>(argc > 0 ? argc : 0);
    argvUtf8Pts = std::make_unique<const char*[]>(argcSizet + 1);
    argvStorage = std::make_unique<std::string[]>(argcSizet);
    for (size_t i = 0; i < argcSizet; ++i) {
        auto utf8 = HyoutaUtils::TextUtils::WStringToUtf8(argv[i], wcslen(argv[i]));
        if (utf8) {
            argvStorage[i] = std::move(*utf8);
        } else {
            printf("Failed to convert argument %zu to UTF8.\n", i);
            return -1;
        }
        argvUtf8Pts[i] = argvStorage[i].c_str();
    }
    argvUtf8Pts[argcSizet] = nullptr;
    return InternalMain(argc, const_cast<char**>(argvUtf8Pts.get()));
}
#else
int main(int argc, char** argv) {
    return InternalMain(argc, argv);
}
#endif
