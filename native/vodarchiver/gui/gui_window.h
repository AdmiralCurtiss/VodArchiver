#pragma once

namespace VodArchiver {
struct GuiState;
}

namespace VodArchiver::GUI {
struct Window {
    virtual ~Window();

    // returns false if the window should be deleted
    virtual bool RenderFrame(GuiState& state) = 0;

    // called before the window is deleted
    virtual void Cleanup(GuiState& state) = 0;
};
} // namespace VodArchiver::GUI
