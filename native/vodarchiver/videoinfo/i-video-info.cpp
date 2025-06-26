#include "i-video-info.h"

namespace VodArchiver {
std::string_view StreamServiceToString(StreamService service) {
    switch (service) {
        case StreamService::Twitch: return "Twitch";
        case StreamService::Hitbox: return "Hitbox";
        case StreamService::TwitchChatReplay: return "TwitchChatReplay";
        case StreamService::Youtube: return "Youtube";
        case StreamService::RawUrl: return "RawUrl";
        case StreamService::FFMpegJob: return "FFMpegJob";
        default: return "Unknown";
    }
}

std::optional<StreamService> StreamServiceFromString(std::string_view sv) {
    if (sv == "Unknown") {
        return StreamService::Unknown;
    } else if (sv == "Twitch") {
        return StreamService::Twitch;
    } else if (sv == "Hitbox") {
        return StreamService::Hitbox;
    } else if (sv == "TwitchChatReplay") {
        return StreamService::TwitchChatReplay;
    } else if (sv == "Youtube") {
        return StreamService::Youtube;
    } else if (sv == "RawUrl") {
        return StreamService::RawUrl;
    } else if (sv == "FFMpegJob") {
        return StreamService::FFMpegJob;
    }
    return std::nullopt;
}

std::string_view RecordingStateToString(RecordingState state) {
    switch (state) {
        case RecordingState::Live: return "Live";
        case RecordingState::Recorded: return "Recorded";
        default: return "Unknown";
    }
}

std::optional<RecordingState> RecordingStateFromString(std::string_view sv) {
    if (sv == "Unknown") {
        return RecordingState::Unknown;
    } else if (sv == "Live") {
        return RecordingState::Live;
    } else if (sv == "Recorded") {
        return RecordingState::Recorded;
    }
    return std::nullopt;
}

std::string_view VideoFileTypeToString(VideoFileType type) {
    switch (type) {
        case VideoFileType::M3U: return "M3U";
        case VideoFileType::FLVs: return "FLVs";
        default: return "Unknown";
    }
}

std::optional<VideoFileType> VideoFileTypeFromString(std::string_view sv) {
    if (sv == "M3U") {
        return VideoFileType::M3U;
    } else if (sv == "FLVs") {
        return VideoFileType::FLVs;
    } else if (sv == "Unknown") {
        return VideoFileType::Unknown;
    }
    return std::nullopt;
}

IVideoInfo::~IVideoInfo() = default;
} // namespace VodArchiver
