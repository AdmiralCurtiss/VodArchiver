#include "twitch-chat-replay-job.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <format>
#include <string>

#include "util/file.h"
#include "util/number.h"

#include "vodarchiver/exec.h"
#include "vodarchiver/system_util.h"
#include "vodarchiver/twitch_util.h"

static bool DeleteDirectoryRecursive(const std::filesystem::path& rootDir) {
    {
        std::error_code ec;
        std::filesystem::directory_iterator iterator(rootDir, ec);
        if (ec) {
            return false;
        }
        while (iterator != std::filesystem::directory_iterator()) {
            if (iterator->is_directory()) {
                if (!DeleteDirectoryRecursive(*iterator)) {
                    return false;
                }
            } else {
                if (!HyoutaUtils::IO::DeleteFile(*iterator)) {
                    return false;
                }
            }
            iterator.increment(ec);
            if (ec) {
                return false;
            }
        }
    }

    return HyoutaUtils::IO::DeleteDirectory(rootDir);
}

static bool DeleteDirectoryRecursive(std::string_view path) {
    return DeleteDirectoryRecursive(HyoutaUtils::IO::FilesystemPathFromUtf8(path));
}

namespace VodArchiver {
namespace {
struct UserInputRequestStreamLiveTwitchChatReplay : IUserInputRequest {
    std::string Question = "Stream still Live";
    std::vector<std::string> Options;
    TwitchChatReplayJob* Job;

    UserInputRequestStreamLiveTwitchChatReplay(TwitchChatReplayJob* job) {
        Job = job;
        Options.push_back("Keep Waiting");
        Options.push_back("Assume Finished");
    }

    const std::string& GetQuestion() const override {
        return Question;
    }

    const std::vector<std::string>& GetOptions() const override {
        return Options;
    }

    void SelectOption(const std::string& option) override {
        Job->AssumeFinished = (option == "Assume Finished");
    }
};
} // namespace

bool TwitchChatReplayJob::IsWaitingForUserInput() const {
    return false; // yes, this is correct, there is a possible user input request here but it never
                  // blocks running the task
}

IUserInputRequest* TwitchChatReplayJob::GetUserInputRequest() const {
    return UserInputRequest.get();
}

static std::string GetTempFilenameWithoutExtension(IVideoInfo& videoInfo) {
    std::array<char, 256> buffer1;
    std::array<char, 256> buffer2;
    return std::format(
        "twitch_{}_v{}_chat_gql", videoInfo.GetUsername(buffer1), videoInfo.GetVideoId(buffer2));
}

static std::string GetTargetFilenameWithoutExtension(IVideoInfo& videoInfo) {
    std::array<char, 256> buffer1;
    std::array<char, 256> buffer2;
    return std::format("twitch_{}_{}_v{}_chat_gql",
                       videoInfo.GetUsername(buffer1),
                       DateTimeToStringForFilesystem(videoInfo.GetVideoTimestamp()),
                       videoInfo.GetVideoId(buffer2));
}

static std::string PathCombine(std::string_view lhs, std::string_view rhs) {
    std::string result(lhs);
    HyoutaUtils::IO::AppendPathElement(result, rhs);
    return result;
}

ResultType RunChatJob(TwitchChatReplayJob& job,
                      JobConfig& jobConfig,
                      TaskCancellation& cancellationToken) {
    std::unique_ptr<IVideoInfo> videoInfo;
    bool assumeFinished = false;
    {
        std::lock_guard lock(*jobConfig.JobsLock);
        job.JobStatus = VideoJobStatus::Running;
        videoInfo = job.VideoInfo->Clone();
        assumeFinished = job.AssumeFinished;
    }

    std::string tempFolderPath;
    std::string targetFolderPath;
    {
        std::lock_guard lock(jobConfig.Mutex);
        tempFolderPath = jobConfig.TempFolderPath;
        targetFolderPath = jobConfig.TargetFolderPath;
    }

    if (cancellationToken.IsCancellationRequested()) {
        return ResultType::Cancelled;
    }

    std::array<char, 256> buffer;
    auto videoId = HyoutaUtils::NumberUtils::ParseInt64(videoInfo->GetVideoId(buffer));
    if (!videoId) {
        return ResultType::Failure;
    }

    {
        std::lock_guard lock(*jobConfig.JobsLock);
        job.JobStatus = VideoJobStatus::Running;
        job.TextStatus = "Retrieving video info...";
    }
    auto video_json = TwitchYTDL::GetVideoJson(*videoId);
    if (!video_json) {
        return ResultType::Failure;
    }
    auto twitchVideoData = TwitchYTDL::VideoFromJson(*video_json);

    {
        auto newVideoInfo = std::make_unique<TwitchVideoInfo>();
        newVideoInfo->Service = StreamService::TwitchChatReplay;
        newVideoInfo->Video = std::move(*twitchVideoData);
        videoInfo = newVideoInfo->Clone();

        std::lock_guard lock(*jobConfig.JobsLock);
        job.VideoInfo = std::move(newVideoInfo);
    }

    if (!assumeFinished && videoInfo->GetVideoRecordingState() == RecordingState::Live) {
        std::lock_guard lock(*jobConfig.JobsLock);
        job.UserInputRequest = std::make_unique<UserInputRequestStreamLiveTwitchChatReplay>(&job);
        return ResultType::TemporarilyUnavailable;
    }

    std::string tempfolder =
        PathCombine(tempFolderPath, GetTempFilenameWithoutExtension(*videoInfo));
    std::string tempname = PathCombine(tempfolder, "a.json");
    std::string filename =
        PathCombine(targetFolderPath, GetTargetFilenameWithoutExtension(*videoInfo) + ".json");

    if (HyoutaUtils::IO::DirectoryExists(std::string_view(tempfolder))
        == HyoutaUtils::IO::ExistsResult::DoesExist) {
        if (!DeleteDirectoryRecursive(std::string_view(tempfolder))) {
            return ResultType::Failure;
        }
    }
    if (!HyoutaUtils::IO::CreateDirectory(std::string_view(tempfolder))) {
        return ResultType::Failure;
    }

    {
        std::lock_guard lock(*jobConfig.JobsLock);
        job.TextStatus = "Downloading chat data...";
    }
    std::vector<std::string> args;
    args.push_back("chatdownload");
    args.push_back("-E");
    args.push_back("--stv");
    args.push_back("false");
    args.push_back("--chat-connections");
    args.push_back("1");
    args.push_back("--id");
    args.push_back(std::string(videoInfo->GetVideoId(buffer)));
    args.push_back("-o");
    args.push_back(tempname);
    if (RunProgram(
            "TwitchDownloaderCLI\\TwitchDownloaderCLI.exe",
            args,
            [&](std::string_view) {},
            [&](std::string_view) {})
        != 0) {
        return ResultType::Failure;
    }

    if (!HyoutaUtils::IO::Move(std::string_view(tempname), std::string_view(filename), true)) {
        return ResultType::Failure;
    }

    DeleteDirectoryRecursive(std::string_view(tempfolder));

    {
        std::lock_guard lock(*jobConfig.JobsLock);
        job.TextStatus = "Done!";
        job.JobStatus = VideoJobStatus::Finished;
    }
    return ResultType::Success;
}

ResultType TwitchChatReplayJob::Run(JobConfig& jobConfig, TaskCancellation& cancellationToken) {
    return RunChatJob(*this, jobConfig, cancellationToken);
}

std::string TwitchChatReplayJob::GenerateOutputFilename() {
    return GetTargetFilenameWithoutExtension(*this->VideoInfo) + ".json";
}

std::unique_ptr<IVideoJob> TwitchChatReplayJob::Clone() const {
    auto clone = std::make_unique<TwitchChatReplayJob>();
    clone->TextStatus = this->TextStatus;
    clone->JobStatus = this->JobStatus;
    clone->HasBeenValidated = this->HasBeenValidated;
    clone->VideoInfo = this->VideoInfo ? this->VideoInfo->Clone() : nullptr;
    clone->JobStartTimestamp = this->JobStartTimestamp;
    clone->JobFinishTimestamp = this->JobFinishTimestamp;
    clone->Notes = this->Notes;
    clone->AssumeFinished = this->AssumeFinished;
    return clone;
}
} // namespace VodArchiver
