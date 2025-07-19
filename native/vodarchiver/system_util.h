#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace VodArchiver {
std::optional<uint64_t> GetFreeDiskSpaceAtPath(std::string_view path);
} // namespace VodArchiver
