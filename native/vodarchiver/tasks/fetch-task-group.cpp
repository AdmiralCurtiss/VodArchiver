#include "fetch-task-group.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <exception>
#include <format>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "util/scope.h"
#include "util/thread.h"
#include "util/xorshift.h"

#include "vodarchiver/job_config.h"
#include "vodarchiver/task_cancellation.h"
#include "vodarchiver/userinfo/i-user-info.h"

namespace VodArchiver {
FetchTaskGroup::FetchTaskGroup(
    std::vector<ServiceVideoCategoryType> services,
    std::recursive_mutex* userInfosLock,
    std::vector<std::unique_ptr<IUserInfo>>* userInfos,
    JobConfig* jobConfig,
    TaskCancellation* cancellationToken,
    std::function<bool(std::unique_ptr<IVideoInfo> info)> enqueueJobCallback,
    std::function<void(std::string_view msg)> addStatusMessageCallback,
    std::function<void()> saveVodsCallback,
    std::function<void()> saveUserInfosCallback,
    uint32_t rngSeed)
  : RNG(rngSeed)
  , Services(std::move(services))
  , UserInfosLock(userInfosLock)
  , UserInfos(userInfos)
  , JobConf(jobConfig)
  , CancellationToken(cancellationToken)
  , EnqueueJobCallback(std::move(enqueueJobCallback))
  , AddStatusMessageCallback(std::move(addStatusMessageCallback))
  , SaveVodsCallback(std::move(saveVodsCallback))
  , SaveUserInfosCallback(std::move(saveUserInfosCallback)) {
    FetchRunnerThread = std::thread(std::bind(&FetchTaskGroup::RunFetchRunnerThreadFunc, this));
}

FetchTaskGroup::~FetchTaskGroup() {
    CancellationToken->CancelTask();
    WaitForFetchRunnerThreadToEnd();
}

void FetchTaskGroup::RunFetchRunnerThreadFunc() {
    {
        std::string_view invalidServices = "None";
        std::string threadName = std::format(
            "Fetch{}",
            Services.empty() ? invalidServices : ServiceVideoCategoryTypeToString(Services[0]));
        HyoutaUtils::SetThreadName(threadName.c_str());
    }

    while (true) {
        if (!CancellationToken->DelayFor(std::chrono::seconds(3))) {
            return;
        }

        try {
            std::unique_ptr<IUserInfo> userInfoClone = nullptr;
            size_t userInfoCloneIndex = 0;
            {
                IUserInfo* earliestUserInfo = nullptr;
                std::lock_guard lock(*UserInfosLock);
                for (size_t i = 0; i < UserInfos->size(); ++i) {
                    auto& u = (*UserInfos)[i];
                    if (!u->AutoDownload) {
                        continue;
                    }

                    auto us = u->GetType();
                    if (std::any_of(Services.begin(),
                                    Services.end(),
                                    [&](ServiceVideoCategoryType t) { return us == t; })
                        && (earliestUserInfo == nullptr
                            || u->LastRefreshedOn < earliestUserInfo->LastRefreshedOn)) {
                        earliestUserInfo = u.get();
                        userInfoCloneIndex = i;
                    }
                }
                if (earliestUserInfo != nullptr) {
                    userInfoClone = earliestUserInfo->Clone();
                }
            }

            if (userInfoClone != nullptr) {
                DateTime earliestStartTime = userInfoClone->LastRefreshedOn.AddHours(7);
                DateTime now = DateTime::UtcNow();
                if (earliestStartTime <= now) {
                    auto afterFetchScope = HyoutaUtils::MakeScopeGuard([&]() {
                        if (!CancellationToken->IsCancellationRequested()) {
                            // try to write back the timestamp into the actual vector
                            bool writtenBack =
                                WriteBack(userInfoClone.get(), userInfoCloneIndex, now);

                            // if we have updated the userinfo, request a save to disk
                            if (writtenBack && userInfoClone->Persistable) {
                                SaveUserInfosCallback();
                            }
                        }
                    });
                    DoFetch(userInfoClone.get());
                }
            }
        } catch (const std::exception& ex) {
            AddStatusMessage(std::format("Error during fetch: {}", ex.what()));
        }
    }
}

void FetchTaskGroup::DoFetch(IUserInfo* userInfo) {
    std::vector<std::unique_ptr<IVideoInfo>> videos;

    while (true) {
        AddStatusMessage(std::format("Fetching {}...", userInfo->ToString()));

        try {
            FetchReturnValue fetchReturnValue;
            size_t Offset = 0;
            do {
                {
                    uint32_t min_wait_ms = 155000;
                    uint32_t max_wait_ms = 195000;
                    uint32_t wait_ms = min_wait_ms + (RNG() % (max_wait_ms - min_wait_ms));
                    CancellationToken->DelayFor(std::chrono::milliseconds(wait_ms));
                }
                if (CancellationToken->IsCancellationRequested()) {
                    break;
                }
                fetchReturnValue = userInfo->Fetch(*JobConf, Offset, true);
                Offset += fetchReturnValue.VideoCountThisFetch;
                if (fetchReturnValue.Success) {
                    for (auto& v : fetchReturnValue.Videos) {
                        videos.push_back(std::move(v));
                    }
                }
            } while (fetchReturnValue.Success && fetchReturnValue.HasMore);
            break;
        } catch (const std::exception& ex) {
            AddStatusMessage(std::format("Error during {}: {}", userInfo->ToString(), ex.what()));
            break;
        }
    }

    if (!CancellationToken->IsCancellationRequested()) {
        uint32_t min_wait_ms = 55000;
        uint32_t max_wait_ms = 95000;
        uint32_t wait_ms = min_wait_ms + (RNG() % (max_wait_ms - min_wait_ms));
        CancellationToken->DelayFor(std::chrono::milliseconds(wait_ms));
    }
    AddStatusMessage(std::format("Fetched {} items from {}.", videos.size(), userInfo->ToString()));

    bool createdAny = false;
    for (auto& videoInfo : videos) {
        // Console.WriteLine("Enqueueing " + videoInfo.Username + "/" + videoInfo.VideoId);
        if (EnqueueJobCallback(std::move(videoInfo))) {
            createdAny = true;
        }
    }

    if (createdAny) {
        SaveVodsCallback();
    }
}

bool FetchTaskGroup::WriteBack(IUserInfo* userInfo, size_t expectedIndex, DateTime now) {
    ServiceVideoCategoryType type = userInfo->GetType();
    std::string uid = userInfo->GetUserIdentifier();

    std::lock_guard lock(*UserInfosLock);

    // this only works if the index hasn't changed, but the vector
    // changes so rarely that it's always a good idea to try first
    if (UserInfos->size() < expectedIndex) {
        auto& u = (*UserInfos)[expectedIndex];
        if (u->GetType() == type && u->GetUserIdentifier() == uid) {
            u->LastRefreshedOn = now;
            return true;
        }
    }

    // if this didn't work try to find it elsewhere
    for (size_t i = 0; i < UserInfos->size(); ++i) {
        auto& u = (*UserInfos)[i];
        if (u->GetType() == type && u->GetUserIdentifier() == uid) {
            u->LastRefreshedOn = now;
            return true;
        }
    }

    // couldn't find it
    return false;
}

void FetchTaskGroup::AddStatusMessage(std::string_view msg) {
    AddStatusMessageCallback(msg);
}

void FetchTaskGroup::WaitForFetchRunnerThreadToEnd() {
    if (FetchRunnerThread.joinable()) {
        FetchRunnerThread.join();
    }
}
} // namespace VodArchiver
