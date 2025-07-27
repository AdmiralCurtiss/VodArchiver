#include "youtube-video-job.h"

#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "util/file.h"
#include "util/text.h"

#include "vodarchiver/exec.h"
#include "vodarchiver/ffmpeg_util.h"
#include "vodarchiver/filename_util.h"
#include "vodarchiver/videoinfo/youtube-video-info.h"
#include "vodarchiver/youtube_util.h"

namespace VodArchiver {
bool YoutubeVideoJob::IsWaitingForUserInput() const {
    return false;
}

static std::string PathCombine(std::string_view lhs, std::string_view rhs) {
    std::string result(lhs);
    HyoutaUtils::IO::AppendPathElement(result, rhs);
    return result;
}

static std::string GetTargetFilename(IVideoInfo& vi) {
    return std::format("youtube_{}_{}_{}_{}.mkv",
                       vi.GetUsername(),
                       DateToString(vi.GetVideoTimestamp()),
                       vi.GetVideoId(),
                       Crop(MakeIntercapsFilename(vi.GetVideoTitle()), 80));
}

static ResultType RunYoutubeVideoJob(YoutubeVideoJob& job,
                                     JobConfig& jobConfig,
                                     TaskCancellation& cancellationToken) {
    std::unique_ptr<IVideoInfo> vi;
    bool wantCookies = false;
    {
        std::lock_guard lock(*jobConfig.JobsLock);
        job.JobStatus = VideoJobStatus::Running;
        vi = job.VideoInfo->Clone();
        wantCookies = (job.Notes.find("cookies") != std::string::npos);
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

    if (dynamic_cast<YoutubeVideoInfo*>(vi.get()) == nullptr) {
        {
            std::lock_guard lock(jobConfig.Mutex);
            job.TextStatus = "Retrieving video info...";
        }
        auto result = Youtube::RetrieveVideo(vi->GetVideoId(), vi->GetUsername(), wantCookies);

        switch (result.result) {
            case Youtube::RetrieveVideoResult::Success:
                vi = result.info->Clone();
                {
                    std::lock_guard lock(jobConfig.Mutex);
                    job.VideoInfo = std::move(result.info);
                }
                break;
            case Youtube::RetrieveVideoResult::ParseFailure:
                // this seems to happen randomly from time to time, just retry later
                return ResultType::TemporarilyUnavailable;
            default: return ResultType::Failure;
        }
    }

    std::string filenameWithoutExtension = std::format("youtube_{}_{}_{}",
                                                       vi->GetUsername(),
                                                       DateToString(vi->GetVideoTimestamp()),
                                                       vi->GetVideoId());
    std::string filename = filenameWithoutExtension + ".mkv";
    std::string tempFolder = PathCombine(tempFolderPath, filenameWithoutExtension);
    std::string tempFilepath = PathCombine(tempFolder, filename);

    {
        if (!HyoutaUtils::IO::FileExists(std::string_view(tempFilepath))) {
            if (cancellationToken.IsCancellationRequested()) {
                return ResultType::Cancelled;
            }

            if (!HyoutaUtils::IO::CreateDirectory(std::string_view(tempFolder))) {
                return ResultType::Failure;
            }
            {
                std::lock_guard lock(jobConfig.Mutex);
                job.TextStatus = "Running youtube-dl...";
            }
            // don't know expected filesize, so hope we have a sensible value in minimum free space
            StallWrite(jobConfig,
                       tempFilepath,
                       0,
                       cancellationToken,
                       ShouldStallWriteRegularFile,
                       [&](std::string status) {
                           std::lock_guard lock(jobConfig.Mutex);
                           job.TextStatus = std::move(status);
                       });
            if (cancellationToken.IsCancellationRequested()) {
                return ResultType::Cancelled;
            }
            std::vector<std::string> args = {{
                "-f",
                "bestvideo[height<=?1080]+bestaudio/best",
                "-o",
                tempFilepath,
                "--merge-output-format",
                "mkv",
                "--no-color",
                "--abort-on-error",
                "--abort-on-unavailable-fragment",
                "--no-sponsorblock",
            }};
            if (wantCookies) {
                args.push_back("--cookies");
                args.push_back("d:\\cookies.txt");
            }
            args.push_back("https://www.youtube.com/watch?v=" + vi->GetVideoId());
            std::string output;
            if (RunProgram(
                    "yt-dlp.exe",
                    args,
                    [&](std::string_view sv) {
                        output.append(sv);

                        // only keep the last line
                        std::string_view trimmed = HyoutaUtils::TextUtils::Trim(output);
                        auto pos = trimmed.find_last_of("\r\n");
                        if (pos != std::string_view::npos) {
                            output.assign(trimmed.substr(pos + 1));
                        }

                        {
                            std::lock_guard lock(*jobConfig.JobsLock);
                            job.TextStatus = output;
                        }
                    },
                    [](std::string_view sv) {})
                != 0) {
                return ResultType::Failure;
            }
        }

        std::string finalFilename = GetTargetFilename(*vi);
        std::string finalFilepath = PathCombine(targetFolderPath, finalFilename);
        if (HyoutaUtils::IO::Exists(std::string_view(finalFilepath))) {
            std::lock_guard lock(*jobConfig.JobsLock);
            job.TextStatus = ("File exists: " + finalFilepath);
            return ResultType::Failure;
        }

        {
            std::lock_guard lock(*jobConfig.JobsLock);
            job.TextStatus = "Waiting for free disk IO slot to move...";
        }
        {
            auto diskLock = jobConfig.ExpensiveDiskIO.WaitForFreeSlot(cancellationToken);
            if (cancellationToken.IsCancellationRequested()) {
                return ResultType::Cancelled;
            }

            // sanity check
            {
                std::lock_guard lock(*jobConfig.JobsLock);
                job.TextStatus = "Sanity check on downloaded video...";
            }
            auto probe = FFMpegProbe(tempFilepath);
            if (!probe) {
                std::lock_guard lock(*jobConfig.JobsLock);
                job.TextStatus = "Probe failed!";
                return ResultType::Failure;
            }
            TimeSpan actualVideoLength = probe->Duration;
            TimeSpan expectedVideoLength = vi->GetVideoLength();
            if (std::abs((actualVideoLength - expectedVideoLength).GetTotalSeconds()) > 5.0) {
                // if difference is bigger than 5 seconds something is off, report
                std::lock_guard lock(*jobConfig.JobsLock);
                job.TextStatus = std::format(
                    "Large time difference between expected ({}s) and actual ({}s), stopping.",
                    expectedVideoLength.GetTotalSeconds(),
                    actualVideoLength.GetTotalSeconds());
                return ResultType::Failure;
            }

            {
                std::lock_guard lock(*jobConfig.JobsLock);
                job.TextStatus = "Moving...";
            }
            if (!HyoutaUtils::IO::Move(
                    std::string_view(tempFilepath), std::string_view(finalFilepath), true)) {
                return ResultType::Failure;
            }
            if (!HyoutaUtils::IO::DeleteDirectory(std::string_view(tempFolder))) {
                return ResultType::Failure;
            }
        }
    }

    {
        std::lock_guard lock(*jobConfig.JobsLock);
        job.TextStatus = "Done!";
        job.JobStatus = VideoJobStatus::Finished;
    }
    return ResultType::Success;
}

ResultType YoutubeVideoJob::Run(JobConfig& jobConfig, TaskCancellation& cancellationToken) {
    return RunYoutubeVideoJob(*this, jobConfig, cancellationToken);
}

std::string YoutubeVideoJob::GenerateOutputFilename() {
    auto vi = GetVideoInfo();
    return GetTargetFilename(*vi);
}
} // namespace VodArchiver
