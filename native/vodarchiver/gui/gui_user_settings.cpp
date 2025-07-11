#include "gui_user_settings.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "../common_paths.h"

#include "util/file.h"
#include "util/ini.h"
#include "util/ini_writer.h"
#include "util/system.h"
#include "util/text.h"

namespace VodArchiver {
void InitGuiUserSettings(GuiUserSettings& settings) {
    auto localAppData = CommonPaths::GetLocalVodArchiverGuiSettingsFolder();
    if (localAppData) {
        settings.DefaultPersistentDataPath = std::move(*localAppData);
        HyoutaUtils::IO::AppendPathElement(settings.DefaultPersistentDataPath, "VodArchiver");
        HyoutaUtils::IO::AppendPathElement(settings.DefaultPersistentDataPath, "1.0.0.0");
    } else {
        // TODO: what do we do here? will this ever fail?
        settings.DefaultPersistentDataPath = "";
    }

    auto myVideos = CommonPaths::GetMyVideosFolder();
    if (myVideos) {
        settings.DefaultTargetFolderPath = std::move(*myVideos);
        settings.DefaultTempFolderPath = settings.DefaultTargetFolderPath;
        HyoutaUtils::IO::AppendPathElement(settings.DefaultTempFolderPath, "Temp");
    } else {
        // TODO: what do we do here? will this ever fail?
        settings.DefaultTargetFolderPath = "";
        settings.DefaultTempFolderPath = "";
    }
}

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
    auto* targetFolderPath = ini.FindValue("Paths", "TargetFolderPath");
    if (targetFolderPath) {
        settings.CustomTargetFolderPath = std::string(targetFolderPath->Value);
    }
    auto* tempFolderPath = ini.FindValue("Paths", "TempFolderPath");
    if (tempFolderPath) {
        settings.CustomTempFolderPath = std::string(tempFolderPath->Value);
    }
    auto* persistentDataPath = ini.FindValue("Paths", "PersistentDataPath");
    if (persistentDataPath) {
        settings.CustomPersistentDataPath = std::string(persistentDataPath->Value);
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
    ini.SetString("Paths", "TargetFolderPath", settings.CustomTargetFolderPath);
    ini.SetString("Paths", "TempFolderPath", settings.CustomTempFolderPath);
    ini.SetString("Paths", "PersistentDataPath", settings.CustomPersistentDataPath);
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

const std::string& GetTargetFolderPath(const GuiUserSettings& settings) {
    if (!settings.CustomTargetFolderPath.empty()) {
        return settings.CustomTargetFolderPath;
    }
    return settings.DefaultTargetFolderPath;
}

const std::string& GetTempFolderPath(const GuiUserSettings& settings) {
    if (!settings.CustomTempFolderPath.empty()) {
        return settings.CustomTempFolderPath;
    }
    return settings.DefaultTempFolderPath;
}

const std::string& GetPersistentDataPath(const GuiUserSettings& settings) {
    if (!settings.CustomPersistentDataPath.empty()) {
        return settings.CustomPersistentDataPath;
    }
    return settings.DefaultPersistentDataPath;
}

std::string GetPersistentDataPath(const GuiUserSettings& settings, std::string_view file) {
    std::string path = GetPersistentDataPath(settings);
    HyoutaUtils::IO::AppendPathElement(path, file);
    return path;
}

std::string GetVodXmlPath(const GuiUserSettings& settings) {
    return GetPersistentDataPath(settings, "downloads.bin");
}

std::string GetUserInfoXmlPath(const GuiUserSettings& settings) {
    return GetPersistentDataPath(settings, "users.xml");
}
} // namespace VodArchiver
