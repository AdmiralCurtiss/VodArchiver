#pragma once

#include <optional>
#include <string>

namespace VodArchiver::CommonPaths {
std::optional<std::string> GetLocalAppDataFolder();
std::optional<std::string> GetLocalVodArchiverGuiSettingsFolder();
std::optional<std::string> GetMyVideosFolder();
} // namespace VodArchiver::CommonPaths
