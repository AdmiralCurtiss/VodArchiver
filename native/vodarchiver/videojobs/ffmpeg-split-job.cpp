#include "ffmpeg-split-job.h"

#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "i-video-job.h"
#include "util/file.h"
#include "util/number.h"
#include "util/text.h"

#include "vodarchiver/exec.h"
#include "vodarchiver/videoinfo/generic-video-info.h"

namespace VodArchiver {
FFMpegSplitJob::FFMpegSplitJob() {}

FFMpegSplitJob::FFMpegSplitJob(std::string path, std::string splitTimes) {
    JobStatus = VideoJobStatus::NotStarted;
    auto vi = std::make_unique<GenericVideoInfo>();
    vi->Service = StreamService::FFMpegJob;
    vi->VideoId = std::move(path);
    vi->Username = "_split";
    vi->VideoTitle = splitTimes;
    VideoInfo = std::move(vi);
    TextStatus = "...";
    SplitTimes = std::move(splitTimes);
}

bool FFMpegSplitJob::IsWaitingForUserInput() const {
    return false;
}

static std::string GenerateOutputFilenameInternal(std::string_view fullPath) {
    std::string_view inputName =
        HyoutaUtils::TextUtils::Replace(HyoutaUtils::IO::SplitPath(fullPath).Filename, "%", "%%");
    std::string_view fn = HyoutaUtils::IO::GetFileNameWithoutExtension(inputName);
    std::string_view ext = HyoutaUtils::IO::GetExtension(inputName);
    std::vector<std::string_view> split = HyoutaUtils::TextUtils::Split(fn, "_");
    std::vector<std::string> output;

    bool added = false;
    for (const std::string_view& s : split) {
        if (!added && s.starts_with("v")
            && HyoutaUtils::NumberUtils::ParseUInt64(s.substr(1)).has_value()) {
            output.push_back(std::string(s) + "-p%d");
            added = true;
        } else {
            output.push_back(std::string(s));
        }
    }

    if (!added) {
        output.push_back("%d");
    }

    std::string rejoined = HyoutaUtils::TextUtils::Join(output, "_");
    rejoined.append(ext);
    return rejoined;
}

static std::string GenerateOutputName(std::string_view inputName, std::string_view fullPath) {
    std::string dir = std::string(HyoutaUtils::IO::SplitPath(inputName).Directory);
    HyoutaUtils::IO::AppendPathElement(dir, GenerateOutputFilenameInternal(fullPath));
    return dir;
}

static std::string EvaluateOutputName(std::string_view input, size_t index) {
    std::string result;
    size_t i = 0;
    while (i < input.size()) {
        const char ch = input[i];
        if (ch == '%') {
            ++i;
            if (i < input.size()) {
                const char format = input[i];
                if (format == '%') {
                    // %% -> %
                    result.push_back('%');
                    ++i;
                    continue;
                }
                if (format == 'd') {
                    // %d -> index
                    result.append(std::format("{}", index));
                    ++i;
                    continue;
                }

                // unclear what to do here, should probably report error?
                result.push_back(ch);
                result.push_back(format);
                ++i;
                continue;
            } else {
                // unclear what to do here, should probably report error?
                result.push_back(ch);
                break;
            }
        } else {
            result.push_back(ch);
            ++i;
        }
    }
    return result;
}

static ResultType
    RunSplitJob(FFMpegSplitJob& job, JobConfig& jobConfig, TaskCancellation& cancellationToken) {
    std::unique_ptr<IVideoInfo> videoInfo;
    std::string splitTimes;
    {
        std::lock_guard lock(*jobConfig.JobsLock);
        job.JobStatus = VideoJobStatus::Running;
        job.TextStatus = "Checking files...";
        videoInfo = job.VideoInfo->Clone();
        splitTimes = job.SplitTimes;
    }

    if (cancellationToken.IsCancellationRequested()) {
        return ResultType::Cancelled;
    }

    std::array<char, 256> buffer;
    std::string originalpath = HyoutaUtils::IO::GetAbsolutePath(videoInfo->GetVideoId(buffer));
    std::string newdirpath(HyoutaUtils::IO::GetDirectoryName(originalpath));
    HyoutaUtils::IO::AppendPathElement(newdirpath,
                                       std::format("{}", DateTime::UtcNow().GetTicks()));
    if (HyoutaUtils::IO::Exists(std::string_view(newdirpath))
        == HyoutaUtils::IO::ExistsResult::DoesExist) {
        std::lock_guard lock(*jobConfig.JobsLock);
        job.TextStatus =
            std::format("File or directory at {} already exists, cancelling.", newdirpath);
        return ResultType::Failure;
    }
    if (!HyoutaUtils::IO::CreateDirectory(std::string_view(newdirpath))) {
        std::lock_guard lock(*jobConfig.JobsLock);
        job.TextStatus = std::format("Failed to create directory at {}, cancelling.", newdirpath);
        return ResultType::Failure;
    }
    std::string inname = newdirpath;
    HyoutaUtils::IO::AppendPathElement(inname, HyoutaUtils::IO::GetFileName(originalpath));
    if (HyoutaUtils::IO::Exists(std::string_view(inname))
        == HyoutaUtils::IO::ExistsResult::DoesExist) {
        std::lock_guard lock(*jobConfig.JobsLock);
        job.TextStatus = std::format("File or directory at {} already exists, cancelling.", inname);
        return ResultType::Failure;
    }
    if (!HyoutaUtils::IO::Move(originalpath, inname, false)) {
        std::lock_guard lock(*jobConfig.JobsLock);
        job.TextStatus = std::format("Moving to {} failed, cancelling.", inname);
        return ResultType::Failure;
    }

    std::string outname = GenerateOutputName(inname, videoInfo->GetVideoId(buffer));

    std::vector<std::string> args;
    args.push_back("-i");
    args.push_back(inname);
    args.push_back("-codec");
    args.push_back("copy");
    args.push_back("-map");
    args.push_back("0");
    args.push_back("-f");
    args.push_back("segment");
    args.push_back("-segment_times");
    args.push_back(splitTimes);
    args.push_back("-segment_start_number");
    args.push_back("1");
    args.push_back("-reset_timestamps");
    args.push_back("1");
    args.push_back(outname);

    std::string exampleOutname = EvaluateOutputName(outname, 0);
    bool existedBefore = HyoutaUtils::IO::Exists(std::string_view(exampleOutname))
                         == HyoutaUtils::IO::ExistsResult::DoesExist;

    {
        auto diskLock = jobConfig.ExpensiveDiskIO.WaitForFreeSlot(cancellationToken);
        if (cancellationToken.IsCancellationRequested()) {
            return ResultType::Cancelled;
        }

        {
            std::lock_guard lock(*jobConfig.JobsLock);
            job.TextStatus = "Splitting...";
        }
        StallWrite(jobConfig,
                   exampleOutname,
                   HyoutaUtils::IO::GetFilesize(std::string_view(inname)).value_or(0),
                   cancellationToken,
                   ShouldStallWriteRegularFile,
                   [&](std::string status) {
                       std::lock_guard lock(*jobConfig.JobsLock);
                       job.TextStatus = std::move(status);
                   });
        int retval = RunProgram(
            "ffmpeg_split.exe", args, [](std::string_view sv) {}, [](std::string_view sv) {});
        if (retval != 0) {
            std::lock_guard lock(*jobConfig.JobsLock);
            job.TextStatus = std::format("ffmpeg_split failed with return value {}", retval);
            return ResultType::Failure;
        }
    }

    {
        std::lock_guard lock(*jobConfig.JobsLock);
        job.TextStatus = "Done!";
        job.JobStatus = VideoJobStatus::Finished;
    }
    return ResultType::Success;
}

ResultType FFMpegSplitJob::Run(JobConfig& jobConfig, TaskCancellation& cancellationToken) {
    return RunSplitJob(*this, jobConfig, cancellationToken);
}

std::string FFMpegSplitJob::GenerateOutputFilename() {
    std::array<char, 256> buffer;
    std::string_view fullPath = VideoInfo->GetVideoId(buffer);
    return GenerateOutputFilenameInternal(fullPath);
}

std::unique_ptr<IVideoJob> FFMpegSplitJob::Clone() const {
    auto clone = std::make_unique<FFMpegSplitJob>();
    clone->TextStatus = this->TextStatus;
    clone->JobStatus = this->JobStatus;
    clone->HasBeenValidated = this->HasBeenValidated;
    clone->VideoInfo = this->VideoInfo ? this->VideoInfo->Clone() : nullptr;
    clone->JobStartTimestamp = this->JobStartTimestamp;
    clone->JobFinishTimestamp = this->JobFinishTimestamp;
    clone->Notes = this->Notes;
    clone->SplitTimes = this->SplitTimes;
    return clone;
}
} // namespace VodArchiver
