#include "twitch-chat-replay-job.h"

#include <cstdint>
#include <filesystem>
#include <format>
#include <string>

#include "util/file.h"
#include "util/number.h"

#include "vodarchiver/exec.h"
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
    return false;
}

std::shared_ptr<IUserInputRequest> TwitchChatReplayJob::GetUserInputRequest() const {
    return _UserInputRequest;
}

std::string TwitchChatReplayJob::GetTempFilenameWithoutExtension() const {
    auto videoInfo = GetVideoInfo();
    return std::format("twitch_{}_v{}_chat_gql", videoInfo->GetUsername(), videoInfo->GetVideoId());
}

std::string TwitchChatReplayJob::GetTargetFilenameWithoutExtension() const {
    auto videoInfo = GetVideoInfo();
    return std::format("twitch_{}_{}_v{}_chat_gql",
                       videoInfo->GetUsername(),
                       DateTimeToStringForFilesystem(videoInfo->GetVideoTimestamp()),
                       videoInfo->GetVideoId());
}

std::string TwitchChatReplayJob::GenerateOutputFilename() {
    return GetTargetFilenameWithoutExtension() + ".json";
}

bool TwitchChatReplayJob::ShouldStallWrite(const std::string& path, uint64_t filesize) const {
    return false;
    // long freeSpace = new System.IO.DriveInfo(path).AvailableFreeSpace;
    // return freeSpace
    //        <= (Math.Min(Util.AbsoluteMinimumFreeSpaceBytes, Util.MinimumFreeSpaceBytes) +
    //        filesize);
}

static std::string PathCombine(std::string_view lhs, std::string_view rhs) {
    std::string result(lhs);
    HyoutaUtils::IO::AppendPathElement(result, rhs);
    return result;
}

ResultType TwitchChatReplayJob::Run(JobConfig& jobConfig, TaskCancellation& cancellationToken) {
    if (cancellationToken.IsCancellationRequested()) {
        return ResultType::Cancelled;
    }

    std::string tempFolderPath;
    std::string targetFolderPath;
    {
        std::lock_guard lock(jobConfig.Mutex);
        tempFolderPath = jobConfig.TempFolderPath;
        targetFolderPath = jobConfig.TargetFolderPath;
    }

    auto videoInfo = GetVideoInfo();
    auto videoId = HyoutaUtils::NumberUtils::ParseInt64(videoInfo->GetVideoId());
    if (!videoId) {
        return ResultType::Failure;
    }

    JobStatus = VideoJobStatus::Running;
    SetStatus("Retrieving video info...");
    auto video_json = TwitchYTDL::GetVideoJson(*videoId);
    if (!video_json) {
        return ResultType::Failure;
    }
    auto twitchVideoData = TwitchYTDL::VideoFromJson(*video_json);
    auto newVideoInfo = std::make_shared<TwitchVideoInfo>();
    newVideoInfo->_Service = StreamService::TwitchChatReplay;
    newVideoInfo->_Video = std::move(*twitchVideoData);
    SetVideoInfo(newVideoInfo);

    if (!AssumeFinished && newVideoInfo->GetVideoRecordingState() == RecordingState::Live) {
        _UserInputRequest = std::make_unique<UserInputRequestStreamLiveTwitchChatReplay>(this);
        return ResultType::TemporarilyUnavailable;
    }

    std::string tempfolder = PathCombine(tempFolderPath, GetTempFilenameWithoutExtension());
    std::string tempname = PathCombine(tempfolder, "a.json");
    std::string filename =
        PathCombine(targetFolderPath, GetTargetFilenameWithoutExtension() + ".json");

    if (HyoutaUtils::IO::DirectoryExists(std::string_view(tempfolder))) {
        if (!DeleteDirectoryRecursive(std::string_view(tempfolder))) {
            return ResultType::Failure;
        }
    }
    if (!HyoutaUtils::IO::CreateDirectory(std::string_view(tempfolder))) {
        return ResultType::Failure;
    }

    SetStatus("Downloading chat data...");
    std::vector<std::string> args;
    args.push_back("chatdownload");
    args.push_back("-E");
    args.push_back("--stv");
    args.push_back("false");
    args.push_back("--chat-connections");
    args.push_back("1");
    args.push_back("--id");
    args.push_back(newVideoInfo->GetVideoId());
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

    SetStatus("Done!");
    JobStatus = VideoJobStatus::Finished;
    return ResultType::Success;
}
} // namespace VodArchiver
