#include "gui_video_split_window.h"

#include <atomic>
#include <cstdio>
#include <string>
#include <thread>

#include "imgui.h"

#include "gui_state.h"
#include "util/scope.h"
#include "util/text.h"
#include "vodarchiver/job_handling.h"
#include "vodarchiver/videojobs/ffmpeg-split-job.h"
#include "vodarchiver_imgui_utils.h"

namespace VodArchiver::GUI {
void VideoSplitWindow::Cleanup(GuiState& state) {
    state.WindowIdsVideoSplitWindow.ReturnId(WindowId);
}

VideoSplitWindow::VideoSplitWindow(GuiState& state)
  : WindowId(GenerateWindowId(state.WindowIdsVideoSplitWindow,
                              WindowIdString.data(),
                              WindowIdString.size(),
                              WindowTitle,
                              sizeof(WindowTitle))) {}

VideoSplitWindow::~VideoSplitWindow() = default;

bool VideoSplitWindow::RenderFrame(GuiState& state) {
    ImGuiUtils::SetInitialNextWindowSizeWidthOnly(400.0f * state.CurrentDpi);
    bool open = true;
    bool visible = ImGui::Begin(WindowIdString.data(), &open, ImGuiWindowFlags_None);
    auto windowScope = HyoutaUtils::MakeScopeGuard([&]() { ImGui::End(); });
    if (!visible) {
        return open;
    }

    {
        if (ImGui::BeginTable("Table", 3, ImGuiTableFlags_SizingFixedFit)) {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Button", ImGuiTableColumnFlags_WidthFixed);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("File:");
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-FLT_MIN);
            ImGui::InputText(
                "##Input", InputPath.data(), InputPath.size(), ImGuiInputTextFlags_ElideLeft);
            ImGui::TableNextColumn();
            if (ImGui::Button("Browse...##BrowseInput")) {
                std::vector<FileFilter> filters;
                filters.reserve(2);
                filters.push_back(FileFilter{"MPEG4 video file (*.mp4)", "*.mp4"});
                filters.push_back(FileFilter{"All files (*.*)", "*"});
                InputFileBrowser.Reset(FileBrowserMode::OpenExistingFile,
                                       HyoutaUtils::TextUtils::StripToNull(InputPath),
                                       std::move(filters),
                                       "p3a",
                                       false,
                                       false,
                                       VodArchiver::EvalUseCustomFileBrowser(state.GuiSettings));
                ImGui::OpenPopup("Select video to split");
            }

            ImGuiUtils::SetNextWindowSizeForNearFullscreenPopup();
            bool modal_open = true;
            if (ImGui::BeginPopupModal(
                    "Select video to split", &modal_open, ImGuiWindowFlags_NoSavedSettings)) {
                FileBrowserResult result =
                    InputFileBrowser.RenderFrame(state, "Select video to split");
                if (result != FileBrowserResult::None) {
                    if (result == FileBrowserResult::FileSelected) {
                        HyoutaUtils::TextUtils::WriteToFixedLengthBuffer(
                            InputPath, InputFileBrowser.GetSelectedPath(), true);
                    }
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Splits:");
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-FLT_MIN);
            ImGui::InputText(
                "##Output", SplitTimes.data(), SplitTimes.size(), ImGuiInputTextFlags_ElideLeft);
            ImGui::TableNextColumn();

            ImGui::EndTable();
        }

        if (ImGuiUtils::ButtonFullWidth("Split")) {
            std::string_view path = HyoutaUtils::TextUtils::StripToNull(InputPath);
            std::string_view times = HyoutaUtils::TextUtils::StripToNull(SplitTimes);
            if (!path.empty() && !times.empty()) {
                auto job = std::make_unique<FFMpegSplitJob>(std::string(path), std::string(times));
                EnqueueJob(state.Jobs, std::move(job), [&](IVideoJob* newJob) {
                    AddJobToTaskGroupIfAutoenqueue(state.VideoTaskGroups, newJob);
                });
                InputPath[0] = '\0';
                SplitTimes[0] = '\0';
            }
        }
    }

    static constexpr char closeLabel[] = "Close";
    float closeTextWidth = ImGui::CalcTextSize(closeLabel, nullptr, true).x;
    float closeButtonWidth = closeTextWidth + (ImGui::GetStyle().FramePadding.x * 2.0f);
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
