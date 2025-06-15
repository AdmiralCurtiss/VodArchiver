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
};

bool LoadUserSettingsFromIni(GuiUserSettings& settings, std::string_view path);
bool LoadUserSettingsFromIni(GuiUserSettings& settings, const HyoutaUtils::Ini::IniFile& ini);
bool WriteUserSettingsToIni(const GuiUserSettings& settings, std::string_view path);
bool WriteUserSettingsToIni(const GuiUserSettings& settings, HyoutaUtils::Ini::IniWriter& ini);

bool EvalUseCustomFileBrowser(const GuiUserSettings& settings);
} // namespace VodArchiver
