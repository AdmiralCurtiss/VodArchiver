#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace HyoutaUtils::Ini {
struct IniFile;
struct IniWriter;
} // namespace HyoutaUtils::Ini

namespace VodArchiver {
enum class GuiUserSettings_UseCustomFileBrowser : uint8_t {
    Auto,
    Always,
    Never, // this will still use the custom browser if we have no other option
};

struct GuiUserSettings {
    bool GamepadSwapConfirmCancel = false;
    GuiUserSettings_UseCustomFileBrowser UseCustomFileBrowser =
        GuiUserSettings_UseCustomFileBrowser::Auto;

    std::string DefaultTargetFolderPath;
    std::string CustomTargetFolderPath;
    std::string DefaultTempFolderPath;
    std::string CustomTempFolderPath;
    std::string DefaultPersistentDataPath;
    std::string CustomPersistentDataPath;
};

void InitGuiUserSettings(GuiUserSettings& settings);
bool LoadUserSettingsFromIni(GuiUserSettings& settings, std::string_view path);
bool LoadUserSettingsFromIni(GuiUserSettings& settings, const HyoutaUtils::Ini::IniFile& ini);
bool WriteUserSettingsToIni(const GuiUserSettings& settings, std::string_view path);
bool WriteUserSettingsToIni(const GuiUserSettings& settings, HyoutaUtils::Ini::IniWriter& ini);

bool EvalUseCustomFileBrowser(const GuiUserSettings& settings);
const std::string& GetTargetFolderPath(const GuiUserSettings& settings);
const std::string& GetTempFolderPath(const GuiUserSettings& settings);
const std::string& GetPersistentDataPath(const GuiUserSettings& settings);
std::string GetPersistentDataPath(const GuiUserSettings& settings, std::string_view file);
std::string GetVodXmlPath(const GuiUserSettings& settings);
std::string GetUserInfoXmlPath(const GuiUserSettings& settings);
} // namespace VodArchiver
