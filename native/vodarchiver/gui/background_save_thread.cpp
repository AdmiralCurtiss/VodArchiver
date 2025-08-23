#include "background_save_thread.h"

#include <functional>

#include "gui_state.h"

#include "vodarchiver/userinfo/serialization.h"
#include "vodarchiver/videojobs/serialization.h"

namespace VodArchiver::GUI {
BackgroundSaveThread::BackgroundSaveThread(GuiState& state)
  : State(state), Thread(std::bind(&BackgroundSaveThread::ThreadFunc, this)) {}

BackgroundSaveThread::~BackgroundSaveThread() {
    {
        std::lock_guard lock(Mutex);
        Finishing = true;
    }
    CondVar.notify_all();
    Thread.join();
}

void BackgroundSaveThread::RequestSaveJobs() {
    {
        std::lock_guard lock(Mutex);
        SaveJobsRequested = true;
    }
    CondVar.notify_all();
}

void BackgroundSaveThread::RequestSaveUsers() {
    {
        std::lock_guard lock(Mutex);
        SaveUsersRequested = true;
    }
    CondVar.notify_all();
}

void BackgroundSaveThread::ThreadFunc() {
    while (true) {
        bool saveJobsRequested = false;
        bool saveUsersRequested = false;
        bool finishing = false;
        {
            std::unique_lock lock(Mutex);
            CondVar.wait(lock,
                         [&] { return SaveJobsRequested || SaveUsersRequested || Finishing; });
            saveJobsRequested = SaveJobsRequested;
            saveUsersRequested = SaveUsersRequested;
            finishing = Finishing;
        }

        if (finishing) {
            return;
        }

        if (saveJobsRequested) {
            std::string path;
            {
                std::lock_guard lock(State.JobConf.Mutex);
                path = State.JobConf.VodXmlPath;
            }
            {
                std::lock_guard lock(Mutex);
                SaveJobsRequested = false;
            }
            {
                std::lock_guard lock(State.Jobs.JobsLock);
                WriteJobsToFile(State.Jobs.JobsVector, path);
            }
        }

        if (saveUsersRequested) {
            std::string path;
            {
                std::lock_guard lock(State.JobConf.Mutex);
                path = State.JobConf.UserInfoXmlPath;
            }
            {
                std::lock_guard lock(Mutex);
                SaveUsersRequested = false;
            }
            {
                std::lock_guard lock(State.UserInfosLock);
                WriteUserInfosToFile(State.UserInfos, path);
            }
        }
    }
}
} // namespace VodArchiver::GUI
