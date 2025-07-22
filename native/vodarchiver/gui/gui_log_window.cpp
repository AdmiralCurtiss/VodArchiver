#include "gui_log_window.h"

#include <array>
#include <cstddef>
#include <string>

#include "imgui.h"

#include "util/scope.h"

#include "gui_state.h"
#include "gui_window.h"
#include "window_id_management.h"

namespace VodArchiver::GUI {
void LogWindow::Cleanup(GuiState& state) {
    state.WindowIdsSettingsWindow.ReturnId(WindowId);
}

LogWindow::LogWindow(GuiState& state, std::string logMessage)
  : WindowId(GenerateWindowId(state.WindowIdsSettingsWindow,
                              WindowIdString.data(),
                              WindowIdString.size(),
                              WindowTitle,
                              sizeof(WindowTitle)))
  , LogMessage(std::move(logMessage)) {}

LogWindow::~LogWindow() = default;

bool LogWindow::RenderFrame(GuiState& state) {
    ImGui::SetNextWindowSize(ImVec2(600.0f * state.CurrentDpi, 300.0f * state.CurrentDpi),
                             ImGuiCond_FirstUseEver);
    bool open = true;
    bool visible = ImGui::Begin(WindowIdString.data(), &open, ImGuiWindowFlags_None);
    auto windowScope = HyoutaUtils::MakeScopeGuard([&]() { ImGui::End(); });
    if (!visible) {
        return open;
    }

    static constexpr char closeLabel[] = "Close";
    auto closeTextSize = ImGui::CalcTextSize(closeLabel, nullptr, true);
    float closeButtonWidth = closeTextSize.x + (ImGui::GetStyle().FramePadding.x * 2.0f);
    float closeButtonHeight = closeTextSize.y + (ImGui::GetStyle().FramePadding.y * 2.0f);

    ImGui::InputTextMultiline("##Text",
                              LogMessage.data(),
                              LogMessage.size() + 1,
                              ImVec2(-FLT_MIN,
                                     ImGui::GetContentRegionAvail().y
                                         - (closeButtonHeight + ImGui::GetStyle().ItemSpacing.y)),
                              ImGuiInputTextFlags_ReadOnly);

    {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x
                             - closeButtonWidth);
        if (ImGui::Button(closeLabel)) {
            open = false;
        }
    }

    return open;
}
} // namespace VodArchiver::GUI
