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

    // things below this line do not require holding the Mutex

    // pointer to JobList::JobsLock
    std::recursive_mutex* JobsLock;

    // global disk IO locking so multiple threads don't slow eachother to a crawl by accessing the
    // same hard drive at the same time
    DiskMutex ExpensiveDiskIO;
};
} // namespace VodArchiver
