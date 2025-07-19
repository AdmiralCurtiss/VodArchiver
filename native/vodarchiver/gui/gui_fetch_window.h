#pragma once

#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

#include "imgui.h"

#include "gui_background_task.h"
#include "gui_window.h"
#include "vodarchiver/job_config.h"
#include "vodarchiver/task_cancellation.h"
#include "vodarchiver/task_reporting_from_thread.h"
#include "vodarchiver/userinfo/i-user-info.h"
#include "vodarchiver/videoinfo/i-video-info.h"
#include "window_id_management.h"

namespace VodArchiver::GUI {
struct FetchWindow : public VodArchiver::GUI::Window {
    FetchWindow(GuiState& state);
    FetchWindow(const FetchWindow& other) = delete;
    FetchWindow(FetchWindow&& other) = delete;
    FetchWindow& operator=(const FetchWindow& other) = delete;
    FetchWindow& operator=(FetchWindow&& other) = delete;
    ~FetchWindow();

    bool RenderFrame(GuiState& state) override;
    void Cleanup(GuiState& state) override;

private:
    static constexpr char WindowTitle[] = "Fetch Videos";
    std::array<char, GetWindowIdBufferLength(sizeof(WindowTitle))> WindowIdString;
    size_t WindowId;

    std::vector<std::unique_ptr<IVideoInfo>> FetchedItems;
    std::unique_ptr<IUserInfo> FetchTaskActiveUserInfo;
    size_t FetchTaskOffset = 0;
    bool FetchHasMore = false;
    BackgroundTask<FetchReturnValue, JobConfig*, IUserInfo*, size_t, bool> FetchTask;
    TaskCancellation FetchTaskCancellation;
    TaskReportingFromThread FetchTaskReporting;

    size_t SelectedPreset = std::numeric_limits<size_t>::max();
    uint32_t SelectedService = 0;
    std::array<char, 1024> UsernameTextfield{};
    bool SaveCheckbox = false;
    bool FlatCheckbox = false;

    ImVector<int> Items;
    ImVector<int> Selection;
    bool ItemsNeedSort = false;
};
} // namespace VodArchiver::GUI
