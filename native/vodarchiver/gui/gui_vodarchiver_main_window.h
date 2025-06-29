#pragma once

#include <memory>
#include <string>

#include "gui_file_browser.h"
#include "gui_window.h"

#include "imgui.h"

namespace VodArchiver::GUI {
struct VodArchiverMainWindow : public VodArchiver::GUI::Window {
    VodArchiverMainWindow();
    VodArchiverMainWindow(const VodArchiverMainWindow& other) = delete;
    VodArchiverMainWindow(VodArchiverMainWindow&& other) = delete;
    VodArchiverMainWindow& operator=(const VodArchiverMainWindow& other) = delete;
    VodArchiverMainWindow& operator=(VodArchiverMainWindow&& other) = delete;
    ~VodArchiverMainWindow() override;

    bool RenderFrame(GuiState& state) override;
    void Cleanup(GuiState& state) override;

private:
    bool RenderContents(GuiState& state);
    bool HasPendingWindowRequest() const;

    ImVector<int> Items;
    ImVector<int> Selection;
    bool ItemsNeedSort = false;
};
} // namespace VodArchiver::GUI
