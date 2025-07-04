#include "ffmpeg-reencode-job.h"

#include <memory>

#include "util/file.h"

#include "vodarchiver/exec.h"
#include "vodarchiver/ffmpeg_util.h"
#include "vodarchiver/statusupdate/null-status-update.h"
#include "vodarchiver/videoinfo/ffmpeg-reencode-job-video-info.h"
#include "vodarchiver/videoinfo/generic-video-info.h"

namespace VodArchiver {
FFMpegReencodeJob::FFMpegReencodeJob() {}

FFMpegReencodeJob::FFMpegReencodeJob(std::string path,
                                     std::shared_ptr<IStatusUpdate> statusUpdater) {
    JobStatus = VideoJobStatus::NotStarted;
    StatusUpdater =
        statusUpdater == nullptr ? std::make_shared<NullStatusUpdate>() : std::move(statusUpdater);
    auto vi = std::make_unique<GenericVideoInfo>();
    vi->Service = StreamService::FFMpegJob;
    vi->VideoId = std::move(path);
    _VideoInfo = std::move(vi);
    _Status = "...";
}

bool FFMpegReencodeJob::IsWaitingForUserInput() const {
    return false;
}

bool FFMpegReencodeJob::Reencode(const std::string& targetName,
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
                SetStatus(output);
            })
        != 0) {
        return false;
    }

    return HyoutaUtils::IO::Move(tempName, targetName, false);
}

std::string PathCombine(std::string_view lhs, std::string_view rhs) {
    std::string result(lhs);
    HyoutaUtils::IO::AppendPathElement(result, rhs);
    return result;
}

std::string PathCombine(std::string_view lhs, std::string_view mhs, std::string_view rhs) {
    std::string result(lhs);
    HyoutaUtils::IO::AppendPathElement(result, mhs);
    HyoutaUtils::IO::AppendPathElement(result, rhs);
    return result;
}

ResultType FFMpegReencodeJob::Run(TaskCancellation& cancellationToken) {
    JobStatus = VideoJobStatus::Running;
    SetStatus("Checking files...");

    if (cancellationToken.IsCancellationRequested()) {
        return ResultType::Cancelled;
    }

    auto videoInfo = GetVideoInfo();
    std::string file = videoInfo->GetVideoId();
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
        _VideoInfo = std::make_unique<FFMpegReencodeJobVideoInfo>(
            file, *probe, ffmpegOptions, postfixOld, postfixNew, outputformat);
    }

    // if the input file doesn't exist we might still be in a state where we can set this to
    // finished if the output file already exists, so continue anyway

    bool newfileexists = HyoutaUtils::IO::FileExists(std::string_view(newfile));
    bool newfilelocalexists = HyoutaUtils::IO::FileExists(std::string_view(newfileinlocal));
    if (!newfileexists && !newfilelocalexists) {
        if (!encodeinput.has_value()) {
            // neither input nor output exist, bail
            SetStatus("Missing!");
            return ResultType::Failure;
        }

        if (HyoutaUtils::IO::FileExists(std::string_view(tempfile))) {
            if (!HyoutaUtils::IO::DeleteFile(std::string_view(tempfile))) {
                SetStatus("Internal error.");
                return ResultType::Failure;
            }
        }

        if (cancellationToken.IsCancellationRequested()) {
            return ResultType::Cancelled;
        }

        SetStatus("Encoding " + newfile + "...");
        // await StallWrite(newfile, new FileInfo(encodeinput).Length, cancellationToken);
        if (cancellationToken.IsCancellationRequested()) {
            return ResultType::Cancelled;
        }
        if (!HyoutaUtils::IO::CreateDirectory(std::string_view(postfixdir))) {
            SetStatus("Internal error.");
            return ResultType::Failure;
        }
        FFMpegReencodeJobVideoInfo* ffmpegVideoInfo =
            dynamic_cast<FFMpegReencodeJobVideoInfo*>(_VideoInfo.get());
        if (!ffmpegVideoInfo) {
            SetStatus("Internal error.");
            return ResultType::Failure;
        }
        if (!Reencode(newfile, *encodeinput, tempfile, ffmpegVideoInfo->FFMpegOptions)) {
            SetStatus("Internal error.");
            return ResultType::Failure;
        }
    }

    if (!newfileexists && newfilelocalexists) {
        if (!HyoutaUtils::IO::CreateDirectory(std::string_view(postfixdir))) {
            SetStatus("Internal error.");
            return ResultType::Failure;
        }
        if (!HyoutaUtils::IO::Move(newfileinlocal, newfile)) {
            SetStatus("Internal error.");
            return ResultType::Failure;
        }
    }

    if (HyoutaUtils::IO::FileExists(std::string_view(file))
        && !HyoutaUtils::IO::FileExists(std::string_view(oldfileinchunked))) {
        if (!HyoutaUtils::IO::CreateDirectory(std::string_view(chunkeddir))) {
            SetStatus("Internal error.");
            return ResultType::Failure;
        }
        if (!HyoutaUtils::IO::Move(file, oldfileinchunked)) {
            SetStatus("Internal error.");
            return ResultType::Failure;
        }
    }

    SetStatus("Done!");
    JobStatus = VideoJobStatus::Finished;
    return ResultType::Success;
}

std::string FFMpegReencodeJob::GenerateOutputFilename() {
    throw "not implemented";
}
} // namespace VodArchiver
