#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string_view>
#include <thread>
#include <vector>

#include "util/xorshift.h"

#include "vodarchiver/job_config.h"
#include "vodarchiver/task_cancellation.h"
#include "vodarchiver/userinfo/i-user-info.h"
#include "vodarchiver/videoinfo/i-video-info.h"

namespace VodArchiver {
struct FetchTaskGroup {
    HyoutaUtils::RNG::xorshift RNG;
    std::vector<ServiceVideoCategoryType> Services;
    std::recursive_mutex* UserInfosLock = nullptr;
    std::vector<std::unique_ptr<IUserInfo>>* UserInfos = nullptr;
    JobConfig* JobConf = nullptr;
    TaskCancellation* CancellationToken = nullptr;

    std::function<bool(std::unique_ptr<IVideoInfo> info)> EnqueueJobCallback;
    std::function<void(std::string_view msg)> AddStatusMessageCallback;
    std::function<void()> SaveVodsCallback;
    std::function<void()> SaveUserInfosCallback;

    std::thread FetchRunnerThread;

    FetchTaskGroup(std::vector<ServiceVideoCategoryType> services,
                   std::recursive_mutex* userInfosLock,
                   std::vector<std::unique_ptr<IUserInfo>>* userInfos,
                   JobConfig* jobConfig,
                   TaskCancellation* cancellationToken,
                   std::function<bool(std::unique_ptr<IVideoInfo> info)> enqueueJobCallback,
                   std::function<void(std::string_view msg)> addStatusMessageCallback,
                   std::function<void()> saveVodsCallback,
                   std::function<void()> saveUserInfosCallback,
                   uint32_t rngSeed);
    ~FetchTaskGroup();

private:
    void RunFetchRunnerThreadFunc();
    void DoFetch(IUserInfo* userInfo);
    bool WriteBack(IUserInfo* userInfo, size_t expectedIndex, DateTime now);
    void AddStatusMessage(std::string_view msg);
    void WaitForFetchRunnerThreadToEnd();
};
} // namespace VodArchiver
