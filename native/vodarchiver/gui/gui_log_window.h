#pragma once

#include <array>
#include <cstddef>
#include <string>

#include "gui_state.h"
#include "gui_window.h"
#include "window_id_management.h"

namespace VodArchiver::GUI {
struct LogWindow : public VodArchiver::GUI::Window {
    LogWindow(GuiState& state, std::string logMessage);
    LogWindow(const LogWindow& other) = delete;
    LogWindow(LogWindow&& other) = delete;
    LogWindow& operator=(const LogWindow& other) = delete;
    LogWindow& operator=(LogWindow&& other) = delete;
    ~LogWindow();

    bool RenderFrame(GuiState& state) override;
    void Cleanup(GuiState& state) override;

private:
    static constexpr char WindowTitle[] = "Log";
    std::array<char, GetWindowIdBufferLength(sizeof(WindowTitle))> WindowIdString;
    size_t WindowId;

    std::string LogMessage;
};
} // namespace VodArchiver::GUI
