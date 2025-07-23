#include "gui_vodarchiver_main_window.h"

#include <algorithm>
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
#include "vodarchiver/common_paths.h"
#include "vodarchiver/videojobs/i-video-job.h"
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
            if (ImGui::MenuItem("Fetch videos...")) {
                state.Windows.emplace_back(std::make_unique<GUI::FetchWindow>(state));
            }
            if (ImGui::MenuItem("Split videos...")) {
                state.Windows.emplace_back(std::make_unique<GUI::VideoSplitWindow>(state));
            }
            if (ImGui::MenuItem("Settings...")) {
                state.Windows.emplace_back(std::make_unique<GUI::SettingsWindow>(state));
            }
            if (ImGui::MenuItem("Fetch Log...")) {
                std::string msg;
                {
                    std::lock_guard lock(state.FetchTaskStatusMessageLock);
                    msg = state.FetchTaskStatusMessages;
                }
                state.Windows.emplace_back(std::make_unique<GUI::LogWindow>(state, std::move(msg)));
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

    ImGuiUtils::TextUnformattedRightAlign("VodArchiver " VODARCHIVER_VERSION);

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
        int items_count = static_cast<int>(state.Jobs.JobsVector.size());

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

            // Sort our data if sort specs have been changed!
            ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs();
            if (sort_specs && sort_specs->SpecsDirty)
                items_need_sort = true;
            if (sort_specs && items_need_sort && items.Size > 1) {
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
                                        return l->GetVideoInfo()->GetService()
                                               < r->GetVideoInfo()->GetService();
                                    });
                            } else {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoJob* l = state.Jobs.JobsVector[lhs].get();
                                        IVideoJob* r = state.Jobs.JobsVector[rhs].get();
                                        return l->GetVideoInfo()->GetService()
                                               > r->GetVideoInfo()->GetService();
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
                                        return l->GetVideoInfo()->GetVideoId()
                                               < r->GetVideoInfo()->GetVideoId();
                                    });
                            } else {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoJob* l = state.Jobs.JobsVector[lhs].get();
                                        IVideoJob* r = state.Jobs.JobsVector[rhs].get();
                                        return l->GetVideoInfo()->GetVideoId()
                                               > r->GetVideoInfo()->GetVideoId();
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
                                        return l->GetVideoInfo()->GetUsername()
                                               < r->GetVideoInfo()->GetUsername();
                                    });
                            } else {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoJob* l = state.Jobs.JobsVector[lhs].get();
                                        IVideoJob* r = state.Jobs.JobsVector[rhs].get();
                                        return l->GetVideoInfo()->GetUsername()
                                               > r->GetVideoInfo()->GetUsername();
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
                                        return l->GetVideoInfo()->GetVideoTitle()
                                               < r->GetVideoInfo()->GetVideoTitle();
                                    });
                            } else {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoJob* l = state.Jobs.JobsVector[lhs].get();
                                        IVideoJob* r = state.Jobs.JobsVector[rhs].get();
                                        return l->GetVideoInfo()->GetVideoTitle()
                                               > r->GetVideoInfo()->GetVideoTitle();
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
                                        return l->GetVideoInfo()->GetVideoGame()
                                               < r->GetVideoInfo()->GetVideoGame();
                                    });
                            } else {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoJob* l = state.Jobs.JobsVector[lhs].get();
                                        IVideoJob* r = state.Jobs.JobsVector[rhs].get();
                                        return l->GetVideoInfo()->GetVideoGame()
                                               > r->GetVideoInfo()->GetVideoGame();
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
                                        return l->GetVideoInfo()->GetVideoTimestamp()
                                               < r->GetVideoInfo()->GetVideoTimestamp();
                                    });
                            } else {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoJob* l = state.Jobs.JobsVector[lhs].get();
                                        IVideoJob* r = state.Jobs.JobsVector[rhs].get();
                                        return l->GetVideoInfo()->GetVideoTimestamp()
                                               > r->GetVideoInfo()->GetVideoTimestamp();
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
                                        return l->GetVideoInfo()->GetVideoLength()
                                               < r->GetVideoInfo()->GetVideoLength();
                                    });
                            } else {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoJob* l = state.Jobs.JobsVector[lhs].get();
                                        IVideoJob* r = state.Jobs.JobsVector[rhs].get();
                                        return l->GetVideoInfo()->GetVideoLength()
                                               > r->GetVideoInfo()->GetVideoLength();
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
                                        return l->GetVideoInfo()->GetVideoRecordingState()
                                               < r->GetVideoInfo()->GetVideoRecordingState();
                                    });
                            } else {
                                std::stable_sort(
                                    items.begin(), items.end(), [&](int lhs, int rhs) -> bool {
                                        IVideoJob* l = state.Jobs.JobsVector[lhs].get();
                                        IVideoJob* r = state.Jobs.JobsVector[rhs].get();
                                        return l->GetVideoInfo()->GetVideoRecordingState()
                                               > r->GetVideoInfo()->GetVideoRecordingState();
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
            }
            items_need_sort = false;

            ImGui::TableHeadersRow();

            ImGuiListClipper clipper;
            clipper.Begin(items.Size);
            while (clipper.Step()) {
                for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; ++row_n) {
                    const int ID = items[row_n];
                    IVideoJob* item = state.Jobs.JobsVector[ID].get();
                    // if (!filter.PassFilter(item->Name))
                    //     continue;

                    const bool item_is_selected = selection.contains(ID);
                    ImGui::PushID(ID);
                    ImGui::TableNextRow(ImGuiTableRowFlags_None, 0.0f);

                    if (ImGui::TableSetColumnIndex(ColumnID_Index)) {
                        ImGui::Text("%d", ID);
                    }
                    if (ImGui::TableSetColumnIndex(ColumnID_Valid)) {
                        ImGui::Text("%s", item->HasBeenValidated ? "true" : "false");
                    }
                    if (ImGui::TableSetColumnIndex(ColumnID_Service)) {
                        std::string_view sv =
                            StreamServiceToString(item->GetVideoInfo()->GetService());
                        ImGui::TextUnformatted(sv.data(), sv.data() + sv.size());
                    }
                    if (ImGui::TableSetColumnIndex(ColumnID_VideoID)) {
                        std::string s = item->GetVideoInfo()->GetVideoId();
                        ImGui::TextUnformatted(s.data(), s.data() + s.size());
                    }
                    if (ImGui::TableSetColumnIndex(ColumnID_Username)) {
                        std::string s = item->GetVideoInfo()->GetUsername();
                        ImGui::TextUnformatted(s.data(), s.data() + s.size());
                    }
                    if (ImGui::TableSetColumnIndex(ColumnID_Title)) {
                        std::string s = item->GetVideoInfo()->GetVideoTitle();
                        ImGui::TextUnformatted(s.data(), s.data() + s.size());
                    }
                    if (ImGui::TableSetColumnIndex(ColumnID_Game)) {
                        std::string s = item->GetVideoInfo()->GetVideoGame();
                        ImGui::TextUnformatted(s.data(), s.data() + s.size());
                    }
                    if (ImGui::TableSetColumnIndex(ColumnID_Timestamp)) {
                        std::string s =
                            DateTimeToStringForGui(item->GetVideoInfo()->GetVideoTimestamp());
                        ImGui::TextUnformatted(s.data(), s.data() + s.size());
                    }
                    if (ImGui::TableSetColumnIndex(ColumnID_Duration)) {
                        std::string s = std::format(
                            "{}", item->GetVideoInfo()->GetVideoLength().GetTotalSeconds());
                        ImGui::TextUnformatted(s.data(), s.data() + s.size());
                    }
                    if (ImGui::TableSetColumnIndex(ColumnID_RecordingState)) {
                        std::string_view s =
                            RecordingStateToString(item->GetVideoInfo()->GetVideoRecordingState());
                        ImGui::TextUnformatted(s.data(), s.data() + s.size());
                    }
                    if (ImGui::TableSetColumnIndex(ColumnID_Notes)) {
                        const std::string& s = item->Notes;
                        ImGui::TextUnformatted(s.data(), s.data() + s.size());
                    }
                    if (ImGui::TableSetColumnIndex(ColumnID_Status)) {
                        const std::string& s = item->GetStatus();
                        ImGui::TextUnformatted(s.data(), s.data() + s.size());
                    }
                    if (ImGui::TableSetColumnIndex(ColumnID_Actions)) {
                        if (ImGui::SmallButton("...##JobActionsPopupButton")) {
                            ImGui::OpenPopup("JobActionsPopup");
                        }
                        if (ImGui::BeginPopup("JobActionsPopup")) {
                            auto service = item->GetVideoInfo()->GetService();
                            auto is_in_queue = [&]() -> bool {
                                for (auto& g : state.VideoTaskGroups) {
                                    if (g->Service == service) {
                                        return g->IsInQueue(item);
                                    }
                                }
                                return false;
                            };

                            auto uir = item->GetUserInputRequest();
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
                                            if (g->Service == service) {
                                                g->Dequeue(item);
                                                break;
                                            }
                                        }
                                    }
                                } else {
                                    if (ImGui::Selectable("Enqueue")) {
                                        for (auto& g : state.VideoTaskGroups) {
                                            if (g->Service == service) {
                                                g->Add(item);
                                                break;
                                            }
                                        }
                                    }
                                }
                                if (ImGui::Selectable("Download now")) {
                                    for (auto& g : state.VideoTaskGroups) {
                                        if (g->Service == service) {
                                            g->Add(item, true);
                                            break;
                                        }
                                    }
                                }
                            }
                            if (ImGui::Selectable("Copy Video ID")) {
                                std::string id = item->GetVideoInfo()->GetVideoId();
                                ImGui::SetClipboardText(id.c_str());
                            }
                            if (ImGui::Selectable("Copy Output Filename")) {
                                std::string fn = item->GenerateOutputFilename();
                                ImGui::SetClipboardText(fn.c_str());
                            }
                            if (ImGui::Selectable("Copy Status")) {
                                const std::string& status = item->GetStatus();
                                ImGui::SetClipboardText(status.c_str());
                            }
                            if (item->JobStatus == VideoJobStatus::Running) {
                                if (ImGui::Selectable("Stop")) {
                                    for (auto& g : state.VideoTaskGroups) {
                                        if (g->Service == service) {
                                            g->CancelJob(item);
                                            break;
                                        }
                                    }
                                }
                            }
                            if (item->JobStatus == VideoJobStatus::NotStarted) {
                                if (ImGui::Selectable("Kill")) {
                                    item->JobStatus = VideoJobStatus::Dead;
                                    item->_Status = "[Manually killed] " + item->_Status;
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


    if (ImGuiUtils::ButtonRightAlign("Close")) {
        return false;
    }

    return true;
}
} // namespace VodArchiver::GUI
