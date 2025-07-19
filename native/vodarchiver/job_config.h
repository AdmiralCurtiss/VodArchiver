#pragma once

#include <cstdint>
#include <mutex>
#include <string>

namespace VodArchiver {
// config data that the user may change at any time through the GUI
struct JobConfig {
    std::recursive_mutex Mutex; // must be held when accessing anything else here

    std::string TargetFolderPath;
    std::string TempFolderPath;
    std::string TwitchClientId;
    std::string TwitchClientSecret;
    uint64_t MinimumFreeSpaceBytes = 0;
    uint64_t AbsoluteMinimumFreeSpaceBytes = 0;
};
} // namespace VodArchiver
