#pragma once

#include <optional>
#include <string>

namespace VodArchiver::CommonPaths {
std::optional<std::string> GetSavedGamesFolder();
std::optional<std::string> GetLocalAppDataFolder();
std::optional<std::string> GetLocalVodArchiverGuiSettingsFolder();
} // namespace VodArchiver::CommonPaths
