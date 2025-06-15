#include "gui_user_settings.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "util/file.h"
#include "util/ini.h"
#include "util/ini_writer.h"
#include "util/system.h"
#include "util/text.h"

namespace VodArchiver {
bool LoadUserSettingsFromIni(GuiUserSettings& settings, std::string_view path) {
    HyoutaUtils::IO::File file(path, HyoutaUtils::IO::OpenMode::Read);
    if (!file.IsOpen()) {
        return false;
    }
    HyoutaUtils::Ini::IniFile ini;
    if (!ini.ParseFile(file)) {
        return false;
    }
    return LoadUserSettingsFromIni(settings, ini);
}

bool LoadUserSettingsFromIni(GuiUserSettings& settings, const HyoutaUtils::Ini::IniFile& ini) {
    auto* jpConfirm = ini.FindValue("GuiBehavior", "GamepadSwapConfirmCancel");
    if (jpConfirm) {
        settings.GamepadSwapConfirmCancel =
            HyoutaUtils::TextUtils::CaseInsensitiveEquals(jpConfirm->Value, "true");
    }
    auto* useCustomFileBrowser = ini.FindValue("GuiBehavior", "UseCustomFileBrowser");
    if (useCustomFileBrowser) {
        if (HyoutaUtils::TextUtils::CaseInsensitiveEquals(useCustomFileBrowser->Value, "Always")) {
            settings.UseCustomFileBrowser = GuiUserSettings_UseCustomFileBrowser::Always;
        } else if (HyoutaUtils::TextUtils::CaseInsensitiveEquals(useCustomFileBrowser->Value,
                                                                 "Never")) {
            settings.UseCustomFileBrowser = GuiUserSettings_UseCustomFileBrowser::Never;
        } else {
            settings.UseCustomFileBrowser = GuiUserSettings_UseCustomFileBrowser::Auto;
        }
    }
    return true;
}

bool WriteUserSettingsToIni(const GuiUserSettings& settings, std::string_view path) {
    // read existing ini first to keep values from eg. later versions
    HyoutaUtils::Ini::IniWriter writer;
    {
        HyoutaUtils::Ini::IniFile ini;
        HyoutaUtils::IO::File file(path, HyoutaUtils::IO::OpenMode::Read);
        if (file.IsOpen() && ini.ParseFile(file)) {
            writer.AddExistingIni(ini);
        }
    }
    if (!WriteUserSettingsToIni(settings, writer)) {
        return false;
    }
    const std::string text = writer.GenerateIniText();
    return HyoutaUtils::IO::WriteFileAtomic(path, text.data(), text.size());
}

bool WriteUserSettingsToIni(const GuiUserSettings& settings, HyoutaUtils::Ini::IniWriter& ini) {
    ini.SetBool("GuiBehavior", "GamepadSwapConfirmCancel", settings.GamepadSwapConfirmCancel);
    ini.SetString(
        "GuiBehavior",
        "UseCustomFileBrowser",
        settings.UseCustomFileBrowser == GuiUserSettings_UseCustomFileBrowser::Always  ? "Always"
        : settings.UseCustomFileBrowser == GuiUserSettings_UseCustomFileBrowser::Never ? "Never"
                                                                                       : "Auto");
    return true;
}

bool EvalUseCustomFileBrowser(const GuiUserSettings& settings) {
    // Don't use the native dialog if we're in a context where we want a controller-navigable UI.
    return settings.UseCustomFileBrowser == GuiUserSettings_UseCustomFileBrowser::Always ? true
           : settings.UseCustomFileBrowser == GuiUserSettings_UseCustomFileBrowser::Never
               ? false
               : (HyoutaUtils::Sys::GetEnvironmentVar("SteamTenfoot") == "1"
                  || HyoutaUtils::Sys::GetEnvironmentVar("SteamDeck") == "1");
}
} // namespace VodArchiver
