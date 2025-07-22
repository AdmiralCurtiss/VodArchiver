#pragma once

#include <memory>
#include <mutex>
#include <vector>

#include "imgui.h"

#include "../job_config.h"
#include "../job_handling.h"
#include "../task_cancellation.h"
#include "../tasks/fetch-task-group.h"
#include "../tasks/video-task-group.h"
#include "gui_user_settings.h"
#include "window_id_management.h"

namespace VodArchiver::GUI {
struct Window;
}

namespace VodArchiver {
struct IVideoJob;
struct IUserInfo;

struct GuiState {
    // A Window may add new windows to this vector at any time, but not remove or modify any.
    std::vector<std::unique_ptr<GUI::Window>> Windows;

    // Window ID management. One instance per window type. This seems kind of convoluted but I'm not
    // sure if there's a better way to do this with imgui?
    VodArchiver::GUI::WindowIdTracker WindowIdsFetchWindow;
    VodArchiver::GUI::WindowIdTracker WindowIdsVideoSplitWindow;
    VodArchiver::GUI::WindowIdTracker WindowIdsSettingsWindow;

    // User settings, like recently used paths.
    VodArchiver::GuiUserSettings GuiSettings;

    float CurrentDpi = 0.0f;

    JobList Jobs;

    std::recursive_mutex UserInfosLock;
    std::vector<std::unique_ptr<IUserInfo>> UserInfos;

    TaskCancellation CancellationToken;
    std::vector<std::unique_ptr<VideoTaskGroup>> VideoTaskGroups;
    std::vector<std::unique_ptr<FetchTaskGroup>> FetchTaskGroups;

    VodArchiver::JobConfig JobConf;

    ~GuiState();
};
} // namespace VodArchiver
