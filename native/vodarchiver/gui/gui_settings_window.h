#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "gui_state.h"
#include "gui_window.h"
#include "window_id_management.h"

namespace VodArchiver::GUI {
struct SettingsWindow : public VodArchiver::GUI::Window {
    SettingsWindow(GuiState& state);
    SettingsWindow(const SettingsWindow& other) = delete;
    SettingsWindow(SettingsWindow&& other) = delete;
    SettingsWindow& operator=(const SettingsWindow& other) = delete;
    SettingsWindow& operator=(SettingsWindow&& other) = delete;
    ~SettingsWindow();

    bool RenderFrame(GuiState& state) override;
    void Cleanup(GuiState& state) override;

private:
    static constexpr char WindowTitle[] = "Settings";
    std::array<char, GetWindowIdBufferLength(sizeof(WindowTitle))> WindowIdString;
    size_t WindowId;

    std::array<char, 2048> TargetFolderPath{};
    std::array<char, 2048> TempFolderPath{};
    std::array<char, 2048> PersistentDataLocation{};
    std::array<char, 128> TwitchClientId{};
    std::array<char, 128> TwitchClientSecret{};
    std::array<char, 24> MinimumFreeSpace{};
    std::array<char, 24> AbsoluteMinimumFreeSpace{};
    bool UseCustomPersistentDataLocation = false;

    bool TargetFolderPathEdited = false;
    bool TempFolderPathEdited = false;
    bool UseCustomPersistentDataLocationEdited = false;
    bool PersistentDataLocationEdited = false;
    bool TwitchClientIdEdited = false;
    bool TwitchClientSecretEdited = false;
    bool MinimumFreeSpaceEdited = false;
    bool AbsoluteMinimumFreeSpaceEdited = false;
};
} // namespace VodArchiver::GUI
