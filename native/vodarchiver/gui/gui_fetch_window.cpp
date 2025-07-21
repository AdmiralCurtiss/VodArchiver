#include "gui_fetch_window.h"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <format>
#include <string>
#include <thread>

#include "imgui.h"

#include "gui_state.h"
#include "util/scope.h"
#include "util/text.h"
#include "vodarchiver/job_handling.h"
#include "vodarchiver/userinfo/archive-org-user-info.h"
#include "vodarchiver/userinfo/ffmpeg-job-user-info.h"
#include "vodarchiver/userinfo/hitbox-user-info.h"
#include "vodarchiver/userinfo/rss-feed-user-info.h"
#include "vodarchiver/userinfo/twitch-user-info.h"
#include "vodarchiver/userinfo/youtube-channel-user-info.h"
#include "vodarchiver/userinfo/youtube-playlist-user-info.h"
#include "vodarchiver/userinfo/youtube-url-user-info.h"
#include "vodarchiver/userinfo/youtube-user-user-info.h"
#include "vodarchiver_imgui_utils.h"

namespace VodArchiver::GUI {
static FetchReturnValue
    RunFetchTask(JobConfig* jobConfig, IUserInfo* userInfo, size_t offset, bool flat) {
    if (userInfo == nullptr) {
        return FetchReturnValue{.Success = false, .HasMore = false};
    }
    try {
        return userInfo->Fetch(*jobConfig, offset, flat);
    } catch (...) {
        return FetchReturnValue{.Success = false, .HasMore = false};
    }
}

void FetchWindow::Cleanup(GuiState& state) {
    state.WindowIdsFetchWindow.ReturnId(WindowId);
}

FetchWindow::FetchWindow(GuiState& state)
  : WindowId(GenerateWindowId(state.WindowIdsFetchWindow,
                              WindowIdString.data(),
                              WindowIdString.size(),
                              WindowTitle,
                              sizeof(WindowTitle)))
  , FetchTask(RunFetchTask) {}

FetchWindow::~FetchWindow() = default;

bool FetchWindow::RenderFrame(GuiState& state) {
    ImGui::SetNextWindowSize(ImVec2(400.0f * state.CurrentDpi, 300.0f * state.CurrentDpi),
                             ImGuiCond_FirstUseEver);
    bool open = true;
    bool visible = ImGui::Begin(WindowIdString.data(), &open, ImGuiWindowFlags_None);
    auto windowScope = HyoutaUtils::MakeScopeGuard([&]() { ImGui::End(); });
    if (!visible) {
        return open || FetchTask.Engaged();
    }

    const bool alreadyEngaged = FetchTask.Engaged();
    auto fetchTaskRunningScope = ImGuiUtils::ConditionallyDisabledScope(alreadyEngaged);

    {
        std::lock_guard lock(state.UserInfosLock);
        auto itemsInListScope = ImGuiUtils::ConditionallyDisabledScope(!FetchedItems.empty());
        bool noPresetSelected = !(SelectedPreset < state.UserInfos.size());
        static constexpr char noPresetSelectedString[] = " == No Preset == ";
        std::string preview = noPresetSelected ? std::string(noPresetSelectedString)
                                               : state.UserInfos[SelectedPreset]->ToString();
        if (ImGui::BeginCombo("Presets", preview.c_str(), ImGuiComboFlags_PopupAlignLeft)) {
            {
                if (ImGui::Selectable(noPresetSelectedString, noPresetSelected))
                    SelectedPreset = std::numeric_limits<size_t>::max();
                if (noPresetSelected)
                    ImGui::SetItemDefaultFocus();
            }
            for (size_t i = 0; i < state.UserInfos.size(); ++i) {
                const bool is_selected = (SelectedPreset == i);
                std::string item_str = state.UserInfos[i]->ToString();
                if (ImGui::Selectable(item_str.c_str(), is_selected))
                    SelectedPreset = i;
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }
    {
        auto itemsInListScope = ImGuiUtils::ConditionallyDisabledScope(!FetchedItems.empty());
        std::string preview =
            SelectedService < static_cast<uint32_t>(ServiceVideoCategoryType::COUNT)
                ? std::string(ServiceVideoCategoryTypeToString(
                      static_cast<ServiceVideoCategoryType>(SelectedService)))
                : "?";
        if (ImGui::BeginCombo("Service", preview.c_str(), ImGuiComboFlags_PopupAlignLeft)) {
            for (uint32_t i = 0; i < static_cast<uint32_t>(ServiceVideoCategoryType::COUNT); ++i) {
                const bool is_selected = (SelectedService == i);
                std::string item_str = std::string(
                    ServiceVideoCategoryTypeToString(static_cast<ServiceVideoCategoryType>(i)));
                if (ImGui::Selectable(item_str.c_str(), is_selected))
                    SelectedService = i;
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }
    {
        auto itemsInListScope = ImGuiUtils::ConditionallyDisabledScope(!FetchedItems.empty());
        ImGui::InputText("Username", UsernameTextfield.data(), UsernameTextfield.size());
        ImGui::SameLine();
        ImGui::Checkbox("Save", &SaveCheckbox);
        ImGui::SameLine();
        ImGui::Checkbox("Flat", &FlatCheckbox);
    }

    {
        auto itemsInListScope =
            ImGuiUtils::ConditionallyDisabledScope(!(FetchedItems.empty() || FetchHasMore));
        ImGui::SameLine();
        if (ImGui::Button(FetchHasMore ? "Fetch More" : "Fetch") && !FetchTask.Engaged()) {
            std::lock_guard lock(state.UserInfosLock);
            IUserInfo* userInfoToFetch;
            if (FetchTaskActiveUserInfo) {
                userInfoToFetch = FetchTaskActiveUserInfo.get();
            } else if (SelectedPreset < state.UserInfos.size()) {
                // FIXME: This goes out of the lock.
                userInfoToFetch = state.UserInfos[SelectedPreset].get();
            } else {
                std::unique_ptr<IUserInfo> tmp;
                switch (static_cast<ServiceVideoCategoryType>(SelectedService)) {
                    case ServiceVideoCategoryType::TwitchRecordings:
                        tmp = std::make_unique<TwitchUserInfo>(
                            std::string(HyoutaUtils::TextUtils::StripToNull(UsernameTextfield)),
                            false);
                        break;
                    case ServiceVideoCategoryType::TwitchHighlights:
                        tmp = std::make_unique<TwitchUserInfo>(
                            std::string(HyoutaUtils::TextUtils::StripToNull(UsernameTextfield)),
                            true);
                        break;
                    case ServiceVideoCategoryType::HitboxRecordings:
                        tmp = std::make_unique<HitboxUserInfo>(
                            std::string(HyoutaUtils::TextUtils::StripToNull(UsernameTextfield)));
                        break;
                    case ServiceVideoCategoryType::YoutubeUser:
                        tmp = std::make_unique<YoutubeUserUserInfo>(
                            std::string(HyoutaUtils::TextUtils::StripToNull(UsernameTextfield)));
                        break;
                    case ServiceVideoCategoryType::YoutubeChannel:
                        tmp = std::make_unique<YoutubeChannelUserInfo>(
                            std::string(HyoutaUtils::TextUtils::StripToNull(UsernameTextfield)));
                        break;
                    case ServiceVideoCategoryType::YoutubePlaylist:
                        tmp = std::make_unique<YoutubePlaylistUserInfo>(
                            std::string(HyoutaUtils::TextUtils::StripToNull(UsernameTextfield)));
                        break;
                    case ServiceVideoCategoryType::RssFeed:
                        tmp = std::make_unique<RssFeedUserInfo>(
                            std::string(HyoutaUtils::TextUtils::StripToNull(UsernameTextfield)));
                        break;
                    case ServiceVideoCategoryType::FFMpegJob:
                        tmp = std::make_unique<FFMpegJobUserInfo>(
                            std::string(HyoutaUtils::TextUtils::StripToNull(UsernameTextfield)),
                            std::string());
                        break;
                    case ServiceVideoCategoryType::ArchiveOrg:
                        tmp = std::make_unique<ArchiveOrgUserInfo>(
                            std::string(HyoutaUtils::TextUtils::StripToNull(UsernameTextfield)));
                        break;
                    case ServiceVideoCategoryType::YoutubeUrl:
                        tmp = std::make_unique<YoutubeUrlUserInfo>(
                            std::string(HyoutaUtils::TextUtils::StripToNull(UsernameTextfield)));
                        break;
                    default: break;
                }
                userInfoToFetch = tmp.get();
                FetchTaskActiveUserInfo = std::move(tmp);
            }

            FetchTask.Engage(&state.JobConf, userInfoToFetch, FetchTaskOffset, FlatCheckbox);
        }
    }

    if (FetchTask.ResultAvailable()) {
        auto rv = FetchTask.FetchResultAndDisengage();
        if (rv.Success && !rv.Videos.empty()) {
            FetchTaskOffset += rv.VideoCountThisFetch;
            for (auto& v : rv.Videos) {
                FetchedItems.push_back(std::move(v));
            }
            FetchHasMore = rv.HasMore;
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Clear") && !FetchTask.Engaged()) {
        FetchedItems.clear();
        FetchTaskActiveUserInfo.reset();
        FetchTaskOffset = 0;
        FetchHasMore = false;
    }

    {
        static constexpr ImGuiTableFlags flags =
            ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable
            | ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti | ImGuiTableFlags_RowBg
            | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY
            | ImGuiTableFlags_SizingFixedFit;
        static constexpr ImGuiTableColumnFlags columns_base_flags = ImGuiTableColumnFlags_None;

        enum ColumnIDs {
            ColumnID_Service = 0,
            ColumnID_VideoID,
            ColumnID_Username,
            ColumnID_Title,
            ColumnID_Game,
            ColumnID_Timestamp,
            ColumnID_Duration,
            ColumnID_RecordingState,
            ColumnID_Type,
            ColumnID_Actions,
            ColumnIDCount
        };
        int items_count = static_cast<int>(FetchedItems.size());

        // Update item list if we changed the number of items
        ImVector<int>& items = this->Items;
        ImVector<int>& selection = this->Selection;
        bool& items_need_sort = this->ItemsNeedSort;
        if (items.Size != items_count) {
            items.resize(items_count);
            for (int n = 0; n < items_count; ++n) {
                items[n] = n;
            }
        }

        // Submit table
        if (ImGui::BeginTable("JobTable", ColumnIDCount, flags, ImVec2(0.0f, -28.0f), 0.0f)) {
            // Declare columns
            ImGui::TableSetupColumn("Service",
                                    columns_base_flags | ImGuiTableColumnFlags_WidthFixed,
                                    0.0f,
                                    ColumnID_Service);
            ImGui::TableSetupColumn("Video ID", columns_base_flags, 0.0f, ColumnID_VideoID);
            ImGui::TableSetupColumn("Username", columns_base_flags, 0.0f, ColumnID_Username);
            ImGui::TableSetupColumn("Title", columns_base_flags, 0.0f, ColumnID_Title);
            ImGui::TableSetupColumn("Game", columns_base_flags, 0.0f, ColumnID_Game);
            ImGui::TableSetupColumn("Timestamp", columns_base_flags, 0.0f, ColumnID_Timestamp);
            ImGui::TableSetupColumn("Duration", columns_base_flags, 0.0f, ColumnID_Duration);
            ImGui::TableSetupColumn(
                "Recording State", columns_base_flags, 0.0f, ColumnID_RecordingState);
            ImGui::TableSetupColumn("Type", columns_base_flags, 0.0f, ColumnID_Type);
            ImGui::TableSetupColumn("Actions", columns_base_flags, 0.0f, ColumnID_Actions);
            ImGui::TableSetupScrollFreeze(0, 1);

            // Sort our data if sort specs have been changed!
            ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs();
            if (sort_specs && sort_specs->SpecsDirty)
                items_need_sort = true;
            if (sort_specs && items_need_sort && items.Size > 1) {
                for (int spec = (sort_specs->SpecsCount - 1); spec >= 0; --spec) {
                    switch (sort_specs->Specs[spec].ColumnUserID) {
                        case ColumnID_Service:
                            if (sort_specs->Specs[spec].SortDirection
                                == ImGuiSortDirection_Ascending) {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoInfo* l = FetchedItems[lhs].get();
                                        IVideoInfo* r = FetchedItems[rhs].get();
                                        return l->GetService() < r->GetService();
                                    });
                            } else {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoInfo* l = FetchedItems[lhs].get();
                                        IVideoInfo* r = FetchedItems[rhs].get();
                                        return l->GetService() > r->GetService();
                                    });
                            }
                            break;
                        case ColumnID_VideoID:
                            if (sort_specs->Specs[spec].SortDirection
                                == ImGuiSortDirection_Ascending) {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoInfo* l = FetchedItems[lhs].get();
                                        IVideoInfo* r = FetchedItems[rhs].get();
                                        return l->GetVideoId() < r->GetVideoId();
                                    });
                            } else {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoInfo* l = FetchedItems[lhs].get();
                                        IVideoInfo* r = FetchedItems[rhs].get();
                                        return l->GetVideoId() > r->GetVideoId();
                                    });
                            }
                            break;
                        case ColumnID_Username:
                            if (sort_specs->Specs[spec].SortDirection
                                == ImGuiSortDirection_Ascending) {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoInfo* l = FetchedItems[lhs].get();
                                        IVideoInfo* r = FetchedItems[rhs].get();
                                        return l->GetUsername() < r->GetUsername();
                                    });
                            } else {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoInfo* l = FetchedItems[lhs].get();
                                        IVideoInfo* r = FetchedItems[rhs].get();
                                        return l->GetUsername() > r->GetUsername();
                                    });
                            }
                            break;
                        case ColumnID_Title:
                            if (sort_specs->Specs[spec].SortDirection
                                == ImGuiSortDirection_Ascending) {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoInfo* l = FetchedItems[lhs].get();
                                        IVideoInfo* r = FetchedItems[rhs].get();
                                        return l->GetVideoTitle() < r->GetVideoTitle();
                                    });
                            } else {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoInfo* l = FetchedItems[lhs].get();
                                        IVideoInfo* r = FetchedItems[rhs].get();
                                        return l->GetVideoTitle() > r->GetVideoTitle();
                                    });
                            }
                            break;
                        case ColumnID_Game:
                            if (sort_specs->Specs[spec].SortDirection
                                == ImGuiSortDirection_Ascending) {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoInfo* l = FetchedItems[lhs].get();
                                        IVideoInfo* r = FetchedItems[rhs].get();
                                        return l->GetVideoGame() < r->GetVideoGame();
                                    });
                            } else {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoInfo* l = FetchedItems[lhs].get();
                                        IVideoInfo* r = FetchedItems[rhs].get();
                                        return l->GetVideoGame() > r->GetVideoGame();
                                    });
                            }
                            break;
                        case ColumnID_Timestamp:
                            if (sort_specs->Specs[spec].SortDirection
                                == ImGuiSortDirection_Ascending) {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoInfo* l = FetchedItems[lhs].get();
                                        IVideoInfo* r = FetchedItems[rhs].get();
                                        return l->GetVideoTimestamp() < r->GetVideoTimestamp();
                                    });
                            } else {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoInfo* l = FetchedItems[lhs].get();
                                        IVideoInfo* r = FetchedItems[rhs].get();
                                        return l->GetVideoTimestamp() > r->GetVideoTimestamp();
                                    });
                            }
                            break;
                        case ColumnID_Duration:
                            if (sort_specs->Specs[spec].SortDirection
                                == ImGuiSortDirection_Ascending) {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoInfo* l = FetchedItems[lhs].get();
                                        IVideoInfo* r = FetchedItems[rhs].get();
                                        return l->GetVideoLength() < r->GetVideoLength();
                                    });
                            } else {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoInfo* l = FetchedItems[lhs].get();
                                        IVideoInfo* r = FetchedItems[rhs].get();
                                        return l->GetVideoLength() > r->GetVideoLength();
                                    });
                            }
                            break;
                        case ColumnID_RecordingState:
                            if (sort_specs->Specs[spec].SortDirection
                                == ImGuiSortDirection_Ascending) {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoInfo* l = FetchedItems[lhs].get();
                                        IVideoInfo* r = FetchedItems[rhs].get();
                                        return l->GetVideoRecordingState()
                                               < r->GetVideoRecordingState();
                                    });
                            } else {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoInfo* l = FetchedItems[lhs].get();
                                        IVideoInfo* r = FetchedItems[rhs].get();
                                        return l->GetVideoRecordingState()
                                               > r->GetVideoRecordingState();
                                    });
                            }
                            break;
                        case ColumnID_Type:
                            if (sort_specs->Specs[spec].SortDirection
                                == ImGuiSortDirection_Ascending) {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoInfo* l = FetchedItems[lhs].get();
                                        IVideoInfo* r = FetchedItems[rhs].get();
                                        return l->GetVideoType() < r->GetVideoType();
                                    });
                            } else {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoInfo* l = FetchedItems[lhs].get();
                                        IVideoInfo* r = FetchedItems[rhs].get();
                                        return l->GetVideoType() > r->GetVideoType();
                                    });
                            }
                            break;
                        default: break;
                    }
                }
                sort_specs->SpecsDirty = false;
            }
            items_need_sort = false;

            ImGui::TableHeadersRow();

            ImGuiListClipper clipper;
            clipper.Begin(items.Size);
            while (clipper.Step()) {
                for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; ++row_n) {
                    const int ID = items[row_n];
                    IVideoInfo* item = FetchedItems[ID].get();
                    // if (!filter.PassFilter(item->Name))
                    //     continue;

                    const bool item_is_selected = selection.contains(ID);
                    ImGui::PushID(ID);
                    ImGui::TableNextRow(ImGuiTableRowFlags_None, 0.0f);

                    if (ImGui::TableSetColumnIndex(ColumnID_Service)) {
                        std::string_view sv = StreamServiceToString(item->GetService());
                        ImGui::TextUnformatted(sv.data(), sv.data() + sv.size());
                    }
                    if (ImGui::TableSetColumnIndex(ColumnID_VideoID)) {
                        std::string s = item->GetVideoId();
                        ImGui::TextUnformatted(s.data(), s.data() + s.size());
                    }
                    if (ImGui::TableSetColumnIndex(ColumnID_Username)) {
                        std::string s = item->GetUsername();
                        ImGui::TextUnformatted(s.data(), s.data() + s.size());
                    }
                    if (ImGui::TableSetColumnIndex(ColumnID_Title)) {
                        std::string s = item->GetVideoTitle();
                        ImGui::TextUnformatted(s.data(), s.data() + s.size());
                    }
                    if (ImGui::TableSetColumnIndex(ColumnID_Game)) {
                        std::string s = item->GetVideoGame();
                        ImGui::TextUnformatted(s.data(), s.data() + s.size());
                    }
                    if (ImGui::TableSetColumnIndex(ColumnID_Timestamp)) {
                        std::string s = DateTimeToStringForGui(item->GetVideoTimestamp());
                        ImGui::TextUnformatted(s.data(), s.data() + s.size());
                    }
                    if (ImGui::TableSetColumnIndex(ColumnID_Duration)) {
                        std::string s = std::format("{}", item->GetVideoLength().GetTotalSeconds());
                        ImGui::TextUnformatted(s.data(), s.data() + s.size());
                    }
                    if (ImGui::TableSetColumnIndex(ColumnID_RecordingState)) {
                        std::string_view s = RecordingStateToString(item->GetVideoRecordingState());
                        ImGui::TextUnformatted(s.data(), s.data() + s.size());
                    }
                    if (ImGui::TableSetColumnIndex(ColumnID_Type)) {
                        std::string_view s = VideoFileTypeToString(item->GetVideoType());
                        ImGui::TextUnformatted(s.data(), s.data() + s.size());
                    }
                    if (ImGui::TableSetColumnIndex(ColumnID_Actions)) {
                        if (ImGui::Button("Download")) {
                            CreateAndEnqueueJob(state.Jobs, item->Clone(), [&](IVideoJob* newJob) {
                                AddJobToTaskGroupIfAutoenqueue(state.VideoTaskGroups, newJob);
                            });
                        }
                    }

                    ImGui::PopID();
                }
            }

            ImGui::EndTable();
        }
    }

    {
        std::lock_guard lock(state.UserInfosLock);
        bool autoDownloadFakeFlag = false;
        bool* autoDownloadFlagPtr = &autoDownloadFakeFlag;
        auto autoDownloadDisabledScope =
            HyoutaUtils::MakeDisposableScopeGuard([&]() { ImGui::EndDisabled(); });
        if (SelectedPreset < state.UserInfos.size()) {
            autoDownloadDisabledScope.Dispose();
            autoDownloadFlagPtr = &(state.UserInfos[SelectedPreset]->AutoDownload);
        } else {
            ImGui::BeginDisabled();
        }
        ImGui::Checkbox("Auto Download this Preset", autoDownloadFlagPtr);
    }

    ImGui::SameLine();
    if (ImGuiUtils::ButtonRightAlign("Download Fetched")) {
        for (auto& f : FetchedItems) {
            CreateAndEnqueueJob(state.Jobs, f->Clone(), [&](IVideoJob* newJob) {
                AddJobToTaskGroupIfAutoenqueue(state.VideoTaskGroups, newJob);
            });
        }
    }

    return open || FetchTask.Engaged();
}
} // namespace VodArchiver::GUI
