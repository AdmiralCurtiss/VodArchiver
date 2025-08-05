#include "gui_vodarchiver_main_window.h"

#include <algorithm>
#include <array>
#include <cassert>
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

#include "gui_fetch_window.h"
#include "gui_log_window.h"
#include "gui_settings_window.h"
#include "gui_state.h"
#include "gui_user_settings.h"
#include "gui_video_split_window.h"
#include "util/file.h"
#include "util/hash/sha1.h"
#include "util/scope.h"
#include "util/system.h"
#include "util/text.h"
#include "vodarchiver/common_paths.h"
#include "vodarchiver/videojobs/i-video-job.h"
#include "vodarchiver_imgui_utils.h"
#include "vodarchiver_version.h"

namespace VodArchiver::GUI {
VodArchiverMainWindow::VodArchiverMainWindow() {
    // turn off finished/dead by default
    FilterJobStatus |= static_cast<uint32_t>(1u << static_cast<uint32_t>(VideoJobStatus::Finished));
    FilterJobStatus |= static_cast<uint32_t>(1u << static_cast<uint32_t>(VideoJobStatus::Dead));
}

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
            if (ImGui::MenuItem("Fetch videos...")) {
                state.Windows.emplace_back(std::make_unique<GUI::FetchWindow>(state));
            }
            if (ImGui::MenuItem("Split videos...")) {
                state.Windows.emplace_back(std::make_unique<GUI::VideoSplitWindow>(state));
            }
            if (ImGui::MenuItem("Settings...")) {
                state.Windows.emplace_back(std::make_unique<GUI::SettingsWindow>(state));
            }
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

    if (ImGui::Button("Filter...##FilterSettingsPopupButton")) {
        ImGui::OpenPopup("FilterSettingsPopup");
    }
    if (ImGui::BeginPopup("FilterSettingsPopup")) {
        ImGui::PushID("StreamServiceFilterOptions");
        for (uint32_t i = 0; i < static_cast<uint32_t>(StreamService::COUNT); ++i) {
            ImGui::PushID(i);
            std::string_view ss = StreamServiceToString(static_cast<StreamService>(i));
            bool previousState = (FilterStreamService & (1u << i)) == 0;
            bool newState = previousState;
            ImGui::Selectable(ss.data(), &newState, ImGuiSelectableFlags_DontClosePopups);
            if (previousState != newState) {
                if (newState) {
                    FilterStreamService &= ~static_cast<uint32_t>(1u << i);
                } else {
                    FilterStreamService |= static_cast<uint32_t>(1u << i);
                }
                ItemsNeedFilter = true;
            }
            ImGui::PopID();
        }
        ImGui::PopID();
        ImGui::Separator();
        ImGui::PushID("JobStatusFilterOptions");
        for (uint32_t i = 0; i < static_cast<uint32_t>(VideoJobStatus::COUNT); ++i) {
            ImGui::PushID(i);
            std::string_view ss = VideoJobStatusToString(static_cast<VideoJobStatus>(i));
            bool previousState = (FilterJobStatus & (1u << i)) == 0;
            bool newState = previousState;
            ImGui::Selectable(ss.data(), &newState, ImGuiSelectableFlags_DontClosePopups);
            if (previousState != newState) {
                if (newState) {
                    FilterJobStatus &= ~static_cast<uint32_t>(1u << i);
                } else {
                    FilterJobStatus |= static_cast<uint32_t>(1u << i);
                }
                ItemsNeedFilter = true;
            }
            ImGui::PopID();
        }
        ImGui::PopID();

        ImGui::EndPopup();
    }

    ImGui::SameLine();

    if (ImGui::InputText("##FilterString", FilterTextfield.data(), FilterTextfield.size())) {
        ItemsNeedFilter = true;
    }

    ImGui::Spacing();

    auto disabledScope = HyoutaUtils::MakeDisposableScopeGuard([&]() { ImGui::EndDisabled(); });
    if (HasPendingWindowRequest()) {
        ImGui::BeginDisabled();
    } else {
        disabledScope.Dispose();
    }

    {
        std::lock_guard lock(state.Jobs.JobsLock);

        static constexpr ImGuiTableFlags flags =
            ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable
            | ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti | ImGuiTableFlags_RowBg
            | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY
            | ImGuiTableFlags_SizingFixedFit;
        static constexpr ImGuiTableColumnFlags columns_base_flags = ImGuiTableColumnFlags_None;

        enum ColumnIDs {
            ColumnID_Index = 0,
            ColumnID_Valid,
            ColumnID_Service,
            ColumnID_VideoID,
            ColumnID_Username,
            ColumnID_Title,
            ColumnID_Game,
            ColumnID_Timestamp,
            ColumnID_Duration,
            ColumnID_RecordingState,
            ColumnID_Notes,
            ColumnID_Status,
            ColumnID_Actions,
            ColumnIDCount
        };

        // Submit table
        if (ImGui::BeginTable("JobTable", ColumnIDCount, flags, ImVec2(0.0f, -28.0f), 0.0f)) {
            // Declare columns
            ImGui::TableSetupColumn("Index",
                                    columns_base_flags | ImGuiTableColumnFlags_DefaultSort
                                        | ImGuiTableColumnFlags_WidthFixed
                                        | ImGuiTableColumnFlags_NoHide,
                                    0.0f,
                                    ColumnID_Index);
            ImGui::TableSetupColumn("Valid",
                                    columns_base_flags | ImGuiTableColumnFlags_WidthFixed,
                                    0.0f,
                                    ColumnID_Valid);
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
            ImGui::TableSetupColumn("Notes", columns_base_flags, 0.0f, ColumnID_Notes);
            ImGui::TableSetupColumn("Status", columns_base_flags, 0.0f, ColumnID_Status);
            ImGui::TableSetupColumn("Actions", columns_base_flags, 0.0f, ColumnID_Actions);
            ImGui::TableSetupScrollFreeze(0, 1);

            // Update item list if we changed the number of items.
            // We also need to rebuild from scratch if the filter changed, since the list may now
            // include items that were previously filtered out.
            size_t items_count = state.Jobs.JobsVector.size();
            std::vector<uint32_t>& items = ItemIndices;
            if (ItemCountLastSeen != items_count || ItemsNeedFilter) {
                items.resize(items_count);
                for (size_t n = 0; n < items_count; ++n) {
                    items[n] = static_cast<uint32_t>(n);
                }
                ItemCountLastSeen = items_count;
                ItemsNeedSort = true;
                ItemsNeedFilter = true;
            }

            if (ItemsNeedFilter) {
                // filtering happens in-place, this is safe because out_idx is always <= i
                size_t out_idx = 0;
                for (size_t i = 0; i < items.size(); ++i) {
                    const uint32_t ID = items[i];
                    IVideoJob* item = state.Jobs.JobsVector[ID].get();
                    if (PassFilter(item)) {
                        items[out_idx] = ID;
                        ++out_idx;
                    }
                }
                items.resize(out_idx);

                ItemsNeedFilter = false;
            }

            // Sort our data if sort specs have been changed!
            ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs();
            if (sort_specs && sort_specs->SpecsDirty) {
                ItemsNeedSort = true;
            }
            if (sort_specs && ItemsNeedSort && items.size() > 1) {
                for (int spec = (sort_specs->SpecsCount - 1); spec >= 0; --spec) {
                    switch (sort_specs->Specs[spec].ColumnUserID) {
                        case ColumnID_Index:
                            if (sort_specs->Specs[spec].SortDirection
                                == ImGuiSortDirection_Ascending) {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        return lhs < rhs;
                                    });
                            } else {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        return lhs > rhs;
                                    });
                            }
                            break;
                        case ColumnID_Service:
                            if (sort_specs->Specs[spec].SortDirection
                                == ImGuiSortDirection_Ascending) {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoJob* l = state.Jobs.JobsVector[lhs].get();
                                        IVideoJob* r = state.Jobs.JobsVector[rhs].get();
                                        return l->VideoInfo->GetService()
                                               < r->VideoInfo->GetService();
                                    });
                            } else {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoJob* l = state.Jobs.JobsVector[lhs].get();
                                        IVideoJob* r = state.Jobs.JobsVector[rhs].get();
                                        return l->VideoInfo->GetService()
                                               > r->VideoInfo->GetService();
                                    });
                            }
                            break;
                        case ColumnID_VideoID:
                            if (sort_specs->Specs[spec].SortDirection
                                == ImGuiSortDirection_Ascending) {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoJob* l = state.Jobs.JobsVector[lhs].get();
                                        IVideoJob* r = state.Jobs.JobsVector[rhs].get();
                                        std::array<char, 256> buffer1;
                                        std::array<char, 256> buffer2;
                                        return l->VideoInfo->GetVideoId(buffer1)
                                               < r->VideoInfo->GetVideoId(buffer2);
                                    });
                            } else {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoJob* l = state.Jobs.JobsVector[lhs].get();
                                        IVideoJob* r = state.Jobs.JobsVector[rhs].get();
                                        std::array<char, 256> buffer1;
                                        std::array<char, 256> buffer2;
                                        return l->VideoInfo->GetVideoId(buffer1)
                                               > r->VideoInfo->GetVideoId(buffer2);
                                    });
                            }
                            break;
                        case ColumnID_Username:
                            if (sort_specs->Specs[spec].SortDirection
                                == ImGuiSortDirection_Ascending) {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoJob* l = state.Jobs.JobsVector[lhs].get();
                                        IVideoJob* r = state.Jobs.JobsVector[rhs].get();
                                        std::array<char, 256> buffer1;
                                        std::array<char, 256> buffer2;
                                        return l->VideoInfo->GetUsername(buffer1)
                                               < r->VideoInfo->GetUsername(buffer2);
                                    });
                            } else {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoJob* l = state.Jobs.JobsVector[lhs].get();
                                        IVideoJob* r = state.Jobs.JobsVector[rhs].get();
                                        std::array<char, 256> buffer1;
                                        std::array<char, 256> buffer2;
                                        return l->VideoInfo->GetUsername(buffer1)
                                               > r->VideoInfo->GetUsername(buffer2);
                                    });
                            }
                            break;
                        case ColumnID_Title:
                            if (sort_specs->Specs[spec].SortDirection
                                == ImGuiSortDirection_Ascending) {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoJob* l = state.Jobs.JobsVector[lhs].get();
                                        IVideoJob* r = state.Jobs.JobsVector[rhs].get();
                                        std::array<char, 256> buffer1;
                                        std::array<char, 256> buffer2;
                                        return l->VideoInfo->GetVideoTitle(buffer1)
                                               < r->VideoInfo->GetVideoTitle(buffer2);
                                    });
                            } else {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoJob* l = state.Jobs.JobsVector[lhs].get();
                                        IVideoJob* r = state.Jobs.JobsVector[rhs].get();
                                        std::array<char, 256> buffer1;
                                        std::array<char, 256> buffer2;
                                        return l->VideoInfo->GetVideoTitle(buffer1)
                                               > r->VideoInfo->GetVideoTitle(buffer2);
                                    });
                            }
                            break;
                        case ColumnID_Game:
                            if (sort_specs->Specs[spec].SortDirection
                                == ImGuiSortDirection_Ascending) {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoJob* l = state.Jobs.JobsVector[lhs].get();
                                        IVideoJob* r = state.Jobs.JobsVector[rhs].get();
                                        std::array<char, 256> buffer1;
                                        std::array<char, 256> buffer2;
                                        return l->VideoInfo->GetVideoGame(buffer1)
                                               < r->VideoInfo->GetVideoGame(buffer2);
                                    });
                            } else {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoJob* l = state.Jobs.JobsVector[lhs].get();
                                        IVideoJob* r = state.Jobs.JobsVector[rhs].get();
                                        std::array<char, 256> buffer1;
                                        std::array<char, 256> buffer2;
                                        return l->VideoInfo->GetVideoGame(buffer1)
                                               > r->VideoInfo->GetVideoGame(buffer2);
                                    });
                            }
                            break;
                        case ColumnID_Timestamp:
                            if (sort_specs->Specs[spec].SortDirection
                                == ImGuiSortDirection_Ascending) {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoJob* l = state.Jobs.JobsVector[lhs].get();
                                        IVideoJob* r = state.Jobs.JobsVector[rhs].get();
                                        return l->VideoInfo->GetVideoTimestamp()
                                               < r->VideoInfo->GetVideoTimestamp();
                                    });
                            } else {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoJob* l = state.Jobs.JobsVector[lhs].get();
                                        IVideoJob* r = state.Jobs.JobsVector[rhs].get();
                                        return l->VideoInfo->GetVideoTimestamp()
                                               > r->VideoInfo->GetVideoTimestamp();
                                    });
                            }
                            break;
                        case ColumnID_Duration:
                            if (sort_specs->Specs[spec].SortDirection
                                == ImGuiSortDirection_Ascending) {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoJob* l = state.Jobs.JobsVector[lhs].get();
                                        IVideoJob* r = state.Jobs.JobsVector[rhs].get();
                                        return l->VideoInfo->GetVideoLength()
                                               < r->VideoInfo->GetVideoLength();
                                    });
                            } else {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoJob* l = state.Jobs.JobsVector[lhs].get();
                                        IVideoJob* r = state.Jobs.JobsVector[rhs].get();
                                        return l->VideoInfo->GetVideoLength()
                                               > r->VideoInfo->GetVideoLength();
                                    });
                            }
                            break;
                        case ColumnID_RecordingState:
                            if (sort_specs->Specs[spec].SortDirection
                                == ImGuiSortDirection_Ascending) {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoJob* l = state.Jobs.JobsVector[lhs].get();
                                        IVideoJob* r = state.Jobs.JobsVector[rhs].get();
                                        return l->VideoInfo->GetVideoRecordingState()
                                               < r->VideoInfo->GetVideoRecordingState();
                                    });
                            } else {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoJob* l = state.Jobs.JobsVector[lhs].get();
                                        IVideoJob* r = state.Jobs.JobsVector[rhs].get();
                                        return l->VideoInfo->GetVideoRecordingState()
                                               > r->VideoInfo->GetVideoRecordingState();
                                    });
                            }
                            break;
                        case ColumnID_Notes:
                            if (sort_specs->Specs[spec].SortDirection
                                == ImGuiSortDirection_Ascending) {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoJob* l = state.Jobs.JobsVector[lhs].get();
                                        IVideoJob* r = state.Jobs.JobsVector[rhs].get();
                                        return l->Notes < r->Notes;
                                    });
                            } else {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoJob* l = state.Jobs.JobsVector[lhs].get();
                                        IVideoJob* r = state.Jobs.JobsVector[rhs].get();
                                        return l->Notes > r->Notes;
                                    });
                            }
                            break;
                        case ColumnID_Status:
                            if (sort_specs->Specs[spec].SortDirection
                                == ImGuiSortDirection_Ascending) {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoJob* l = state.Jobs.JobsVector[lhs].get();
                                        IVideoJob* r = state.Jobs.JobsVector[rhs].get();
                                        return l->JobStatus < r->JobStatus;
                                    });
                            } else {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoJob* l = state.Jobs.JobsVector[lhs].get();
                                        IVideoJob* r = state.Jobs.JobsVector[rhs].get();
                                        return l->JobStatus > r->JobStatus;
                                    });
                            }
                            break;
                        default: break;
                    }
                }
                sort_specs->SpecsDirty = false;
                ItemsNeedSort = false;
            }

            ImGui::TableHeadersRow();

            ImGuiListClipper clipper;
            clipper.Begin(items.size());
            while (clipper.Step()) {
                for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; ++row_n) {
                    const uint32_t ID = items[row_n];
                    IVideoJob* item = state.Jobs.JobsVector[ID].get();

                    ImGui::PushID(ID);
                    ImGui::TableNextRow(ImGuiTableRowFlags_None, 0.0f);

                    if (ImGui::TableSetColumnIndex(ColumnID_Index)) {
                        ImGui::Text("%d", ID);
                    }
                    if (ImGui::TableSetColumnIndex(ColumnID_Valid)) {
                        ImGui::Text("%s", item->HasBeenValidated ? "true" : "false");
                    }
                    if (ImGui::TableSetColumnIndex(ColumnID_Service)) {
                        std::string_view sv = StreamServiceToString(item->VideoInfo->GetService());
                        ImGui::TextUnformatted(sv.data(), sv.data() + sv.size());
                    }
                    if (ImGui::TableSetColumnIndex(ColumnID_VideoID)) {
                        std::array<char, 256> buffer;
                        std::string_view s = item->VideoInfo->GetVideoId(buffer);
                        ImGui::TextUnformatted(s.data(), s.data() + s.size());
                    }
                    if (ImGui::TableSetColumnIndex(ColumnID_Username)) {
                        std::array<char, 256> buffer;
                        std::string_view s = item->VideoInfo->GetUsername(buffer);
                        ImGui::TextUnformatted(s.data(), s.data() + s.size());
                    }
                    if (ImGui::TableSetColumnIndex(ColumnID_Title)) {
                        std::array<char, 256> buffer;
                        std::string_view s = item->VideoInfo->GetVideoTitle(buffer);
                        ImGui::TextUnformatted(s.data(), s.data() + s.size());
                    }
                    if (ImGui::TableSetColumnIndex(ColumnID_Game)) {
                        std::array<char, 256> buffer;
                        std::string_view s = item->VideoInfo->GetVideoGame(buffer);
                        ImGui::TextUnformatted(s.data(), s.data() + s.size());
                    }
                    if (ImGui::TableSetColumnIndex(ColumnID_Timestamp)) {
                        std::array<char, 24> buffer;
                        std::string_view s =
                            DateTimeToStringForGui(item->VideoInfo->GetVideoTimestamp(), buffer);
                        ImGui::TextUnformatted(s.data(), s.data() + s.size());
                    }
                    if (ImGui::TableSetColumnIndex(ColumnID_Duration)) {
                        std::array<char, 24> buffer;
                        std::string_view s =
                            TimeSpanToStringForGui(item->VideoInfo->GetVideoLength(), buffer);
                        ImGui::TextUnformatted(s.data(), s.data() + s.size());
                    }
                    if (ImGui::TableSetColumnIndex(ColumnID_RecordingState)) {
                        std::string_view s =
                            RecordingStateToString(item->VideoInfo->GetVideoRecordingState());
                        ImGui::TextUnformatted(s.data(), s.data() + s.size());
                    }
                    if (ImGui::TableSetColumnIndex(ColumnID_Notes)) {
                        const std::string& s = item->Notes;
                        ImGui::TextUnformatted(s.data(), s.data() + s.size());
                    }
                    if (ImGui::TableSetColumnIndex(ColumnID_Status)) {
                        const std::string& s = item->TextStatus;
                        ImGui::TextUnformatted(s.data(), s.data() + s.size());
                    }
                    if (ImGui::TableSetColumnIndex(ColumnID_Actions)) {
                        if (ImGui::SmallButton("...##JobActionsPopupButton")) {
                            ImGui::OpenPopup("JobActionsPopup");
                        }
                        if (ImGui::BeginPopup("JobActionsPopup")) {
                            auto service = item->VideoInfo->GetService();
                            auto is_in_queue = [&]() -> bool {
                                for (auto& g : state.VideoTaskGroups) {
                                    if (g->GetService() == service) {
                                        return g->IsInQueue(item);
                                    }
                                }
                                return false;
                            };

                            IUserInputRequest* uir = item->GetUserInputRequest();
                            if (uir) {
                                if (ImGui::BeginMenu(uir->GetQuestion().c_str())) {
                                    for (auto& option : uir->GetOptions()) {
                                        if (ImGui::MenuItem(option.c_str())) {
                                            uir->SelectOption(option);
                                        }
                                    }
                                    ImGui::EndMenu();
                                }
                            }

                            if (item->JobStatus == VideoJobStatus::NotStarted
                                || item->JobStatus == VideoJobStatus::Dead) {
                                if (is_in_queue()) {
                                    if (ImGui::Selectable("Dequeue")) {
                                        for (auto& g : state.VideoTaskGroups) {
                                            if (g->GetService() == service) {
                                                g->Dequeue(item);
                                                break;
                                            }
                                        }
                                    }
                                } else {
                                    if (ImGui::Selectable("Enqueue")) {
                                        for (auto& g : state.VideoTaskGroups) {
                                            if (g->GetService() == service) {
                                                g->Enqueue(item);
                                                break;
                                            }
                                        }
                                    }
                                }
                                if (ImGui::Selectable("Download now")) {
                                    for (auto& g : state.VideoTaskGroups) {
                                        if (g->GetService() == service) {
                                            g->Enqueue(item, true);
                                            break;
                                        }
                                    }
                                }
                            }
                            if (ImGui::Selectable("Copy Video ID")) {
                                std::array<char, 256> buffer;
                                std::string_view id = item->VideoInfo->GetVideoId(buffer);
                                ImGui::SetClipboardText(id.data());
                            }
                            if (ImGui::Selectable("Copy Output Filename")) {
                                std::string fn = item->GenerateOutputFilename();
                                ImGui::SetClipboardText(fn.c_str());
                            }
                            if (ImGui::Selectable("Copy Status")) {
                                const std::string& status = item->TextStatus;
                                ImGui::SetClipboardText(status.c_str());
                            }
                            if (item->JobStatus == VideoJobStatus::Running) {
                                if (ImGui::Selectable("Stop")) {
                                    for (auto& g : state.VideoTaskGroups) {
                                        if (g->GetService() == service) {
                                            g->CancelJob(item);
                                            break;
                                        }
                                    }
                                }
                            }
                            if (item->JobStatus == VideoJobStatus::NotStarted) {
                                if (ImGui::Selectable("Kill")) {
                                    item->JobStatus = VideoJobStatus::Dead;
                                    item->TextStatus = "[Manually killed] " + item->TextStatus;
                                }
                            }
                            if (item->JobStatus != VideoJobStatus::Running) {
                                if (ImGui::Selectable("Remove")) {
                                    // TODO: Figure out a way to do this thread-safely...
                                }
                            }
                            ImGui::EndPopup();
                        }
                    }

                    ImGui::PopID();
                }
            }

            ImGui::EndTable();
        }
    }

    // bottom bar

    if (ImGui::Button("Log")) {
        std::string msg;
        {
            std::lock_guard lock(state.FetchTaskStatusMessageLock);
            msg = state.FetchTaskStatusMessages;
        }
        state.Windows.emplace_back(std::make_unique<GUI::LogWindow>(state, std::move(msg)));
    }

    ImGui::SameLine();
    {
        std::lock_guard lock(state.FetchTaskStatusMessageLock);
        auto pos = state.FetchTaskStatusMessages.rfind('\n');
        const char* str = state.FetchTaskStatusMessages.c_str();
        if (pos != std::string::npos) {
            str = (&state.FetchTaskStatusMessages[pos]) + 1;
        }
        ImGui::TextUnformatted(
            str, state.FetchTaskStatusMessages.data() + state.FetchTaskStatusMessages.size());
    }

    ImGui::SameLine();
    {
        static constexpr char queueSettingsLabel[] = "Queue Settings...";
        static constexpr char whenFinishedLabel[] = "when downloads are finished";
        float qTextWidth = ImGui::CalcTextSize(queueSettingsLabel, nullptr, true).x;
        float wTextWidth = ImGui::CalcTextSize(whenFinishedLabel, nullptr, true).x;
        float qButtonWidth = qTextWidth + (ImGui::GetStyle().FramePadding.x * 2.0f);
        float dropdownWidth = 100.0f * state.CurrentDpi;

        ImGui::SetCursorPosX(
            ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x
            - (qButtonWidth + ImGui::GetStyle().ItemSpacing.x * 2.0 + dropdownWidth + wTextWidth));
        if (ImGui::Button(queueSettingsLabel)) {
            ImGui::OpenPopup("QueueSettingsPopup");
        }
        if (ImGui::BeginPopup("QueueSettingsPopup")) {
            if (ImGui::Selectable("Enqueue all", false, ImGuiSelectableFlags_DontClosePopups)) {
                std::lock_guard lock(state.Jobs.JobsLock);
                for (auto& job : state.Jobs.JobsVector) {
                    if (job->JobStatus == VideoJobStatus::NotStarted) {
                        StreamService service = job->VideoInfo->GetService();
                        if (static_cast<int>(service) >= 0
                            && static_cast<size_t>(static_cast<int>(service))
                                   < state.VideoTaskGroups.size()) {
                            state.VideoTaskGroups[static_cast<size_t>(static_cast<int>(service))]
                                ->Enqueue(job.get());
                        }
                    }
                }
            }
            if (ImGui::BeginMenu("Enqueue only...")) {
                ImGui::PushID("EnqueueOnlyOptions");
                for (uint32_t i = 0; i < static_cast<uint32_t>(StreamService::COUNT); ++i) {
                    ImGui::PushID(i);
                    std::string_view ss = StreamServiceToString(static_cast<StreamService>(i));
                    if (ImGui::Selectable(ss.data(), false, ImGuiSelectableFlags_DontClosePopups)) {
                        std::lock_guard lock(state.Jobs.JobsLock);
                        for (auto& job : state.Jobs.JobsVector) {
                            if (job->JobStatus == VideoJobStatus::NotStarted
                                && job->VideoInfo->GetService() == static_cast<StreamService>(i)) {
                                state.VideoTaskGroups[i]->Enqueue(job.get());
                            }
                        }
                    }
                    ImGui::PopID();
                }
                ImGui::PopID();
                ImGui::EndMenu();
            }
            if (ImGui::Selectable("Dequeue all", false, ImGuiSelectableFlags_DontClosePopups)) {
                for (auto& g : state.VideoTaskGroups) {
                    g->DequeueAll();
                }
            }
            if (ImGui::BeginMenu("Dequeue only...")) {
                ImGui::PushID("DequeueOnlyOptions");
                for (uint32_t i = 0; i < static_cast<uint32_t>(StreamService::COUNT); ++i) {
                    ImGui::PushID(i);
                    std::string_view ss = StreamServiceToString(static_cast<StreamService>(i));
                    if (ImGui::Selectable(ss.data(), false, ImGuiSelectableFlags_DontClosePopups)) {
                        state.VideoTaskGroups[i]->DequeueAll();
                    }
                    ImGui::PopID();
                }
                ImGui::PopID();
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Auto-enqueue...")) {
                ImGui::PushID("AutoEnqueueOptions");
                for (uint32_t i = 0; i < static_cast<uint32_t>(StreamService::COUNT); ++i) {
                    ImGui::PushID(i);
                    std::string_view ss = StreamServiceToString(static_cast<StreamService>(i));
                    bool previousAutoEnqueue = state.VideoTaskGroups[i]->IsAutoEnqueue();
                    bool newAutoEnqueue = previousAutoEnqueue;
                    ImGui::Selectable(
                        ss.data(), &newAutoEnqueue, ImGuiSelectableFlags_DontClosePopups);
                    if (previousAutoEnqueue != newAutoEnqueue) {
                        state.VideoTaskGroups[i]->SetAutoEnqueue(newAutoEnqueue);
                    }
                    ImGui::PopID();
                }
                ImGui::PopID();
                ImGui::EndMenu();
            }
            ImGui::EndPopup();
        }

        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x
                             - (ImGui::GetStyle().ItemSpacing.x + dropdownWidth + wTextWidth));
        ImGui::SetNextItemWidth(dropdownWidth);
        if (ImGui::BeginCombo("##WhenDone", "Do nothing", ImGuiComboFlags_PopupAlignLeft)) {
            // TODO
            ImGui::EndCombo();
        }

        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x
                             - wTextWidth);
        ImGui::TextUnformatted(whenFinishedLabel,
                               whenFinishedLabel + (sizeof(whenFinishedLabel) - 1));
    }

    return true;
}

bool VodArchiverMainWindow::PassFilter(IVideoJob* item) const {
    assert(item != nullptr);
    assert(item->VideoInfo != nullptr);

    if (FilterStreamService != 0) {
        StreamService service = item->VideoInfo->GetService();
        if (service < StreamService::COUNT
            && (FilterStreamService & (1u << static_cast<uint32_t>(service))) != 0) {
            return false;
        }
    }

    if (FilterJobStatus != 0) {
        VideoJobStatus status = item->JobStatus;
        if (status < VideoJobStatus::COUNT
            && (FilterJobStatus & (1u << static_cast<uint32_t>(status))) != 0) {
            return false;
        }
    }

    std::string_view textFilter = HyoutaUtils::TextUtils::StripToNull(FilterTextfield);
    if (!textFilter.empty()) {
        std::array<char, 256> buffer;
        bool containsAny = HyoutaUtils::TextUtils::CaseInsensitiveContains(
                               item->VideoInfo->GetUsername(buffer), textFilter)
                           || HyoutaUtils::TextUtils::CaseInsensitiveContains(
                               item->VideoInfo->GetVideoId(buffer), textFilter)
                           || HyoutaUtils::TextUtils::CaseInsensitiveContains(
                               item->VideoInfo->GetVideoTitle(buffer), textFilter)
                           || HyoutaUtils::TextUtils::CaseInsensitiveContains(
                               item->VideoInfo->GetVideoGame(buffer), textFilter);
        if (!containsAny) {
            return false;
        }
    }

    return true;
}

} // namespace VodArchiver::GUI
