#include "gui_settings_window.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <mutex>

#include "imgui.h"

#include "util/number.h"
#include "util/scope.h"
#include "util/text.h"

#include "gui_state.h"
#include "gui_user_settings.h"
#include "gui_window.h"
#include "window_id_management.h"

namespace VodArchiver::GUI {
void SettingsWindow::Cleanup(GuiState& state) {
    state.WindowIdsSettingsWindow.ReturnId(WindowId);
}

SettingsWindow::SettingsWindow(GuiState& state)
  : WindowId(GenerateWindowId(state.WindowIdsSettingsWindow,
                              WindowIdString.data(),
                              WindowIdString.size(),
                              WindowTitle,
                              sizeof(WindowTitle))) {
    HyoutaUtils::TextUtils::WriteToFixedLengthBuffer(
        TargetFolderPath, GetTargetFolderPath(state.GuiSettings), true);
    HyoutaUtils::TextUtils::WriteToFixedLengthBuffer(
        TempFolderPath, GetTempFolderPath(state.GuiSettings), true);
    HyoutaUtils::TextUtils::WriteToFixedLengthBuffer(
        PersistentDataLocation, GetPersistentDataPath(state.GuiSettings), true);
    HyoutaUtils::TextUtils::WriteToFixedLengthBuffer(
        TwitchClientId, state.GuiSettings.TwitchClientId, true);
    HyoutaUtils::TextUtils::WriteToFixedLengthBuffer(
        TwitchClientSecret, state.GuiSettings.TwitchClientSecret, true);
    std::format_to_n(MinimumFreeSpace.data(),
                     MinimumFreeSpace.size() - 1,
                     "{}",
                     state.GuiSettings.MinimumFreeSpaceBytes);
    std::format_to_n(AbsoluteMinimumFreeSpace.data(),
                     AbsoluteMinimumFreeSpace.size() - 1,
                     "{}",
                     state.GuiSettings.AbsoluteMinimumFreeSpaceBytes);
    UseCustomPersistentDataLocation = state.GuiSettings.UseCustomPersistentDataPath;
}

SettingsWindow::~SettingsWindow() = default;

bool SettingsWindow::RenderFrame(GuiState& state) {
    ImGui::SetNextWindowSize(ImVec2(600.0f * state.CurrentDpi, 300.0f * state.CurrentDpi),
                             ImGuiCond_FirstUseEver);
    bool open = true;
    bool visible = ImGui::Begin(WindowIdString.data(), &open, ImGuiWindowFlags_None);
    auto windowScope = HyoutaUtils::MakeScopeGuard([&]() { ImGui::End(); });
    if (!visible) {
        return open;
    }

    if (ImGui::BeginTable("Table", 2, ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Download Folder:");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::InputText("##TargetFolder",
                             TargetFolderPath.data(),
                             TargetFolderPath.size(),
                             ImGuiInputTextFlags_ElideLeft)) {
            TargetFolderPathEdited = true;
        }

        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Temp Folder for Parts:");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::InputText("##TempFolder",
                             TempFolderPath.data(),
                             TempFolderPath.size(),
                             ImGuiInputTextFlags_ElideLeft)) {
            TempFolderPathEdited = true;
        }

        ImGui::TableNextColumn();
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::Checkbox("Custom Location for Persistent Download & User Lists",
                            &UseCustomPersistentDataLocation)) {
            UseCustomPersistentDataLocationEdited = true;
        }

        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Custom Location:");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::InputText("##PersistentDataLocation",
                             PersistentDataLocation.data(),
                             PersistentDataLocation.size(),
                             ImGuiInputTextFlags_ElideLeft)) {
            PersistentDataLocationEdited = true;
        }

        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Twitch Client ID:");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::InputText("##TwitchClientId",
                             TwitchClientId.data(),
                             TwitchClientId.size(),
                             ImGuiInputTextFlags_ElideLeft)) {
            TwitchClientIdEdited = true;
        }

        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Twitch Client Secret:");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::InputText("##TwitchClientSecret",
                             TwitchClientSecret.data(),
                             TwitchClientSecret.size(),
                             ImGuiInputTextFlags_ElideLeft)) {
            TwitchClientSecretEdited = true;
        }

        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Standard Minimum Free Space:");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::InputText("##MinimumFreeSpace",
                             MinimumFreeSpace.data(),
                             MinimumFreeSpace.size(),
                             ImGuiInputTextFlags_ElideLeft)) {
            MinimumFreeSpaceEdited = true;
        }

        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Absolute Minimum Free Space:");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::InputText("##AbsoluteMinimumFreeSpace",
                             AbsoluteMinimumFreeSpace.data(),
                             AbsoluteMinimumFreeSpace.size(),
                             ImGuiInputTextFlags_ElideLeft)) {
            AbsoluteMinimumFreeSpaceEdited = true;
        }

        ImGui::EndTable();
    }

    static constexpr char closeLabel[] = "Close";
    static constexpr char saveLabel[] = "Save";
    float closeTextWidth = ImGui::CalcTextSize(closeLabel, nullptr, true).x;
    float saveTextWidth = ImGui::CalcTextSize(saveLabel, nullptr, true).x;
    float closeButtonWidth = closeTextWidth + (ImGui::GetStyle().FramePadding.x * 2.0f);
    float saveButtonWidth = saveTextWidth + (ImGui::GetStyle().FramePadding.x * 2.0f);
    {
        ImGui::SetCursorPosX(
            ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x
            - (saveButtonWidth + ImGui::GetStyle().ItemSpacing.x + closeButtonWidth));
        if (ImGui::Button(closeLabel)) {
            open = false;
        }
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x
                             - saveButtonWidth);
        if (ImGui::Button(saveLabel)) {
            if (TargetFolderPathEdited) {
                state.GuiSettings.CustomTargetFolderPath =
                    std::string(HyoutaUtils::TextUtils::StripToNull(TargetFolderPath));
            }
            if (TempFolderPathEdited) {
                state.GuiSettings.CustomTempFolderPath =
                    std::string(HyoutaUtils::TextUtils::StripToNull(TempFolderPath));
            }
            if (UseCustomPersistentDataLocationEdited) {
                state.GuiSettings.UseCustomPersistentDataPath = UseCustomPersistentDataLocation;
            }
            if (PersistentDataLocationEdited) {
                state.GuiSettings.CustomPersistentDataPath =
                    std::string(HyoutaUtils::TextUtils::StripToNull(PersistentDataLocation));
            }
            if (TwitchClientIdEdited) {
                state.GuiSettings.TwitchClientId =
                    std::string(HyoutaUtils::TextUtils::StripToNull(TwitchClientId));
            }
            if (TwitchClientSecretEdited) {
                state.GuiSettings.TwitchClientSecret =
                    std::string(HyoutaUtils::TextUtils::StripToNull(TwitchClientSecret));
            }
            if (MinimumFreeSpaceEdited) {
                auto p = HyoutaUtils::NumberUtils::ParseUInt64(
                    HyoutaUtils::TextUtils::StripToNull(MinimumFreeSpace));
                if (p) {
                    state.GuiSettings.MinimumFreeSpaceBytes = *p;
                }
            }
            if (AbsoluteMinimumFreeSpaceEdited) {
                auto p = HyoutaUtils::NumberUtils::ParseUInt64(
                    HyoutaUtils::TextUtils::StripToNull(AbsoluteMinimumFreeSpace));
                if (p) {
                    state.GuiSettings.AbsoluteMinimumFreeSpaceBytes = *p;
                }
            }

            std::lock_guard lock(state.JobConf.Mutex);
            state.JobConf.TargetFolderPath = GetTargetFolderPath(state.GuiSettings);
            state.JobConf.TempFolderPath = GetTempFolderPath(state.GuiSettings);
            state.JobConf.TwitchClientId = state.GuiSettings.TwitchClientId;
            state.JobConf.TwitchClientSecret = state.GuiSettings.TwitchClientSecret;
            state.JobConf.MinimumFreeSpaceBytes = state.GuiSettings.MinimumFreeSpaceBytes;
            state.JobConf.AbsoluteMinimumFreeSpaceBytes =
                state.GuiSettings.AbsoluteMinimumFreeSpaceBytes;

            open = false;
        }
    }

    return open;
}
} // namespace VodArchiver::GUI
