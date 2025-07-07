#include "youtube-channel-user-info.h"

#include <format>
#include <string>
#include <utility>

namespace VodArchiver {
YoutubeChannelUserInfo::YoutubeChannelUserInfo() = default;

YoutubeChannelUserInfo::YoutubeChannelUserInfo(std::string channel) : Channel(std::move(channel)) {}

YoutubeChannelUserInfo::~YoutubeChannelUserInfo() = default;

ServiceVideoCategoryType YoutubeChannelUserInfo::GetType() {
    return ServiceVideoCategoryType::YoutubeChannel;
}

std::string YoutubeChannelUserInfo::GetUserIdentifier() {
    return Channel;
}

FetchReturnValue YoutubeChannelUserInfo::Fetch(size_t offset, bool flat) {
    throw "not implemented";
}

std::string YoutubeChannelUserInfo::ToString() {
    if (!Comment.empty()) {
        return std::format(
            "{}: [{}] {}",
            ServiceVideoCategoryTypeToString(ServiceVideoCategoryType::YoutubeChannel),
            Comment,
            Channel);
    }
    return IUserInfo::ToString();
}
} // namespace VodArchiver
