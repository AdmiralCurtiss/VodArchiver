#include "youtube-user-user-info.h"

#include <string>
#include <utility>

namespace VodArchiver {
YoutubeUserUserInfo::YoutubeUserUserInfo() = default;

YoutubeUserUserInfo::YoutubeUserUserInfo(std::string username) : Username(std::move(username)) {}

YoutubeUserUserInfo::~YoutubeUserUserInfo() = default;

ServiceVideoCategoryType YoutubeUserUserInfo::GetType() {
    return ServiceVideoCategoryType::YoutubeUser;
}

std::string YoutubeUserUserInfo::GetUserIdentifier() {
    return Username;
}

FetchReturnValue YoutubeUserUserInfo::Fetch(size_t offset, bool flat) {
    throw "not implemented";
}
} // namespace VodArchiver
