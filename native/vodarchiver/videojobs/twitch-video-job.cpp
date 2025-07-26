#include "twitch-video-job.h"

#include <chrono>
#include <format>
#include <string>
#include <thread>
#include <vector>

#include "rapidjson/document.h"

#include "util/file.h"
#include "util/number.h"
#include "util/text.h"

#include "vodarchiver/curl_util.h"
#include "vodarchiver/exec.h"
#include "vodarchiver/ffmpeg_util.h"
#include "vodarchiver/filename_util.h"
#include "vodarchiver/twitch_util.h"
#include "vodarchiver/videoinfo/twitch-video-info.h"

namespace VodArchiver {
namespace {
struct UserInputRequestTimeMismatchCombined : IUserInputRequest {
    std::string Question = "Handle Combined Mismatch";
    std::vector<std::string> Options;
    TwitchVideoJob* Job;

    UserInputRequestTimeMismatchCombined(TwitchVideoJob* job) {
        Job = job;
        Options.push_back("Try again");
        Options.push_back("Accept despite the mismatch");
    }

    const std::string& GetQuestion() const override {
        return Question;
    }

    const std::vector<std::string>& GetOptions() const override {
        return Options;
    }

    void SelectOption(const std::string& option) override {
        if (option == "Accept despite the mismatch") {
            Job->IgnoreTimeDifferenceCombined = true;
        }
        Job->_IsWaitingForUserInput = false;
        Job->_UserInputRequest = nullptr;
    }
};
struct UserInputRequestTimeMismatchRemuxed : IUserInputRequest {
    std::string Question = "Handle Remuxed Mismatch";
    std::vector<std::string> Options;
    TwitchVideoJob* Job;

    UserInputRequestTimeMismatchRemuxed(TwitchVideoJob* job) {
        Job = job;
        Options.push_back("Try again");
        Options.push_back("Accept despite the mismatch");
    }

    const std::string& GetQuestion() const override {
        return Question;
    }

    const std::vector<std::string>& GetOptions() const override {
        return Options;
    }

    void SelectOption(const std::string& option) override {
        if (option == "Accept despite the mismatch") {
            Job->IgnoreTimeDifferenceRemuxed = true;
        }
        Job->_IsWaitingForUserInput = false;
        Job->_UserInputRequest = nullptr;
    }
};
struct UserInputRequestStreamLiveTwitch : IUserInputRequest {
    std::string Question = "Stream still Live";
    std::vector<std::string> Options;
    TwitchVideoJob* Job;

    UserInputRequestStreamLiveTwitch(TwitchVideoJob* job) {
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

static std::string PathCombine(std::string_view lhs, std::string_view rhs) {
    std::string result(lhs);
    HyoutaUtils::IO::AppendPathElement(result, rhs);
    return result;
}

bool TwitchVideoJob::IsWaitingForUserInput() const {
    return _IsWaitingForUserInput;
}

std::shared_ptr<IUserInputRequest> TwitchVideoJob::GetUserInputRequest() const {
    return _UserInputRequest;
}

std::string TwitchVideoJob::GenerateOutputFilename() {
    return GetFinalFilenameWithoutExtension() + ".mp4";
}

std::string TwitchVideoJob::GetTempFilenameWithoutExtension() const {
    auto vi = GetVideoInfo();
    return std::format("twitch_{}_v{}_{}", vi->GetUsername(), vi->GetVideoId(), VideoQuality);
}

std::string TwitchVideoJob::GetFinalFilenameWithoutExtension() const {
    auto vi = GetVideoInfo();
    std::string intercapsGame = MakeIntercapsFilename(vi->GetVideoGame());
    std::string intercapsTitle = MakeIntercapsFilename(vi->GetVideoTitle());
    return std::format("twitch_{}_{}_v{}_{}_{}_{}",
                       vi->GetUsername(),
                       DateTimeToStringForFilesystem(vi->GetVideoTimestamp()),
                       vi->GetVideoId(),
                       intercapsGame,
                       Crop(intercapsTitle, 80),
                       VideoQuality);
}

ResultType TwitchVideoJob::Download(JobConfig& jobConfig,
                                    std::vector<std::string>& files,
                                    TaskCancellation& cancellationToken,
                                    const std::string& targetFolder,
                                    const std::vector<DownloadInfo>& downloadInfos,
                                    int delayPerDownload) {
    files.clear();
    files.reserve(downloadInfos.size());

    HyoutaUtils::IO::CreateDirectory(std::string_view(targetFolder));

    const int MaxTries = 5;
    int triesLeft = MaxTries;
    while (files.size() < downloadInfos.size()) {
        if (triesLeft <= 0) {
            SetStatus(std::format("Failed to download individual parts after {} tries, aborting.",
                                  MaxTries));
            return ResultType::Failure;
        }
        files.clear();
        for (size_t i = 0; i < downloadInfos.size(); ++i) {
            // for (int i = downloadInfos.Count - 1; i >= 0; --i) {
            if (cancellationToken.IsCancellationRequested()) {
                return ResultType::Cancelled;
            }

            const DownloadInfo& downloadInfo = downloadInfos[i];
            std::string outpath = PathCombine(targetFolder, downloadInfo.FilesystemId + ".ts");
            std::string outpath_temp = outpath + ".tmp";
            if (HyoutaUtils::IO::FileExists(std::string_view(outpath_temp))) {
                HyoutaUtils::IO::DeleteFile(std::string_view(outpath_temp));
            }

            if (outpath.ends_with("--2d--muted--2e--ts__.ts")) {
                std::string alt_outpath =
                    outpath.substr(
                        0, outpath.size() - std::string_view("--2d--muted--2e--ts__.ts").size())
                    + "--2e--ts__.ts";
                if (HyoutaUtils::IO::FileExists(std::string_view(alt_outpath))) {
                    if (i % 100 == 99) {
                        SetStatus(
                            std::format("Already have part {}/{}...", i + 1, downloadInfos.size()));
                    }
                    files.push_back(std::move(alt_outpath));
                    continue;
                }
            }

            if (HyoutaUtils::IO::FileExists(std::string_view(outpath))) {
                if (i % 100 == 99) {
                    SetStatus(
                        std::format("Already have part {}/{}...", i + 1, downloadInfos.size()));
                }
                files.push_back(std::move(outpath));
                continue;
            }

            bool success = false;
            {
                SetStatus(std::format(
                    "Downloading files... ({}/{})", files.size() + 1, downloadInfos.size()));

                std::vector<VodArchiver::curl::Range> ranges;
                if (downloadInfo.Length && downloadInfo.Offset) {
                    ranges.reserve(1);
                    ranges.push_back(VodArchiver::curl::Range{.Start = *downloadInfo.Offset,
                                                              .End = *downloadInfo.Offset
                                                                     + *downloadInfo.Length - 1});
                }
                auto data = VodArchiver::curl::GetFromUrlToMemory(
                    downloadInfo.Url, std::vector<std::string>(), ranges);
                if (!data || data->ResponseCode >= 400) {
                    continue;
                }

                StallWrite(jobConfig,
                           outpath_temp,
                           data->Data.size(),
                           cancellationToken,
                           ShouldStallWriteRegularFile,
                           [this](std::string status) { SetStatus(std::move(status)); });
                if (!HyoutaUtils::IO::WriteFileAtomic(
                        std::string_view(outpath_temp), data->Data.data(), data->Data.size())) {
                    return ResultType::Failure;
                }
                if (cancellationToken.IsCancellationRequested()) {
                    return ResultType::Cancelled;
                }
                success = true;
            }

            if (success) {
                StallWrite(jobConfig,
                           outpath,
                           HyoutaUtils::IO::GetFilesize(std::string_view(outpath_temp)).value_or(0),
                           cancellationToken,
                           ShouldStallWriteRegularFile,
                           [this](std::string status) { SetStatus(std::move(status)); });
                if (cancellationToken.IsCancellationRequested()) {
                    return ResultType::Cancelled;
                }
                HyoutaUtils::IO::Move(outpath_temp, outpath, false);
                files.push_back(std::move(outpath));
            }

            if (delayPerDownload > 0) {
                if (!cancellationToken.DelayFor(std::chrono::milliseconds(delayPerDownload))) {
                    return ResultType::Cancelled;
                }
            }
        }

        if (files.size() < downloadInfos.size()) {
            if (!cancellationToken.DelayFor(std::chrono::seconds(60))) {
                return ResultType::Cancelled;
            }
            --triesLeft;
        }
    }

    return ResultType::Success;
}

ResultType TwitchVideoJob::Combine(TaskCancellation& cancellationToken,
                                   const std::string& combinedFilename,
                                   const std::vector<std::string>& files) {
    // Console.WriteLine("Combining into " + combinedFilename + "...");
    std::string tempname = combinedFilename + ".tmp";
    HyoutaUtils::IO::File fs(std::string_view(tempname), HyoutaUtils::IO::OpenMode::Write);
    if (!fs.IsOpen()) {
        return ResultType::Failure;
    }
    for (auto& file : files) {
        if (cancellationToken.IsCancellationRequested()) {
            fs.Delete();
            return ResultType::Cancelled;
        }
        HyoutaUtils::IO::File part(std::string_view(file), HyoutaUtils::IO::OpenMode::Read);
        if (!part.IsOpen()) {
            return ResultType::Failure;
        }
        auto len = part.GetLength();
        if (!len) {
            return ResultType::Failure;
        }
        if (fs.Write(part, *len) != *len) {
            return ResultType::Failure;
        }
    }
    if (!fs.Rename(std::string_view(combinedFilename))) {
        return ResultType::Failure;
    }
    return ResultType::Success;
}

bool TwitchVideoJob::Remux(const std::string& targetName,
                           const std::string& sourceName,
                           const std::string& tempName) {
    HyoutaUtils::IO::CreateDirectory(HyoutaUtils::IO::GetDirectoryName(targetName));
    // Console.WriteLine("Remuxing to " + targetName + "...");
    RunProgram(
        "ffmpeg_remux.exe",
        {{"-i", sourceName, "-codec", "copy", "-bsf:a", "aac_adtstoasc", tempName}},
        [&](std::string_view) {},
        [&](std::string_view) {});
    if (!HyoutaUtils::IO::Move(tempName, targetName, true)) {
        return false;
    }
    // Console.WriteLine("Created " + targetName + "!");
    return true;
}

static std::optional<std::string> TryGetUserCopyBaseurlM3U(const std::string& tmp) {
    HyoutaUtils::IO::File f(std::string_view(tmp), HyoutaUtils::IO::OpenMode::Read);
    if (!f.IsOpen()) {
        return std::nullopt;
    }
    auto len = f.GetLength();
    if (!len) {
        return std::nullopt;
    }
    std::string lines;
    lines.resize(*len);
    if (f.Read(lines.data(), lines.size()) != lines.size()) {
        return std::nullopt;
    }
    if (lines.find("vod-secure.twitch.tv") != std::string::npos
        || lines.find("cloudfront.net") != std::string::npos) {
        return lines;
    }
    return std::nullopt;
}

static std::optional<std::string> TryGetUserCopyTsnamesM3U(const std::string& tmp) {
    HyoutaUtils::IO::File f(std::string_view(tmp), HyoutaUtils::IO::OpenMode::Read);
    if (!f.IsOpen()) {
        return std::nullopt;
    }
    auto len = f.GetLength();
    if (!len) {
        return std::nullopt;
    }
    std::string lines;
    lines.resize(*len);
    if (f.Read(lines.data(), lines.size()) != lines.size()) {
        return std::nullopt;
    }
    if (lines.find(".ts") != std::string::npos || lines.find("autogen:") != std::string::npos) {
        return lines;
    }
    return std::nullopt;
}

static std::optional<std::string> GetM3U8PathFromM3U(const std::string& m3u,
                                                     const std::string& videoType) {
    std::string_view rest = m3u;
    if (rest.starts_with("\xef\xbb\xbf")) { // utf8 bom
        rest = rest.substr(3);
    }
    while (!rest.empty()) {
        const size_t nextLineSeparator = rest.find_first_of("\r\n");
        std::string_view line = rest.substr(0, nextLineSeparator);
        rest = nextLineSeparator != std::string_view::npos ? rest.substr(nextLineSeparator + 1)
                                                           : std::string_view();
        line = HyoutaUtils::TextUtils::Trim(line);
        if (line.empty()) {
            continue;
        }
        if (line.starts_with('#')) {
            continue;
        }

        auto lastSlash = line.rfind('/');
        if (lastSlash == std::string_view::npos || lastSlash == 0) {
            continue;
        }
        auto secondToLastSlash = line.rfind('/', lastSlash - 1);
        if (secondToLastSlash == std::string_view::npos) {
            continue;
        }

        return std::format(
            "{}/{}/{}", line.substr(0, secondToLastSlash), videoType, line.substr(lastSlash));
    }
    return std::nullopt;
}

static std::string GetFolder(const std::string& m3u8path) {
    std::string_view trimmed = HyoutaUtils::TextUtils::Trim(m3u8path);
    auto lastSlash = trimmed.rfind('/');
    if (lastSlash == std::string_view::npos) {
        return std::string(trimmed);
    }
    return std::string(trimmed.substr(0, lastSlash + 1));
}

static std::optional<std::string> ExtractM3u8FromJson(const std::string& str) {
    rapidjson::Document json;
    json.Parse<rapidjson::kParseFullPrecisionFlag | rapidjson::kParseNanAndInfFlag
                   | rapidjson::kParseCommentsFlag,
               rapidjson::UTF8<char>>(str.data(), str.size());
    if (json.HasParseError() || !json.IsObject()) {
        return std::nullopt;
    }
    const auto jo = json.GetObject();

    auto urlIt = jo.FindMember("url");
    if (urlIt != jo.MemberEnd() && urlIt->value.IsString()) {
        std::string_view url(urlIt->value.GetString(), urlIt->value.GetStringLength());
        if (!url.empty()) {
            return std::string(url);
        }
    }

    auto formatsIt = jo.FindMember("formats");
    if (formatsIt != jo.MemberEnd() && formatsIt->value.IsArray()) {
        for (const auto& jsonval : formatsIt->value.GetArray()) {
            if (jsonval.IsObject()) {
                const auto o = jsonval.GetObject();
                auto formatNoteIt = o.FindMember("format_note");
                if (formatNoteIt != o.MemberEnd() && formatNoteIt->value.IsString()
                    && std::string_view(formatNoteIt->value.GetString(),
                                        formatNoteIt->value.GetStringLength())
                           == "Source") {
                    auto urlIt2 = o.FindMember("url");
                    if (urlIt2 != jo.MemberEnd() && urlIt2->value.IsString()) {
                        std::string_view url(urlIt2->value.GetString(),
                                             urlIt2->value.GetStringLength());
                        if (!url.empty()) {
                            return std::string(url);
                        }
                    }
                }
            }
        }
    }

    return std::nullopt;
}

static std::string GenerateFilesystemName(std::string_view l,
                                          std::optional<int64_t> length,
                                          std::optional<int64_t> offset) {
    std::string len = length.has_value() ? std::format("{}", *length) : "";
    std::string off = offset.has_value() ? std::format("{}", *offset) : "";
    return std::format("{}_{}_{}", FileSystemEscapeName(l), len, off);
}

void TwitchVideoJob::GetFilenamesFromM3U8(std::vector<DownloadInfo>& downloadInfos,
                                          const std::string& baseurl,
                                          const std::string& m3u8) {
    downloadInfos.clear();
    std::string_view rest = m3u8;
    if (rest.starts_with("\xef\xbb\xbf")) { // utf8 bom
        rest = rest.substr(3);
    }
    std::optional<uint64_t> offset;
    std::optional<uint64_t> length;
    while (!rest.empty()) {
        const size_t nextLineSeparator = rest.find_first_of("\r\n");
        std::string_view l = rest.substr(0, nextLineSeparator);
        rest = nextLineSeparator != std::string_view::npos ? rest.substr(nextLineSeparator + 1)
                                                           : std::string_view();
        l = HyoutaUtils::TextUtils::Trim(l);
        if (l.empty()) {
            continue;
        }

        if (l.starts_with("#EXT-X-BYTERANGE:")) {
            auto ll = l.substr(sizeof("#EXT-X-BYTERANGE:") - 1);
            auto at = ll.find('@');
            if (at != std::string_view::npos) {
                length = HyoutaUtils::NumberUtils::ParseUInt64(ll.substr(0, at));
                offset = HyoutaUtils::NumberUtils::ParseUInt64(ll.substr(at + 1));
            }
            continue;
        }
        if (l.starts_with('#')) {
            continue;
        }

        downloadInfos.emplace_back(
            DownloadInfo{.Url = std::format("{}{}", baseurl, l),
                         .FilesystemId = GenerateFilesystemName(l, length, offset),
                         .Offset = offset,
                         .Length = length});
        length = std::nullopt;
        offset = std::nullopt;
    }
}

ResultType TwitchVideoJob::GetFileUrlsOfVod(std::vector<DownloadInfo>& downloadInfos,
                                            const std::string& targetFolderPath,
                                            const std::string& tempFolderPath,
                                            TaskCancellation& cancellationToken) {
    downloadInfos.clear();

    auto videoId = HyoutaUtils::NumberUtils::ParseInt64(GetVideoInfo()->GetVideoId());
    if (!videoId) {
        return ResultType::Failure;
    }
    auto video_json = VodArchiver::TwitchYTDL::GetVideoJson(*videoId);
    if (!video_json) {
        return ResultType::Failure;
    }
    auto twitchVideoFromJson = VodArchiver::TwitchYTDL::VideoFromJson(*video_json);
    if (!twitchVideoFromJson) {
        return ResultType::Failure;
    }
    auto videoInfo = std::make_shared<TwitchVideoInfo>();
    videoInfo->_Service = StreamService::Twitch;
    videoInfo->_Video = *twitchVideoFromJson;
    SetVideoInfo(videoInfo);

    std::string folderpath;
    while (true) {
        bool interactive = false;
        if (interactive) {
            SetStatus("");
            std::string tmp1 =
                PathCombine(tempFolderPath, GetTempFilenameWithoutExtension() + "_baseurl.txt");
            std::string tmp2 =
                PathCombine(tempFolderPath, GetTempFilenameWithoutExtension() + "_tsnames.txt");
            std::optional<std::string> linesbaseurl = TryGetUserCopyBaseurlM3U(tmp1);
            std::optional<std::string> linestsnames = TryGetUserCopyTsnamesM3U(tmp2);
            if (!linesbaseurl) {
                std::string d =
                    std::format("# get baseurl for m3u8 from https://www.twitch.tv/videos/{}",
                                videoInfo->GetVideoId());
                HyoutaUtils::IO::WriteFileAtomic(tmp1, d.data(), d.size());
            }
            if (!linestsnames) {
                std::string d =
                    std::format("# get actual m3u file from https://www.twitch.tv/videos/{}",
                                videoInfo->GetVideoId());
                HyoutaUtils::IO::WriteFileAtomic(tmp2, d.data(), d.size());
            }
            if (!linesbaseurl || !linestsnames) {
                return ResultType::UserInputRequired;
            }

            auto m3u8path = GetM3U8PathFromM3U(*linesbaseurl, VideoQuality);
            if (!m3u8path) {
                return ResultType::Failure;
            }
            folderpath = GetFolder(*m3u8path);
            GetFilenamesFromM3U8(downloadInfos, folderpath, *linestsnames);
        } else {
            auto m3u8path = ExtractM3u8FromJson(*video_json);
            if (!m3u8path) {
                return ResultType::Failure;
            }
            folderpath = GetFolder(*m3u8path);
            auto result = VodArchiver::curl::GetFromUrlToMemory(*m3u8path);
            if (!result) {
                return ResultType::Failure;
            }
            if (result->ResponseCode == 404
                && videoInfo->GetVideoRecordingState() == RecordingState::Live) {
                // this can happen on streams that have just started, in this just wait a bit
                // and retry
                if (!cancellationToken.DelayFor(std::chrono::seconds(20))) {
                    return ResultType::Cancelled;
                }
                video_json = VodArchiver::TwitchYTDL::GetVideoJson(*videoId);
                if (!video_json) {
                    return ResultType::Failure;
                }
                twitchVideoFromJson = VodArchiver::TwitchYTDL::VideoFromJson(*video_json);
                if (!twitchVideoFromJson) {
                    return ResultType::Failure;
                }
                videoInfo = std::make_shared<TwitchVideoInfo>();
                videoInfo->_Service = StreamService::Twitch;
                videoInfo->_Video = *twitchVideoFromJson;
                SetVideoInfo(videoInfo);
                continue;
            }
            std::string m3u8(result->Data.data(), result->Data.size());
            GetFilenamesFromM3U8(downloadInfos, folderpath, m3u8);
        }
        break;
    }

    return ResultType::Success;
}

ResultType TwitchVideoJob::Run(JobConfig& jobConfig, TaskCancellation& cancellationToken) {
    if (cancellationToken.IsCancellationRequested()) {
        return ResultType::Cancelled;
    }

    JobStatus = VideoJobStatus::Running;
    SetStatus("Retrieving video info...");

    std::string tempFolderPath;
    std::string targetFolderPath;
    {
        std::lock_guard lock(jobConfig.Mutex);
        tempFolderPath = jobConfig.TempFolderPath;
        targetFolderPath = jobConfig.TargetFolderPath;
    }

    std::vector<DownloadInfo> downloadInfos;
    ResultType getFileUrlsResult =
        GetFileUrlsOfVod(downloadInfos, targetFolderPath, tempFolderPath, cancellationToken);
    if (getFileUrlsResult == ResultType::UserInputRequired) {
        SetStatus("Need manual fetch of file URLs.");
        return ResultType::UserInputRequired;
    }
    if (getFileUrlsResult != ResultType::Success) {
        SetStatus("Failed retrieving file URLs.");
        return ResultType::Failure;
    }

    std::string combinedTempname =
        PathCombine(tempFolderPath, GetTempFilenameWithoutExtension() + "_combined.ts");
    std::string combinedFilename =
        PathCombine(tempFolderPath, GetTempFilenameWithoutExtension() + "_f.ts");
    std::string remuxedTempname =
        PathCombine(tempFolderPath, GetTempFilenameWithoutExtension() + "_combined.mp4");
    std::string remuxedFilename =
        PathCombine(tempFolderPath, GetTempFilenameWithoutExtension() + "_f.mp4");
    std::string targetFilename =
        PathCombine(targetFolderPath, GetFinalFilenameWithoutExtension() + ".mp4");
    std::string baseurlfilepath =
        PathCombine(tempFolderPath, GetTempFilenameWithoutExtension() + "_baseurl.txt");
    std::string tsnamesfilepath =
        PathCombine(tempFolderPath, GetTempFilenameWithoutExtension() + "_tsnames.txt");
    std::string tempFolderForParts = PathCombine(tempFolderPath, GetTempFilenameWithoutExtension());

    if (!HyoutaUtils::IO::FileExists(std::string_view(targetFilename))) {
        if (!HyoutaUtils::IO::FileExists(std::string_view(combinedFilename))) {
            if (cancellationToken.IsCancellationRequested()) {
                return ResultType::Cancelled;
            }

            SetStatus("Downloading files...");
            std::vector<std::string> files;
            while (true) {
                if (cancellationToken.IsCancellationRequested()) {
                    return ResultType::Cancelled;
                }


                std::chrono::steady_clock timer;
                auto timerBegin = timer.now();
                ResultType downloadResult = Download(
                    jobConfig, files, cancellationToken, tempFolderForParts, downloadInfos);
                if (downloadResult != ResultType::Success) {
                    return downloadResult;
                }
                if (this->AssumeFinished
                    || this->GetVideoInfo()->GetVideoRecordingState() == RecordingState::Recorded) {
                    break;
                } else {
                    // we're downloading a stream that is still streaming
                    auto timerEnd = timer.now();
                    auto timerDifference = timerEnd - timerBegin;
                    // if too little time has passed wait a bit to allow the stream to provide new
                    // data
                    if (timerDifference < std::chrono::seconds(150)) {
                        auto ts = std::chrono::seconds(150) - timerDifference;
                        const std::chrono::duration<double, std::ratio<1, 1>> tsDouble = ts;
                        SetStatus(std::format("Waiting {} seconds for stream to update...",
                                              tsDouble.count()));
                        _UserInputRequest =
                            std::make_unique<UserInputRequestStreamLiveTwitch>(this);
                        if (!cancellationToken.DelayFor(ts)) {
                            return ResultType::Cancelled;
                        }
                    }
                    if (HyoutaUtils::IO::FileExists(std::string_view(tsnamesfilepath))) {
                        HyoutaUtils::IO::DeleteFile(std::string_view(tsnamesfilepath));
                    }
                    getFileUrlsResult = GetFileUrlsOfVod(
                        downloadInfos, targetFolderPath, tempFolderPath, cancellationToken);
                    if (getFileUrlsResult != ResultType::Success) {
                        SetStatus("Failed retrieving file URLs.");
                        return ResultType::Failure;
                    }
                }
            }
            _UserInputRequest = nullptr;

            SetStatus("Waiting for free disk IO slot to combine...");
            {
                auto diskLock = jobConfig.ExpensiveDiskIO.WaitForFreeSlot(cancellationToken);
                if (cancellationToken.IsCancellationRequested()) {
                    return ResultType::Cancelled;
                }
                uint64_t expectedTargetFilesize = 0;
                for (const auto& file : files) {
                    expectedTargetFilesize +=
                        HyoutaUtils::IO::GetFilesize(std::string_view(file)).value_or(0);
                }

                SetStatus("Combining downloaded video parts...");
                if (HyoutaUtils::IO::FileExists(std::string_view(combinedTempname))) {
                    HyoutaUtils::IO::DeleteFile(std::string_view(combinedTempname));
                }
                StallWrite(jobConfig,
                           combinedFilename,
                           expectedTargetFilesize,
                           cancellationToken,
                           ShouldStallWriteRegularFile,
                           [this](std::string status) { SetStatus(std::move(status)); });
                if (cancellationToken.IsCancellationRequested()) {
                    return ResultType::Cancelled;
                }
                ResultType combineResult = Combine(cancellationToken, combinedTempname, files);
                if (combineResult != ResultType::Success) {
                    return combineResult;
                }

                // sanity check
                SetStatus("Sanity check on combined video...");
                auto probe = FFMpegProbe(combinedTempname);
                if (!probe) {
                    return ResultType::Failure;
                }
                TimeSpan actualVideoLength = probe->Duration;
                TimeSpan expectedVideoLength = GetVideoInfo()->GetVideoLength();
                if (!IgnoreTimeDifferenceCombined
                    && std::abs((actualVideoLength - expectedVideoLength).GetTotalSeconds())
                           > 5.0) {
                    // if difference is bigger than 5 seconds something is off, report
                    SetStatus(
                        std::format("Large time difference between expected ({}s) and combined "
                                    "({}s), stopping.",
                                    expectedVideoLength.GetTotalSeconds(),
                                    actualVideoLength.GetTotalSeconds()));
                    _UserInputRequest =
                        std::make_unique<UserInputRequestTimeMismatchCombined>(this);
                    _IsWaitingForUserInput = true;
                    return ResultType::UserInputRequired;
                }

                HyoutaUtils::IO::Move(combinedTempname, combinedFilename, true);
                for (const auto& file : files) {
                    HyoutaUtils::IO::DeleteFile(std::string_view(file));
                }
                HyoutaUtils::IO::DeleteDirectory(std::string_view(tempFolderForParts));
            }
        }

        SetStatus("Waiting for free disk IO slot to remux...");
        {
            auto diskLock = jobConfig.ExpensiveDiskIO.WaitForFreeSlot(cancellationToken);
            if (cancellationToken.IsCancellationRequested()) {
                return ResultType::Cancelled;
            }
            SetStatus("Remuxing to MP4...");
            if (HyoutaUtils::IO::FileExists(std::string_view(remuxedTempname))) {
                HyoutaUtils::IO::DeleteFile(std::string_view(remuxedTempname));
            }
            StallWrite(jobConfig,
                       remuxedFilename,
                       HyoutaUtils::IO::GetFilesize(std::string_view(combinedFilename)).value_or(0),
                       cancellationToken,
                       ShouldStallWriteRegularFile,
                       [this](std::string status) { SetStatus(std::move(status)); });
            if (cancellationToken.IsCancellationRequested()) {
                return ResultType::Cancelled;
            }
            if (!Remux(remuxedFilename, combinedFilename, remuxedTempname)) {
                return ResultType::Failure;
            }

            // sanity check
            SetStatus("Sanity check on remuxed video...");
            auto probe = FFMpegProbe(remuxedFilename);
            if (!probe) {
                return ResultType::Failure;
            }
            TimeSpan actualVideoLength = probe->Duration;
            TimeSpan expectedVideoLength = GetVideoInfo()->GetVideoLength();
            if (!IgnoreTimeDifferenceRemuxed
                && std::abs((actualVideoLength - expectedVideoLength).GetTotalSeconds()) > 5.0) {
                // if difference is bigger than 5 seconds something is off, report
                SetStatus(std::format(
                    "Large time difference between expected ({}s) and remuxed ({}s), stopping.",
                    expectedVideoLength.GetTotalSeconds(),
                    actualVideoLength.GetTotalSeconds()));
                _UserInputRequest = std::make_unique<UserInputRequestTimeMismatchRemuxed>(this);
                _IsWaitingForUserInput = true;
                return ResultType::UserInputRequired;
            }

            HyoutaUtils::IO::Move(remuxedFilename, targetFilename, true);
            HyoutaUtils::IO::DeleteFile(std::string_view(combinedFilename));
        }
    }

    SetStatus("Done!");
    JobStatus = VideoJobStatus::Finished;
    if (HyoutaUtils::IO::FileExists(std::string_view(tsnamesfilepath))) {
        HyoutaUtils::IO::DeleteFile(std::string_view(tsnamesfilepath));
    }
    if (HyoutaUtils::IO::FileExists(std::string_view(baseurlfilepath))) {
        HyoutaUtils::IO::DeleteFile(std::string_view(baseurlfilepath));
    }
    return ResultType::Success;
}
} // namespace VodArchiver
