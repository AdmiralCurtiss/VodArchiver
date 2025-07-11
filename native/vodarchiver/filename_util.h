#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace VodArchiver {
std::string MakeStringFileSystemSafeBaseName(std::string_view s);
std::string FileSystemEscapeName(std::string_view s);
std::string MakeIntercapsFilename(std::string_view gamename);
std::string Crop(std::string_view s, size_t length);
} // namespace VodArchiver
