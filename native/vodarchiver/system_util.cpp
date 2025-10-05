#include "system_util.h"

#include <cstdint>
#include <optional>
#include <string_view>

#include "util/file.h"
#include "util/text.h"

#ifdef BUILD_FOR_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <sys/statvfs.h>
#endif

namespace VodArchiver {
std::optional<uint64_t> GetFreeDiskSpaceAtPath(std::string_view path) {
#ifdef BUILD_FOR_WINDOWS
    std::string_view p = path;
    while (p.size() > 0 && !(p.back() == '/' || p.back() == '\\')) {
        p = p.substr(0, p.size() - 1);
    }
    if (p.empty()) {
        return std::nullopt;
    }
    auto wstr = HyoutaUtils::TextUtils::Utf8ToWString(p.data(), p.size());
    if (!wstr) {
        return std::nullopt;
    }
    ULARGE_INTEGER availableFreeBytes{};
    ULARGE_INTEGER totalBytes{};
    ULARGE_INTEGER totalFreeBytes{};
    if (!GetDiskFreeSpaceExW(wstr->c_str(), &availableFreeBytes, &totalBytes, &totalFreeBytes)) {
        return std::nullopt;
    }
    return static_cast<uint64_t>(availableFreeBytes.QuadPart);
#else
    std::string_view p = path;
    while (p.size() > 0 && p.back() != '/') {
        p = p.substr(0, p.size() - 1);
    }
    if (p.empty()) {
        return std::nullopt;
    }
    std::string s(p);
    struct statvfs vfs{};
    if (statvfs(s.c_str(), &vfs) != 0) {
        return std::nullopt;
    }

    uint64_t result = static_cast<uint64_t>(vfs.f_bavail);
    result *= static_cast<uint64_t>(vfs.f_bsize);
    return result;
#endif
}
} // namespace VodArchiver
