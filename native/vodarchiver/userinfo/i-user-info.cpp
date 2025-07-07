#include "i-user-info.h"

#include <format>
#include <optional>
#include <string_view>

namespace VodArchiver {
std::string_view ServiceVideoCategoryTypeToString(ServiceVideoCategoryType type) {
    switch (type) {
        case ServiceVideoCategoryType::TwitchRecordings: return "TwitchRecordings";
        case ServiceVideoCategoryType::TwitchHighlights: return "TwitchHighlights";
        case ServiceVideoCategoryType::HitboxRecordings: return "HitboxRecordings";
        case ServiceVideoCategoryType::YoutubeUser: return "YoutubeUser";
        case ServiceVideoCategoryType::YoutubeChannel: return "YoutubeChannel";
        case ServiceVideoCategoryType::YoutubePlaylist: return "YoutubePlaylist";
        case ServiceVideoCategoryType::RssFeed: return "RssFeed";
        case ServiceVideoCategoryType::FFMpegJob: return "FFMpegJob";
        case ServiceVideoCategoryType::ArchiveOrg: return "ArchiveOrg";
        case ServiceVideoCategoryType::YoutubeUrl: return "YoutubeUrl";
        default: return "Unknown";
    }
}

std::optional<ServiceVideoCategoryType> ServiceVideoCategoryTypeFromString(std::string_view sv) {
    if (sv == "TwitchRecordings") {
        return ServiceVideoCategoryType::TwitchRecordings;
    } else if (sv == "TwitchHighlights") {
        return ServiceVideoCategoryType::TwitchHighlights;
    } else if (sv == "HitboxRecordings") {
        return ServiceVideoCategoryType::HitboxRecordings;
    } else if (sv == "YoutubeUser") {
        return ServiceVideoCategoryType::YoutubeUser;
    } else if (sv == "YoutubeChannel") {
        return ServiceVideoCategoryType::YoutubeChannel;
    } else if (sv == "YoutubePlaylist") {
        return ServiceVideoCategoryType::YoutubePlaylist;
    } else if (sv == "RssFeed") {
        return ServiceVideoCategoryType::RssFeed;
    } else if (sv == "FFMpegJob") {
        return ServiceVideoCategoryType::FFMpegJob;
    } else if (sv == "ArchiveOrg") {
        return ServiceVideoCategoryType::ArchiveOrg;
    } else if (sv == "YoutubeUrl") {
        return ServiceVideoCategoryType::YoutubeUrl;
    }
    return std::nullopt;
}

IUserInfo::~IUserInfo() = default;

std::string IUserInfo::ToString() {
    return std::format("{}: {}", ServiceVideoCategoryTypeToString(GetType()), GetUserIdentifier());
}
} // namespace VodArchiver
