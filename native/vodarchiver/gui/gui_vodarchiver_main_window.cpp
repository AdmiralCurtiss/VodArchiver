#include "gui_vodarchiver_main_window.h"

#include <array>
#include <condition_variable>
#include <format>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "imgui.h"

#include "gui_state.h"
#include "gui_user_settings.h"
#include "util/file.h"
#include "util/hash/sha1.h"
#include "util/scope.h"
#include "util/system.h"
#include "vodarchiver/common_paths.h"
#include "vodarchiver_imgui_utils.h"
#include "vodarchiver_version.h"

namespace VodArchiver::GUI {
VodArchiverMainWindow::VodArchiverMainWindow() = default;
VodArchiverMainWindow::~VodArchiverMainWindow() = default;

void VodArchiverMainWindow::Cleanup(GuiState& state) {}

bool VodArchiverMainWindow::RenderFrame(GuiState& state) {
    bool open = true;
    bool visible = ImGui::Begin("VodArchiver", &open, ImGuiWindowFlags_MenuBar);
    const auto windowScope = HyoutaUtils::MakeScopeGuard([&]() { ImGui::End(); });
    if (visible) {
        open = RenderContents(state) && open;
    }
    return open;
}

bool VodArchiverMainWindow::HasPendingWindowRequest() const {
    return false;
}

bool VodArchiverMainWindow::RenderContents(GuiState& state) {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Toolbox")) {
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Config")) {
            ImGui::MenuItem("Swap Confirm/Cancel on Controller",
                            nullptr,
                            &state.GuiSettings.GamepadSwapConfirmCancel);
            if (ImGui::BeginMenu("File Browser")) {
                if (ImGui::MenuItem("Use file browser from OS if possible",
                                    nullptr,
                                    state.GuiSettings.UseCustomFileBrowser
                                        == GuiUserSettings_UseCustomFileBrowser::Never)) {
                    state.GuiSettings.UseCustomFileBrowser =
                        GuiUserSettings_UseCustomFileBrowser::Never;
                }
                if (ImGui::MenuItem("Use custom file browser",
                                    nullptr,
                                    state.GuiSettings.UseCustomFileBrowser
                                        == GuiUserSettings_UseCustomFileBrowser::Always)) {
                    state.GuiSettings.UseCustomFileBrowser =
                        GuiUserSettings_UseCustomFileBrowser::Always;
                }
                if (ImGui::MenuItem("Autodetect",
                                    nullptr,
                                    state.GuiSettings.UseCustomFileBrowser
                                        == GuiUserSettings_UseCustomFileBrowser::Auto)) {
                    state.GuiSettings.UseCustomFileBrowser =
                        GuiUserSettings_UseCustomFileBrowser::Auto;
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGuiUtils::TextUnformattedRightAlign("VodArchiver " VODARCHIVER_VERSION);

    ImGui::Spacing();

    auto disabledScope = HyoutaUtils::MakeDisposableScopeGuard([&]() { ImGui::EndDisabled(); });
    if (HasPendingWindowRequest()) {
        ImGui::BeginDisabled();
    } else {
        disabledScope.Dispose();
    }

    if (ImGuiUtils::ButtonRightAlign("Close")) {
        return false;
    }

    return true;
}
} // namespace VodArchiver::GUI
