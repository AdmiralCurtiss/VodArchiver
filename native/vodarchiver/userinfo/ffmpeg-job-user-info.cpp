#include "ffmpeg-job-user-info.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "util/file.h"

#include "vodarchiver/ffmpeg_util.h"
#include "vodarchiver/videoinfo/ffmpeg-reencode-job-video-info.h"

namespace VodArchiver {
FFMpegJobUserInfo::FFMpegJobUserInfo() = default;

FFMpegJobUserInfo::FFMpegJobUserInfo(std::string path, std::string preset)
  : Path(std::move(path)), Preset(std::move(preset)) {}

FFMpegJobUserInfo::~FFMpegJobUserInfo() = default;

ServiceVideoCategoryType FFMpegJobUserInfo::GetType() {
    return ServiceVideoCategoryType::FFMpegJob;
}

std::string FFMpegJobUserInfo::GetUserIdentifier() {
    return Path;
}

static std::optional<std::vector<std::unique_ptr<IVideoInfo>>>
    FetchReencodeableFiles(const std::string& path, const std::string& additionalOptions) {
    std::string chunked = "_chunked";
    std::string postfix = "_x264crf23";

    std::vector<std::string> reencodeFiles;

    {
        std::filesystem::path rootDir = HyoutaUtils::IO::FilesystemPathFromUtf8(path);
        std::error_code ec;
        std::filesystem::directory_iterator iterator(rootDir, ec);
        if (ec) {
            return std::nullopt;
        }
        while (iterator != std::filesystem::directory_iterator()) {
            if (!iterator->is_directory()) {
                std::string file = HyoutaUtils::IO::FilesystemPathToUtf8(*iterator);
                std::string_view name = HyoutaUtils::IO::GetFileNameWithoutExtension(file);
                std::string_view ext = HyoutaUtils::IO::GetExtension(file);

                if (ext == ".mp4" && name.ends_with(chunked)) {
                    std::string newfile(path);
                    HyoutaUtils::IO::AppendPathElement(newfile, postfix);
                    HyoutaUtils::IO::AppendPathElement(
                        newfile,
                        std::format(
                            "{}{}{}", name.substr(0, name.size() - chunked.size()), postfix, ext));
                    if (!HyoutaUtils::IO::FileExists(std::string_view(newfile))) {
                        reencodeFiles.emplace_back(std::move(file));
                    }
                }
            }
            iterator.increment(ec);
            if (ec) {
                return std::nullopt;
            }
        }
    }

    std::vector<std::unique_ptr<IVideoInfo>> rv;
    for (const std::string& f : reencodeFiles) {
        auto probe = FFMpegProbe(f);
        if (!probe) {
            continue;
        }
        int framerate = 0;
        try {
            for (auto& stream : probe->Streams) {
                if (stream.Framerate > 0.0f) {
                    framerate = std::clamp(std::llround(stream.Framerate),
                                           static_cast<long long>(0),
                                           static_cast<long long>(std::numeric_limits<int>::max()));
                    break;
                }
            }
        } catch (...) {
            framerate = 30;
        }
        if (framerate <= 0) {
            framerate = 30;
        }

        // super hacky... probably should improve this stuff
        std::vector<std::string> ffmpegOptions;
        std::string postfixOld;
        std::string postfixNew;
        std::string outputformat;
        if (additionalOptions == "rescale720_30fps") {
            ffmpegOptions = {{"-c:v",
                              "libx264",
                              "-preset",
                              "slower",
                              "-crf",
                              "23",
                              "-g",
                              "300",
                              "-x264-params",
                              "min-keyint=30:b-adapt=2",
                              "-sws_flags",
                              "lanczos",
                              "-vf",
                              "scale=-2:720",
                              "-c:a",
                              "copy",
                              "-max_muxing_queue_size",
                              "100000",
                              "-r",
                              "30"}};
            postfixOld = "_chunked";
            postfixNew = "_x264crf23scaled720p30";
            outputformat = "mkv";
        } else if (additionalOptions == "rescale480_30fps") {
            ffmpegOptions = {{"-c:v",
                              "libx264",
                              "-preset",
                              "slower",
                              "-crf",
                              "23",
                              "-g",
                              "300",
                              "-x264-params",
                              "min-keyint=30:b-adapt=2",
                              "-sws_flags",
                              "lanczos",
                              "-vf",
                              "scale=-2:480",
                              "-c:a",
                              "copy",
                              "-max_muxing_queue_size",
                              "100000",
                              "-r",
                              "30"}};
            postfixOld = "_chunked";
            postfixNew = "_x264crf23scaled480p30";
            outputformat = "mkv";
        } else if (additionalOptions == "30fps") {
            ffmpegOptions = {{"-c:v",
                              "libx264",
                              "-preset",
                              "slower",
                              "-crf",
                              "23",
                              "-g",
                              "300",
                              "-x264-params",
                              "min-keyint=30:b-adapt=2",
                              "-c:a",
                              "copy",
                              "-max_muxing_queue_size",
                              "100000",
                              "-r",
                              "30"}};
            postfixOld = "_chunked";
            postfixNew = "_x264crf23-30";
            outputformat = "mkv";
        } else if (additionalOptions == "rescale720") {
            ffmpegOptions = {{"-c:v",
                              "libx264",
                              "-preset",
                              "slower",
                              "-crf",
                              "23",
                              "-g",
                              std::format("{}", framerate * 10),
                              "-x264-params",
                              std::format("min-keyint={}:b-adapt=2", framerate),
                              "-sws_flags",
                              "lanczos",
                              "-vf",
                              "scale=-2:720",
                              "-c:a",
                              "copy",
                              "-max_muxing_queue_size",
                              "100000"}};
            postfixOld = "_chunked";
            postfixNew = "_x264crf23scaled720p";
            outputformat = "mp4";
        } else if (additionalOptions == "rescale480") {
            ffmpegOptions = {{"-c:v",
                              "libx264",
                              "-preset",
                              "slower",
                              "-crf",
                              "23",
                              "-g",
                              std::format("{}", framerate * 10),
                              "-x264-params",
                              std::format("min-keyint={}:b-adapt=2", framerate),
                              "-sws_flags",
                              "lanczos",
                              "-vf",
                              "scale=-2:480",
                              "-c:a",
                              "copy",
                              "-max_muxing_queue_size",
                              "100000"}};
            postfixOld = "_chunked";
            postfixNew = "_x264crf23scaled480p";
            outputformat = "mp4";
        } else {
            ffmpegOptions = {{"-c:v",
                              "libx264",
                              "-preset",
                              "slower",
                              "-crf",
                              "23",
                              "-g",
                              std::format("{}", framerate * 10),
                              "-x264-params",
                              std::format("min-keyint={}:b-adapt=2", framerate),
                              "-c:a",
                              "copy",
                              "-max_muxing_queue_size",
                              "100000"}};
            postfixOld = "_chunked";
            postfixNew = "_x264crf23";
            outputformat = "mp4";
        }

        auto retval = std::make_unique<FFMpegReencodeJobVideoInfo>(
            f, *probe, ffmpegOptions, postfixOld, postfixNew, outputformat);

        rv.push_back(std::move(retval));
    }
    return rv;
}


FetchReturnValue FFMpegJobUserInfo::Fetch(JobConfig& jobConfig, size_t offset, bool flat) {
    std::vector<std::unique_ptr<IVideoInfo>> videosToAdd;
    bool hasMore = true;
    int64_t maxVideos = -1;
    int64_t currentVideos = -1;

    auto reencodableFiles = FetchReencodeableFiles(Path, Preset);
    if (!reencodableFiles.has_value()) {
        return FetchReturnValue{.Success = false, .HasMore = false};
    }
    hasMore = false;
    currentVideos = reencodableFiles->size();
    for (auto& m : *reencodableFiles) {
        videosToAdd.push_back(std::move(m));
    }

    if (videosToAdd.size() <= 0) {
        return FetchReturnValue{.Success = true,
                                .HasMore = false,
                                .TotalVideos = maxVideos,
                                .VideoCountThisFetch = 0,
                                .Videos = std::move(videosToAdd)};
    }

    return FetchReturnValue{.Success = true,
                            .HasMore = hasMore,
                            .TotalVideos = maxVideos,
                            .VideoCountThisFetch = currentVideos,
                            .Videos = std::move(videosToAdd)};
}

std::unique_ptr<IUserInfo> FFMpegJobUserInfo::Clone() const {
    auto u = std::make_unique<FFMpegJobUserInfo>();
    u->Persistable = this->Persistable;
    u->AutoDownload = this->AutoDownload;
    u->LastRefreshedOn = this->LastRefreshedOn;
    u->Path = this->Path;
    u->Preset = this->Preset;
    return u;
}
} // namespace VodArchiver
