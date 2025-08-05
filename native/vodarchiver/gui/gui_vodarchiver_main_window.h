#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "gui_file_browser.h"
#include "gui_window.h"

namespace VodArchiver {
struct IVideoJob;
}

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

    bool PassFilter(IVideoJob* item) const;

    std::vector<uint32_t> ItemIndices;
    size_t ItemCountLastSeen = 0;
    bool ItemsNeedSort = false;
    bool ItemsNeedFilter = false;

    // bit index == enum index, 0 means visible, 1 means not visible
    uint32_t FilterStreamService = 0;
    uint32_t FilterJobStatus = 0;

    std::array<char, 1024> FilterTextfield{};
};
} // namespace VodArchiver::GUI
