#include "main_gui.h"

#include <string_view>

#include "imgui.h"

#include "gui_fonts.h"
#include "gui_state.h"
#include "gui_user_settings.h"
#include "gui_vodarchiver_main_window.h"
#include "gui_window.h"
#include "util/file.h"
#include "util/scope.h"
#include "util/text.h"
#include "vodarchiver/common_paths.h"
#include "vodarchiver/curl_util.h"
#include "vodarchiver/job_handling.h"
#include "vodarchiver/userinfo/serialization.h"
#include "vodarchiver/videojobs/serialization.h"
#include "vodarchiver_version.h"

#ifdef BUILD_FOR_WINDOWS
#include "gui_setup_dx11.h"
#else
#include "gui_setup_glfw_vulkan.h"
#endif

namespace VodArchiver {
static bool RenderFrame(ImGuiIO& io, GuiState& state) {
    size_t windowCount = state.Windows.size();
    for (size_t i = 0; i < windowCount;) {
        GUI::Window* window = state.Windows[i].get();
        ImGui::PushID(static_cast<const void*>(window));
        if (!window->RenderFrame(state)) {
            window->Cleanup(state);
            state.Windows.erase(state.Windows.begin() + i);
            --windowCount;
        } else {
            ++i;
        }
        ImGui::PopID();
    }

    // ImGui::ShowDemoWindow();

    return windowCount > 0;
}

int RunGui(int argc, char** argvUtf8) {
    VodArchiver::curl::InitCurl();
    auto curlCleanup = HyoutaUtils::MakeScopeGuard([]() { VodArchiver::curl::DeinitCurl(); });

    GuiState state;
    InitGuiUserSettings(state.GuiSettings);
    std::optional<std::string> guiSettingsFolder =
        CommonPaths::GetLocalVodArchiverGuiSettingsFolder();
    std::string userIniPath;
    std::string imguiIniPath;
    if (guiSettingsFolder) {
        HyoutaUtils::IO::CreateDirectory(std::string_view(*guiSettingsFolder));
        userIniPath = *guiSettingsFolder;
        userIniPath.append("/gui.ini");
        imguiIniPath = *guiSettingsFolder;
        imguiIniPath.append("/imgui.ini");
        VodArchiver::LoadUserSettingsFromIni(state.GuiSettings, userIniPath);
    }

    {
        auto jobs = ParseJobsFromFile(GetVodXmlPath(state.GuiSettings));
        if (jobs) {
            state.Jobs = std::move(*jobs);
        }
    }
    {
        auto userinfos = ParseUserInfosFromFile(GetUserInfoXmlPath(state.GuiSettings));
        if (userinfos) {
            state.UserInfos = std::move(*userinfos);
        }
    }
    {
        std::lock_guard lock(state.JobConf.Mutex);
        state.JobConf.TargetFolderPath = GetTargetFolderPath(state.GuiSettings);
        state.JobConf.TempFolderPath = GetTempFolderPath(state.GuiSettings);
        state.JobConf.TwitchClientId = state.GuiSettings.TwitchClientId;
        state.JobConf.TwitchClientSecret = state.GuiSettings.TwitchClientSecret;
    }

    for (int s = static_cast<int>(StreamService::Unknown);
         s < static_cast<int>(StreamService::COUNT);
         ++s) {
        state.VideoTaskGroups.emplace_back(std::make_unique<VideoTaskGroup>(
            static_cast<StreamService>(s),
            []() {},
            []() {},
            &state.JobConf,
            &state.CancellationToken));
    }

    {
        uint32_t rngSeed =
            static_cast<uint32_t>(DateTime::UtcNow().Data / (DateTime::TICKS_PER_SECOND / 1000));
        auto make_fetch_task_group = [&](std::vector<ServiceVideoCategoryType> services) {
            state.FetchTaskGroups.emplace_back(std::make_unique<FetchTaskGroup>(
                std::move(services),
                &state.UserInfosLock,
                &state.UserInfos,
                &state.JobConf,
                &state.CancellationToken,
                [&](std::unique_ptr<IVideoInfo> info) {
                    std::lock_guard lock(state.JobsLock);
                    CreateAndEnqueueJob(state.Jobs, std::move(info));
                },
                rngSeed));
            rngSeed = ((rngSeed >> 3) | (rngSeed << 29));
        };
        make_fetch_task_group({{ServiceVideoCategoryType::TwitchRecordings,
                                ServiceVideoCategoryType::TwitchHighlights}});
        make_fetch_task_group({{ServiceVideoCategoryType::YoutubeUser,
                                ServiceVideoCategoryType::YoutubeChannel,
                                ServiceVideoCategoryType::YoutubePlaylist,
                                ServiceVideoCategoryType::YoutubeUrl}});
        make_fetch_task_group({{ServiceVideoCategoryType::RssFeed}});
        make_fetch_task_group({{ServiceVideoCategoryType::FFMpegJob}});
    }

    state.Windows.emplace_back(std::make_unique<GUI::VodArchiverMainWindow>());

    const auto load_imgui_ini = [&](ImGuiIO& io, GuiState& state) -> void {
        if (!guiSettingsFolder) {
            return;
        }
        HyoutaUtils::IO::File f(std::string_view(imguiIniPath), HyoutaUtils::IO::OpenMode::Read);
        if (!f.IsOpen()) {
            return;
        }
        auto length = f.GetLength();
        if (!length || *length == 0 || *length > (10 * 1024 * 1024)) {
            return;
        }
        const size_t bufferSize = static_cast<size_t>(*length);
        std::unique_ptr<char[]> buffer(new (std::nothrow) char[bufferSize]);
        if (!buffer) {
            return;
        }
        if (f.Read(buffer.get(), bufferSize) != bufferSize) {
            return;
        }
        ImGui::LoadIniSettingsFromMemory(buffer.get(), bufferSize);
    };
    const auto save_imgui_ini = [&](ImGuiIO& io, GuiState& state) -> void {
        if (!guiSettingsFolder) {
            return;
        }
        size_t length = 0;
        const char* buffer = ImGui::SaveIniSettingsToMemory(&length);
        if (!buffer) {
            return;
        }
        HyoutaUtils::IO::WriteFileAtomic(std::string_view(imguiIniPath), buffer, length);
    };

    const std::string_view windowTitle("VodArchiver " VODARCHIVER_VERSION);
    ImVec4 backgroundColor(0.070f, 0.125f, 0.070f, 1.000f); // dark gray-ish green
    // ImVec4 backgroundColor(0.060f, 0.000f, 0.125f, 1.000f); // dark blue/purple
    // ImVec4 backgroundColor(0.000f, 0.000f, 0.000f, 1.000f); // black
#ifdef BUILD_FOR_WINDOWS
    auto wstr = HyoutaUtils::TextUtils::Utf8ToWString(windowTitle.data(), windowTitle.size());
    int rv = RunGuiDX11(state,
                        wstr ? wstr->c_str() : L"VodArchiver",
                        backgroundColor,
                        LoadFonts,
                        RenderFrame,
                        load_imgui_ini,
                        save_imgui_ini);
#else
    int rv = RunGuiGlfwVulkan(state,
                              windowTitle.data(),
                              backgroundColor,
                              LoadFonts,
                              RenderFrame,
                              load_imgui_ini,
                              save_imgui_ini);
#endif
    if (guiSettingsFolder) {
        HyoutaUtils::IO::CreateDirectory(std::string_view(*guiSettingsFolder));
        VodArchiver::WriteUserSettingsToIni(state.GuiSettings, userIniPath);
    }
    return rv;
}
} // namespace VodArchiver
