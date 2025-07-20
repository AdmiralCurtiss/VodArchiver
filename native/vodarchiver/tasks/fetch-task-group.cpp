#include "fetch-task-group.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <exception>
#include <format>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "util/scope.h"
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
    std::function<void(std::unique_ptr<IVideoInfo> info)> enqueueJobCallback,
    uint32_t rngSeed)
  : RNG(rngSeed)
  , Services(std::move(services))
  , UserInfosLock(userInfosLock)
  , UserInfos(userInfos)
  , JobConf(jobConfig)
  , CancellationToken(cancellationToken)
  , EnqueueJobCallback(std::move(enqueueJobCallback)) {
    FetchRunnerThread = std::thread(std::bind(&FetchTaskGroup::RunFetchRunnerThreadFunc, this));
}

FetchTaskGroup::~FetchTaskGroup() {
    CancellationToken->CancelTask();
    WaitForFetchRunnerThreadToEnd();
}

void FetchTaskGroup::RunFetchRunnerThreadFunc() {
    while (true) {
        if (!CancellationToken->DelayFor(std::chrono::seconds(3))) {
            return;
        }

        try {
            // FIXME: This leaks out of the lock
            IUserInfo* earliestUserInfo = nullptr;
            {
                std::lock_guard lock(*UserInfosLock);
                for (auto& u : *UserInfos) {
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
                    }
                }
            }

            if (earliestUserInfo != nullptr) {
                DateTime earliestStartTime = earliestUserInfo->LastRefreshedOn.AddHours(7);
                DateTime now = DateTime::UtcNow();
                if (earliestStartTime <= now) {
                    auto afterFetchScope = HyoutaUtils::MakeScopeGuard([&]() {
                        if (!CancellationToken->IsCancellationRequested()) {
                            earliestUserInfo->LastRefreshedOn = now;
                            if (earliestUserInfo->Persistable) {
                                // TODO
                                // UserInfoPersister.AddOrUpdate(earliestUserInfo);
                                // UserInfoPersister.Save();
                            }
                        }
                    });
                    DoFetch(earliestUserInfo);
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

    for (auto& videoInfo : videos) {
        // Console.WriteLine("Enqueueing " + videoInfo.Username + "/" + videoInfo.VideoId);
        EnqueueJobCallback(std::move(videoInfo));
    }
}

void FetchTaskGroup::AddStatusMessage(std::string_view msg) {
    // TODO
}

void FetchTaskGroup::WaitForFetchRunnerThreadToEnd() {
    if (FetchRunnerThread.joinable()) {
        FetchRunnerThread.join();
    }
}
} // namespace VodArchiver
