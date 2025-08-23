#pragma once

#include <condition_variable>
#include <mutex>
#include <thread>

namespace VodArchiver {
struct GuiState;
}

namespace VodArchiver::GUI {
struct BackgroundSaveThread {
    BackgroundSaveThread(GuiState& state);
    ~BackgroundSaveThread();

    void RequestSaveJobs();
    void RequestSaveUsers();

private:
    void ThreadFunc();

    GuiState& State;
    std::mutex Mutex;
    std::condition_variable CondVar;
    bool SaveJobsRequested = false;
    bool SaveUsersRequested = false;
    bool Finishing = false;
    std::thread Thread;
};
} // namespace VodArchiver::GUI
