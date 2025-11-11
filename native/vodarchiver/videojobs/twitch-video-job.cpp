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
struct DownloadInfo {
    std::string Url;
    std::string FilesystemId;
    std::optional<uint64_t> Offset;
    std::optional<uint64_t> Length;
};
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
        Job->WaitingForUserInput = false;
        Job->UserInputRequest = nullptr;
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
        Job->WaitingForUserInput = false;
        Job->UserInputRequest = nullptr;
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
    return WaitingForUserInput;
}

IUserInputRequest* TwitchVideoJob::GetUserInputRequest() const {
    return UserInputRequest.get();
}

static std::string GetTempFilenameWithoutExtension(IVideoInfo& vi,
                                                   const std::string& videoQuality) {
    std::array<char, 256> buffer1;
    std::array<char, 256> buffer2;
    return std::format(
        "twitch_{}_v{}_{}", vi.GetUsername(buffer1), vi.GetVideoId(buffer2), videoQuality);
}

static std::string GetFinalFilenameWithoutExtension(IVideoInfo& vi,
                                                    const std::string& videoQuality) {
    std::array<char, 256> buffer1;
    std::array<char, 256> buffer2;
    std::string intercapsGame = MakeIntercapsFilename(vi.GetVideoGame(buffer1));
    std::string intercapsTitle = MakeIntercapsFilename(vi.GetVideoTitle(buffer2));
    return std::format("twitch_{}_{}_v{}_{}_{}_{}",
                       vi.GetUsername(buffer1),
                       DateTimeToStringForFilesystem(vi.GetVideoTimestamp()),
                       vi.GetVideoId(buffer2),
                       intercapsGame,
                       Crop(intercapsTitle, 80),
                       videoQuality);
}

static ResultType Download(TwitchVideoJob& job,
                           JobConfig& jobConfig,
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
            std::lock_guard lock(*jobConfig.JobsLock);
            job.TextStatus = std::format(
                "Failed to download individual parts after {} tries, aborting.", MaxTries);
            return ResultType::NetworkError;
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
            HyoutaUtils::IO::DeleteFile(std::string_view(outpath_temp));

            if (outpath.ends_with("--2d--muted--2e--ts__.ts")) {
                std::string alt_outpath =
                    outpath.substr(
                        0, outpath.size() - std::string_view("--2d--muted--2e--ts__.ts").size())
                    + "--2e--ts__.ts";
                if (HyoutaUtils::IO::FileExists(std::string_view(alt_outpath))
                    == HyoutaUtils::IO::ExistsResult::DoesExist) {
                    if (i % 100 == 99) {
                        std::lock_guard lock(*jobConfig.JobsLock);
                        job.TextStatus =
                            std::format("Already have part {}/{}...", i + 1, downloadInfos.size());
                    }
                    files.push_back(std::move(alt_outpath));
                    continue;
                }
            }

            if (HyoutaUtils::IO::FileExists(std::string_view(outpath))
                == HyoutaUtils::IO::ExistsResult::DoesExist) {
                if (i % 100 == 99) {
                    std::lock_guard lock(*jobConfig.JobsLock);
                    job.TextStatus =
                        std::format("Already have part {}/{}...", i + 1, downloadInfos.size());
                }
                files.push_back(std::move(outpath));
                continue;
            }

            bool success = false;
            {
                std::lock_guard lock(*jobConfig.JobsLock);
                job.TextStatus = std::format(
                    "Downloading files... ({}/{})", files.size() + 1, downloadInfos.size());
            }
            {
                std::vector<VodArchiver::curl::Range> ranges;
                if (downloadInfo.Length && downloadInfo.Offset) {
                    ranges.reserve(1);
                    ranges.push_back(VodArchiver::curl::Range{.Start = *downloadInfo.Offset,
                                                              .End = *downloadInfo.Offset
                                                                     + *downloadInfo.Length - 1});
                }
                auto data = VodArchiver::curl::GetFromUrlToMemory(
                    downloadInfo.Url, std::vector<std::string>(), ranges);
                if (!data || data->ResponseCode != 200) {
                    continue;
                }

                StallWrite(jobConfig,
                           outpath_temp,
                           data->Data.size(),
                           cancellationToken,
                           ShouldStallWriteRegularFile,
                           [&](std::string status) {
                               std::lock_guard lock(*jobConfig.JobsLock);
                               job.TextStatus = std::move(status);
                           });
                if (!HyoutaUtils::IO::WriteFileAtomic(
                        std::string_view(outpath_temp), data->Data.data(), data->Data.size())) {
                    std::lock_guard lock(*jobConfig.JobsLock);
                    job.TextStatus = std::format("Failed to write {}", outpath_temp);
                    return ResultType::IOError;
                }
                success = true;
            }

            if (success) {
                StallWrite(jobConfig,
                           outpath,
                           HyoutaUtils::IO::GetFilesize(std::string_view(outpath_temp)).value_or(0),
                           cancellationToken,
                           ShouldStallWriteRegularFile,
                           [&](std::string status) {
                               std::lock_guard lock(*jobConfig.JobsLock);
                               job.TextStatus = std::move(status);
                           });
                HyoutaUtils::IO::Move(outpath_temp, outpath, false);
                files.push_back(std::move(outpath));
            }

            if (cancellationToken.IsCancellationRequested()) {
                return ResultType::Cancelled;
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

static ResultType Combine(TaskCancellation& cancellationToken,
                          const std::string& combinedFilename,
                          const std::vector<std::string>& files) {
    // Console.WriteLine("Combining into " + combinedFilename + "...");
    std::string tempname = combinedFilename + ".tmp";
    HyoutaUtils::IO::File fs(std::string_view(tempname), HyoutaUtils::IO::OpenMode::Write);
    if (!fs.IsOpen()) {
        return ResultType::IOError;
    }
    for (auto& file : files) {
        if (cancellationToken.IsCancellationRequested()) {
            fs.Delete();
            return ResultType::Cancelled;
        }
        HyoutaUtils::IO::File part(std::string_view(file), HyoutaUtils::IO::OpenMode::Read);
        if (!part.IsOpen()) {
            return ResultType::IOError;
        }
        auto len = part.GetLength();
        if (!len) {
            return ResultType::IOError;
        }
        if (fs.Write(part, *len) != *len) {
            return ResultType::IOError;
        }
    }
    if (!fs.Rename(std::string_view(combinedFilename))) {
        return ResultType::IOError;
    }
    return ResultType::Success;
}

static bool Remux(const std::string& targetName,
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
            "{}/{}/{}", line.substr(0, secondToLastSlash), videoType, line.substr(lastSlash + 1));
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
                                          std::optional<uint64_t> length,
                                          std::optional<uint64_t> offset) {
    std::string len = length.has_value() ? std::format("{}", *length) : "";
    std::string off = offset.has_value() ? std::format("{}", *offset) : "";
    return std::format("{}_{}_{}", FileSystemEscapeName(l), len, off);
}

static void GetFilenamesFromM3U8(std::vector<DownloadInfo>& downloadInfos,
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

void MergeTwitchVideoInfo(TwitchVideoInfo* newVideoInfo, IVideoInfo* oldVideoInfo) {
    if (!newVideoInfo || !oldVideoInfo) {
        return;
    }

    // copy important info from the existing object if we don't have it
    if (!newVideoInfo->Video.Username.has_value()) {
        std::array<char, 256> buffer;
        std::string_view sv = oldVideoInfo->GetUsername(buffer);
        if (!sv.empty()) {
            newVideoInfo->Video.Username = std::string(sv);
        }
    }
    if (!newVideoInfo->Video.Title.has_value() || newVideoInfo->Video.Title == "Untitled Broadcast") {
        std::array<char, 256> buffer;
        std::string_view sv = oldVideoInfo->GetVideoTitle(buffer);
        if (!sv.empty()) {
            newVideoInfo->Video.Title = std::string(sv);
        }
    }
    if (!newVideoInfo->Video.Game.has_value()) {
        std::array<char, 256> buffer;
        std::string_view sv = oldVideoInfo->GetVideoGame(buffer);
        if (!sv.empty()) {
            newVideoInfo->Video.Game = std::string(sv);
        }
    }
    if (!newVideoInfo->Video.Description.has_value()) {
        auto* oldTvi = dynamic_cast<TwitchVideoInfo*>(oldVideoInfo);
        if (oldTvi && oldTvi->Video.Description.has_value()) {
            newVideoInfo->Video.Description = oldTvi->Video.Description;
        }
    }
    if (!newVideoInfo->Video.CreatedAt.has_value()) {
        auto* oldTvi = dynamic_cast<TwitchVideoInfo*>(oldVideoInfo);
        if (oldTvi && oldTvi->Video.CreatedAt.has_value()) {
            newVideoInfo->Video.CreatedAt = oldTvi->Video.CreatedAt;
        }
    }
    if (!newVideoInfo->Video.PublishedAt.has_value()) {
        auto* oldTvi = dynamic_cast<TwitchVideoInfo*>(oldVideoInfo);
        if (oldTvi && oldTvi->Video.PublishedAt.has_value()) {
            newVideoInfo->Video.PublishedAt = oldTvi->Video.PublishedAt;
        }
    }
    if (newVideoInfo->Video.Duration == 0) {
        auto* oldTvi = dynamic_cast<TwitchVideoInfo*>(oldVideoInfo);
        if (oldTvi) {
            newVideoInfo->Video.Duration = oldTvi->Video.Duration;
        }
    }
    if (newVideoInfo->Video.State == RecordingState::Unknown) {
        auto* oldTvi = dynamic_cast<TwitchVideoInfo*>(oldVideoInfo);
        if (oldTvi) {
            newVideoInfo->Video.State = oldTvi->Video.State;
        }
    }
}

static ResultType GetFileUrlsOfVod(TwitchVideoJob& job,
                                   JobConfig& jobConfig,
                                   std::unique_ptr<IVideoInfo>& videoInfo,
                                   std::vector<DownloadInfo>& downloadInfos,
                                   const std::string& tempFolderPath,
                                   TaskCancellation& cancellationToken,
                                   const std::string& videoQuality) {
    downloadInfos.clear();

    std::array<char, 256> buffer;
    auto videoId = HyoutaUtils::NumberUtils::ParseInt64(videoInfo->GetVideoId(buffer));
    if (!videoId) {
        return ResultType::Failure;
    }
    auto video_json = VodArchiver::TwitchYTDL::GetVideoJson(*videoId);
    if (!video_json) {
        return ResultType::NetworkError;
    }
    auto twitchVideoFromJson = VodArchiver::TwitchYTDL::VideoFromJson(*video_json);
    if (!twitchVideoFromJson) {
        return ResultType::NetworkError;
    }
    {
        auto newVideoInfo = std::make_unique<TwitchVideoInfo>();
        newVideoInfo->Service = StreamService::Twitch;
        newVideoInfo->Video = *twitchVideoFromJson;
        videoInfo = newVideoInfo->Clone();

        std::lock_guard lock(*jobConfig.JobsLock);
        MergeTwitchVideoInfo(newVideoInfo.get(), job.VideoInfo.get());
        job.VideoInfo = std::move(newVideoInfo);
    }

    std::string folderpath;
    while (true) {
        bool interactive = false;
        if (interactive) {
            std::string tempFilenameWithoutExt =
                GetTempFilenameWithoutExtension(*videoInfo, videoQuality);
            std::string tmp1 = PathCombine(tempFolderPath, tempFilenameWithoutExt + "_baseurl.txt");
            std::string tmp2 = PathCombine(tempFolderPath, tempFilenameWithoutExt + "_tsnames.txt");
            std::optional<std::string> linesbaseurl = TryGetUserCopyBaseurlM3U(tmp1);
            std::optional<std::string> linestsnames = TryGetUserCopyTsnamesM3U(tmp2);
            if (!linesbaseurl) {
                std::string d =
                    std::format("# get baseurl for m3u8 from https://www.twitch.tv/videos/{}",
                                videoInfo->GetVideoId(buffer));
                HyoutaUtils::IO::WriteFileAtomic(tmp1, d.data(), d.size());
            }
            if (!linestsnames) {
                std::string d =
                    std::format("# get actual m3u file from https://www.twitch.tv/videos/{}",
                                videoInfo->GetVideoId(buffer));
                HyoutaUtils::IO::WriteFileAtomic(tmp2, d.data(), d.size());
            }
            if (!linesbaseurl || !linestsnames) {
                return ResultType::UserInputRequired;
            }

            auto m3u8path = GetM3U8PathFromM3U(*linesbaseurl, videoQuality);
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
                return ResultType::NetworkError;
            }
            if (result->ResponseCode == 404
                && videoInfo->GetVideoRecordingState() != RecordingState::Recorded) {
                // this can happen on streams that have just started, in this just wait a bit
                // and retry
                if (!cancellationToken.DelayFor(std::chrono::seconds(20))) {
                    return ResultType::Cancelled;
                }
                video_json = VodArchiver::TwitchYTDL::GetVideoJson(*videoId);
                if (!video_json) {
                    return ResultType::NetworkError;
                }
                twitchVideoFromJson = VodArchiver::TwitchYTDL::VideoFromJson(*video_json);
                if (!twitchVideoFromJson) {
                    return ResultType::NetworkError;
                }

                {
                    auto newVideoInfo = std::make_unique<TwitchVideoInfo>();
                    newVideoInfo->Service = StreamService::Twitch;
                    newVideoInfo->Video = *twitchVideoFromJson;
                    videoInfo = newVideoInfo->Clone();

                    std::lock_guard lock(*jobConfig.JobsLock);
                    MergeTwitchVideoInfo(newVideoInfo.get(), job.VideoInfo.get());
                    job.VideoInfo = std::move(newVideoInfo);
                }
                continue;
            }
            if (result->ResponseCode != 200) {
                return ResultType::NetworkError;
            }
            std::string m3u8(result->Data.data(), result->Data.size());
            GetFilenamesFromM3U8(downloadInfos, folderpath, m3u8);
        }
        break;
    }

    return ResultType::Success;
}

static ResultType RunTwitchVideoJob(TwitchVideoJob& job,
                                    JobConfig& jobConfig,
                                    TaskCancellation& cancellationToken) {
    std::unique_ptr<IVideoInfo> videoInfo;
    std::string videoQuality;
    {
        std::lock_guard lock(*jobConfig.JobsLock);
        job.JobStatus = VideoJobStatus::Running;
        job.TextStatus = "Retrieving video info...";
        videoInfo = job.VideoInfo->Clone();
        videoQuality = job.VideoQuality;
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

    std::vector<DownloadInfo> downloadInfos;
    ResultType getFileUrlsResult = GetFileUrlsOfVod(
        job, jobConfig, videoInfo, downloadInfos, tempFolderPath, cancellationToken, videoQuality);
    if (getFileUrlsResult == ResultType::UserInputRequired) {
        std::lock_guard lock(*jobConfig.JobsLock);
        job.TextStatus = "Need manual fetch of file URLs.";
        return ResultType::UserInputRequired;
    }
    if (getFileUrlsResult != ResultType::Success) {
        std::lock_guard lock(*jobConfig.JobsLock);
        job.TextStatus = "Failed retrieving file URLs.";
        return getFileUrlsResult;
    }

    std::string finalFilenameWithoutExtension =
        GetFinalFilenameWithoutExtension(*videoInfo, videoQuality);
    std::string tempFilenameWithoutExtension =
        GetTempFilenameWithoutExtension(*videoInfo, videoQuality);

    std::string combinedTempname =
        PathCombine(tempFolderPath, tempFilenameWithoutExtension + "_combined.ts");
    std::string combinedFilename =
        PathCombine(tempFolderPath, tempFilenameWithoutExtension + "_f.ts");
    std::string remuxedTempname =
        PathCombine(tempFolderPath, tempFilenameWithoutExtension + "_combined.mp4");
    std::string remuxedFilename =
        PathCombine(tempFolderPath, tempFilenameWithoutExtension + "_f.mp4");
    std::string targetFilename =
        PathCombine(targetFolderPath, finalFilenameWithoutExtension + ".mp4");
    std::string baseurlfilepath =
        PathCombine(tempFolderPath, tempFilenameWithoutExtension + "_baseurl.txt");
    std::string tsnamesfilepath =
        PathCombine(tempFolderPath, tempFilenameWithoutExtension + "_tsnames.txt");
    std::string tempFolderForParts = PathCombine(tempFolderPath, tempFilenameWithoutExtension);

    if (HyoutaUtils::IO::FileExists(std::string_view(targetFilename))
        != HyoutaUtils::IO::ExistsResult::DoesExist) {
        if (HyoutaUtils::IO::FileExists(std::string_view(combinedFilename))
            != HyoutaUtils::IO::ExistsResult::DoesExist) {
            if (cancellationToken.IsCancellationRequested()) {
                return ResultType::Cancelled;
            }

            {
                std::lock_guard lock(*jobConfig.JobsLock);
                job.TextStatus = "Downloading files...";
            }
            std::vector<std::string> files;
            while (true) {
                if (cancellationToken.IsCancellationRequested()) {
                    return ResultType::Cancelled;
                }


                std::chrono::steady_clock timer;
                auto timerBegin = timer.now();
                ResultType downloadResult = Download(
                    job, jobConfig, files, cancellationToken, tempFolderForParts, downloadInfos, 0);
                if (downloadResult != ResultType::Success) {
                    return downloadResult;
                }
                bool assumeFinished = false;
                {
                    std::lock_guard lock(*jobConfig.JobsLock);
                    assumeFinished = job.AssumeFinished;
                }
                if (assumeFinished
                    || videoInfo->GetVideoRecordingState() == RecordingState::Recorded) {
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
                        {
                            std::lock_guard lock(*jobConfig.JobsLock);
                            job.TextStatus = std::format(
                                "Waiting {} seconds for stream to update...", tsDouble.count());
                            job.UserInputRequest =
                                std::make_unique<UserInputRequestStreamLiveTwitch>(&job);
                        }
                        if (!cancellationToken.DelayFor(ts)) {
                            return ResultType::Cancelled;
                        }
                    }
                    HyoutaUtils::IO::DeleteFile(std::string_view(tsnamesfilepath));
                    getFileUrlsResult = GetFileUrlsOfVod(job,
                                                         jobConfig,
                                                         videoInfo,
                                                         downloadInfos,
                                                         tempFolderPath,
                                                         cancellationToken,
                                                         videoQuality);
                    if (getFileUrlsResult != ResultType::Success) {
                        std::lock_guard lock(*jobConfig.JobsLock);
                        job.TextStatus = "Failed retrieving file URLs.";
                        return getFileUrlsResult;
                    }
                }
            }
            {
                std::lock_guard lock(*jobConfig.JobsLock);
                job.UserInputRequest = nullptr;
                job.TextStatus = "Waiting for free disk IO slot to combine...";
            }
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

                {
                    std::lock_guard lock(*jobConfig.JobsLock);
                    job.TextStatus = "Combining downloaded video parts...";
                }
                HyoutaUtils::IO::DeleteFile(std::string_view(combinedTempname));
                StallWrite(jobConfig,
                           combinedFilename,
                           expectedTargetFilesize,
                           cancellationToken,
                           ShouldStallWriteRegularFile,
                           [&](std::string status) {
                               std::lock_guard lock(*jobConfig.JobsLock);
                               job.TextStatus = std::move(status);
                           });
                if (cancellationToken.IsCancellationRequested()) {
                    return ResultType::Cancelled;
                }
                ResultType combineResult = Combine(cancellationToken, combinedTempname, files);
                if (combineResult != ResultType::Success) {
                    std::lock_guard lock(*jobConfig.JobsLock);
                    job.TextStatus = "Combining failed.";
                    return combineResult;
                }

                // sanity check
                {
                    std::lock_guard lock(*jobConfig.JobsLock);
                    job.TextStatus = "Sanity check on combined video...";
                }
                auto probe = FFMpegProbe(combinedTempname);
                if (!probe) {
                    std::lock_guard lock(*jobConfig.JobsLock);
                    job.TextStatus = "Probing combined video failed.";
                    return ResultType::DubiousCombine;
                }
                TimeSpan actualVideoLength = probe->Duration;
                TimeSpan expectedVideoLength = videoInfo->GetVideoLength();
                {
                    std::lock_guard lock(*jobConfig.JobsLock);
                    if (!job.IgnoreTimeDifferenceCombined
                        && std::abs((actualVideoLength - expectedVideoLength).GetTotalSeconds())
                               > 5.0) {
                        // if difference is bigger than 5 seconds something is off, report
                        job.TextStatus = std::format(
                            "Large time difference between expected ({}s) and combined "
                            "({}s), stopping.",
                            expectedVideoLength.GetTotalSeconds(),
                            actualVideoLength.GetTotalSeconds());
                        job.UserInputRequest =
                            std::make_unique<UserInputRequestTimeMismatchCombined>(&job);
                        // job.WaitingForUserInput = true;
                        return ResultType::DubiousCombine;
                    }
                }

                HyoutaUtils::IO::Move(combinedTempname, combinedFilename, true);
                for (const auto& file : files) {
                    HyoutaUtils::IO::DeleteFile(std::string_view(file));
                }
                HyoutaUtils::IO::DeleteDirectory(std::string_view(tempFolderForParts));
            }
        }

        {
            std::lock_guard lock(*jobConfig.JobsLock);
            job.TextStatus = "Waiting for free disk IO slot to remux...";
        }
        {
            auto diskLock = jobConfig.ExpensiveDiskIO.WaitForFreeSlot(cancellationToken);
            if (cancellationToken.IsCancellationRequested()) {
                return ResultType::Cancelled;
            }
            {
                std::lock_guard lock(*jobConfig.JobsLock);
                job.TextStatus = "Remuxing to MP4...";
            }
            HyoutaUtils::IO::DeleteFile(std::string_view(remuxedTempname));
            StallWrite(jobConfig,
                       remuxedFilename,
                       HyoutaUtils::IO::GetFilesize(std::string_view(combinedFilename)).value_or(0),
                       cancellationToken,
                       ShouldStallWriteRegularFile,
                       [&](std::string status) {
                           std::lock_guard lock(*jobConfig.JobsLock);
                           job.TextStatus = std::move(status);
                       });
            if (cancellationToken.IsCancellationRequested()) {
                return ResultType::Cancelled;
            }
            if (!Remux(remuxedFilename, combinedFilename, remuxedTempname)) {
                std::lock_guard lock(*jobConfig.JobsLock);
                job.TextStatus = "Remuxing failed.";
                return ResultType::Failure;
            }

            // sanity check
            {
                std::lock_guard lock(*jobConfig.JobsLock);
                job.TextStatus = "Sanity check on remuxed video...";
            }
            auto probe = FFMpegProbe(remuxedFilename);
            if (!probe) {
                std::lock_guard lock(*jobConfig.JobsLock);
                job.TextStatus = "Probe on remuxed video failed.";
                return ResultType::DubiousRemux;
            }
            TimeSpan actualVideoLength = probe->Duration;
            TimeSpan expectedVideoLength = videoInfo->GetVideoLength();
            {
                std::lock_guard lock(*jobConfig.JobsLock);
                if (!job.IgnoreTimeDifferenceRemuxed
                    && std::abs((actualVideoLength - expectedVideoLength).GetTotalSeconds())
                           > 5.0) {
                    // if difference is bigger than 5 seconds something is off, report
                    job.TextStatus = std::format(
                        "Large time difference between expected ({}s) and remuxed ({}s), stopping.",
                        expectedVideoLength.GetTotalSeconds(),
                        actualVideoLength.GetTotalSeconds());
                    job.UserInputRequest =
                        std::make_unique<UserInputRequestTimeMismatchRemuxed>(&job);
                    job.WaitingForUserInput = true;
                    return ResultType::DubiousRemux;
                }
            }

            HyoutaUtils::IO::Move(remuxedFilename, targetFilename, true);
            HyoutaUtils::IO::DeleteFile(std::string_view(combinedFilename));
        }
    }

    HyoutaUtils::IO::DeleteFile(std::string_view(tsnamesfilepath));
    HyoutaUtils::IO::DeleteFile(std::string_view(baseurlfilepath));
    {
        std::lock_guard lock(*jobConfig.JobsLock);
        job.TextStatus = "Done!";
        job.JobStatus = VideoJobStatus::Finished;
    }
    return ResultType::Success;
}

ResultType TwitchVideoJob::Run(JobConfig& jobConfig, TaskCancellation& cancellationToken) {
    return RunTwitchVideoJob(*this, jobConfig, cancellationToken);
}

std::string TwitchVideoJob::GenerateOutputFilename() {
    return GetFinalFilenameWithoutExtension(*this->VideoInfo, this->VideoQuality) + ".mp4";
}

std::unique_ptr<IVideoJob> TwitchVideoJob::Clone() const {
    auto clone = std::make_unique<TwitchVideoJob>();
    clone->TextStatus = this->TextStatus;
    clone->JobStatus = this->JobStatus;
    clone->HasBeenValidated = this->HasBeenValidated;
    clone->VideoInfo = this->VideoInfo ? this->VideoInfo->Clone() : nullptr;
    clone->JobStartTimestamp = this->JobStartTimestamp;
    clone->JobFinishTimestamp = this->JobFinishTimestamp;
    clone->Notes = this->Notes;
    clone->AssumeFinished = this->AssumeFinished;
    clone->IgnoreTimeDifferenceCombined = this->IgnoreTimeDifferenceCombined;
    clone->IgnoreTimeDifferenceRemuxed = this->IgnoreTimeDifferenceRemuxed;
    clone->VideoQuality = this->VideoQuality;
    return clone;
}
} // namespace VodArchiver
