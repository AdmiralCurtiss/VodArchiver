#pragma once

#include <array>
#include <memory>
#include <string>

#include "gui_file_browser.h"
#include "gui_window.h"
#include "window_id_management.h"

namespace VodArchiver::GUI {
struct VideoSplitWindow : public VodArchiver::GUI::Window {
    VideoSplitWindow(GuiState& state);
    VideoSplitWindow(const VideoSplitWindow& other) = delete;
    VideoSplitWindow(VideoSplitWindow&& other) = delete;
    VideoSplitWindow& operator=(const VideoSplitWindow& other) = delete;
    VideoSplitWindow& operator=(VideoSplitWindow&& other) = delete;
    ~VideoSplitWindow();

    bool RenderFrame(GuiState& state) override;
    void Cleanup(GuiState& state) override;

private:
    static constexpr char WindowTitle[] = "Split Video";
    std::array<char, GetWindowIdBufferLength(sizeof(WindowTitle))> WindowIdString;
    size_t WindowId;

    std::array<char, 1024> InputPath{};
    std::array<char, 1024> SplitTimes{};

    FileBrowser InputFileBrowser;
};
} // namespace VodArchiver::GUI
