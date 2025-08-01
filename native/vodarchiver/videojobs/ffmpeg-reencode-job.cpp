#include "ffmpeg-reencode-job.h"

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "util/file.h"
#include "util/text.h"

#include "vodarchiver/exec.h"
#include "vodarchiver/ffmpeg_util.h"
#include "vodarchiver/videoinfo/ffmpeg-reencode-job-video-info.h"
#include "vodarchiver/videoinfo/generic-video-info.h"

namespace VodArchiver {
FFMpegReencodeJob::FFMpegReencodeJob() {}

bool FFMpegReencodeJob::IsWaitingForUserInput() const {
    return false;
}

static bool Reencode(FFMpegReencodeJob& job,
                     JobConfig& jobConfig,
                     const std::string& targetName,
                     const std::string& sourceName,
                     const std::string& tempName,
                     const std::vector<std::string>& options) {
    std::vector<std::string> args;
    args.push_back("-i");
    args.push_back(sourceName);
    args.insert(args.end(), options.begin(), options.end());
    args.push_back(tempName);

    std::string output;
    if (RunProgram(
            "ffmpeg_encode.exe",
            args,
            [](std::string_view sv) {},
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
            })
        != 0) {
        return false;
    }

    return HyoutaUtils::IO::Move(tempName, targetName, false);
}

static std::string PathCombine(std::string_view lhs, std::string_view rhs) {
    std::string result(lhs);
    HyoutaUtils::IO::AppendPathElement(result, rhs);
    return result;
}

static std::string PathCombine(std::string_view lhs, std::string_view mhs, std::string_view rhs) {
    std::string result(lhs);
    HyoutaUtils::IO::AppendPathElement(result, mhs);
    HyoutaUtils::IO::AppendPathElement(result, rhs);
    return result;
}

static ResultType RunReencodeJob(FFMpegReencodeJob& job,
                                 JobConfig& jobConfig,
                                 TaskCancellation& cancellationToken) {
    std::unique_ptr<IVideoInfo> videoInfo;
    {
        std::lock_guard lock(*jobConfig.JobsLock);
        job.JobStatus = VideoJobStatus::Running;
        job.TextStatus = "Checking files...";
        videoInfo = job.VideoInfo->Clone();
    }

    if (cancellationToken.IsCancellationRequested()) {
        return ResultType::Cancelled;
    }

    std::array<char, 256> buffer;
    std::string file(videoInfo->GetVideoId(buffer));
    std::string_view path = HyoutaUtils::IO::GetDirectoryName(file);
    std::string_view name = HyoutaUtils::IO::GetFileNameWithoutExtension(file);

    std::vector<std::string> ffmpegOptions;
    std::string postfixOld;
    std::string postfixNew;
    std::string outputformat;
    if (auto* ffvi = dynamic_cast<const FFMpegReencodeJobVideoInfo*>(videoInfo.get())) {
        ffmpegOptions = ffvi->FFMpegOptions;
        postfixOld = ffvi->PostfixOld;
        postfixNew = ffvi->PostfixNew;
        outputformat = ffvi->OutputFormat;
    } else {
        ffmpegOptions = {{
            "-c:v",
            "libx264",
            "-preset",
            "slower",
            "-crf",
            "23",
            "-g",
            "2000",
            "-c:a",
            "copy",
            "-max_muxing_queue_size",
            "100000",
        }};
        postfixOld = "_chunked";
        postfixNew = "_x264crf23";
        outputformat = "mp4";
    }
    std::string ext = "." + outputformat;

    std::string chunked = postfixOld;
    std::string postfix = postfixNew;
    std::string newfile = PathCombine(
        path, postfix, std::string(name.substr(0, name.size() - chunked.size())) + postfix + ext);
    std::string newfileinlocal = PathCombine(
        path, std::string(name.substr(0, name.size() - chunked.size())) + postfix + ext);
    std::string tempfile = PathCombine(
        path, std::string(name.substr(0, name.size() - chunked.size())) + postfix + "_TEMP" + ext);
    std::string chunkeddir = PathCombine(path, chunked);
    std::string postfixdir = PathCombine(path, postfix);
    std::string oldfileinchunked = PathCombine(chunkeddir, HyoutaUtils::IO::GetFileName(file));

    std::optional<FFProbeResult> probe;
    std::optional<std::string> encodeinput;
    if (HyoutaUtils::IO::FileExists(std::string_view(file))) {
        probe = FFMpegProbe(file);
        encodeinput = file;
    } else if (HyoutaUtils::IO::FileExists(std::string_view(oldfileinchunked))) {
        probe = FFMpegProbe(oldfileinchunked);
        encodeinput = oldfileinchunked;
    }

    if (probe.has_value()) {
        auto newVideoInfo = std::make_unique<FFMpegReencodeJobVideoInfo>(
            file, *probe, ffmpegOptions, postfixOld, postfixNew, outputformat);
        videoInfo = newVideoInfo->Clone();

        std::lock_guard lock(*jobConfig.JobsLock);
        job.VideoInfo = std::move(newVideoInfo);
    }

    // if the input file doesn't exist we might still be in a state where we can set this to
    // finished if the output file already exists, so continue anyway

    bool newfileexists = HyoutaUtils::IO::FileExists(std::string_view(newfile));
    bool newfilelocalexists = HyoutaUtils::IO::FileExists(std::string_view(newfileinlocal));
    if (!newfileexists && !newfilelocalexists) {
        if (!encodeinput.has_value()) {
            // neither input nor output exist, bail
            std::lock_guard lock(*jobConfig.JobsLock);
            job.TextStatus = "Missing!";
            return ResultType::Failure;
        }

        if (HyoutaUtils::IO::FileExists(std::string_view(tempfile))) {
            if (!HyoutaUtils::IO::DeleteFile(std::string_view(tempfile))) {
                std::lock_guard lock(*jobConfig.JobsLock);
                job.TextStatus = "Internal error.";
                return ResultType::Failure;
            }
        }

        if (cancellationToken.IsCancellationRequested()) {
            return ResultType::Cancelled;
        }

        {
            std::lock_guard lock(*jobConfig.JobsLock);
            job.TextStatus = ("Encoding " + newfile + "...");
        }
        StallWrite(jobConfig,
                   newfile,
                   HyoutaUtils::IO::GetFilesize(std::string_view(*encodeinput)).value_or(0),
                   cancellationToken,
                   ShouldStallWriteRegularFile,
                   [&](std::string status) {
                       std::lock_guard lock(*jobConfig.JobsLock);
                       job.TextStatus = std::move(status);
                   });
        if (cancellationToken.IsCancellationRequested()) {
            return ResultType::Cancelled;
        }
        if (!HyoutaUtils::IO::CreateDirectory(std::string_view(postfixdir))) {
            std::lock_guard lock(*jobConfig.JobsLock);
            job.TextStatus = "Internal error.";
            return ResultType::Failure;
        }
        FFMpegReencodeJobVideoInfo* ffmpegVideoInfo =
            dynamic_cast<FFMpegReencodeJobVideoInfo*>(videoInfo.get());
        if (!ffmpegVideoInfo) {
            std::lock_guard lock(*jobConfig.JobsLock);
            job.TextStatus = "Internal error.";
            return ResultType::Failure;
        }
        if (!Reencode(
                job, jobConfig, newfile, *encodeinput, tempfile, ffmpegVideoInfo->FFMpegOptions)) {
            std::lock_guard lock(*jobConfig.JobsLock);
            job.TextStatus = "Internal error.";
            return ResultType::Failure;
        }
    }

    if (!newfileexists && newfilelocalexists) {
        if (!HyoutaUtils::IO::CreateDirectory(std::string_view(postfixdir))) {
            std::lock_guard lock(*jobConfig.JobsLock);
            job.TextStatus = "Internal error.";
            return ResultType::Failure;
        }
        if (!HyoutaUtils::IO::Move(newfileinlocal, newfile)) {
            std::lock_guard lock(*jobConfig.JobsLock);
            job.TextStatus = "Internal error.";
            return ResultType::Failure;
        }
    }

    if (HyoutaUtils::IO::FileExists(std::string_view(file))
        && !HyoutaUtils::IO::FileExists(std::string_view(oldfileinchunked))) {
        if (!HyoutaUtils::IO::CreateDirectory(std::string_view(chunkeddir))) {
            std::lock_guard lock(*jobConfig.JobsLock);
            job.TextStatus = "Internal error.";
            return ResultType::Failure;
        }
        if (!HyoutaUtils::IO::Move(file, oldfileinchunked)) {
            std::lock_guard lock(*jobConfig.JobsLock);
            job.TextStatus = "Internal error.";
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

ResultType FFMpegReencodeJob::Run(JobConfig& jobConfig, TaskCancellation& cancellationToken) {
    return RunReencodeJob(*this, jobConfig, cancellationToken);
}

std::string FFMpegReencodeJob::GenerateOutputFilename() {
    return "not implemented";
}
} // namespace VodArchiver
