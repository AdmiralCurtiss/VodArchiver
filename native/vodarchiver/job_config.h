#pragma once

#include <cstdint>
#include <mutex>
#include <string>

#include "disk_lock.h"

namespace VodArchiver {
// config data that the user may change at any time through the GUI
struct JobConfig {
    std::recursive_mutex Mutex; // must be held when accessing any of the configs
    std::string TargetFolderPath;
    std::string TempFolderPath;
    std::string TwitchClientId;
    std::string TwitchClientSecret;
    uint64_t MinimumFreeSpaceBytes = 0;
    uint64_t AbsoluteMinimumFreeSpaceBytes = 0;

    // this may be accessed without the mutex held
    DiskMutex ExpensiveDiskIO;
};
} // namespace VodArchiver
