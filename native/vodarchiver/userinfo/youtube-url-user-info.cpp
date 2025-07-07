#include "youtube-url-user-info.h"

#include <string>
#include <utility>

namespace VodArchiver {
YoutubeUrlUserInfo::YoutubeUrlUserInfo() = default;

YoutubeUrlUserInfo::YoutubeUrlUserInfo(std::string url) : Url(std::move(url)) {}

YoutubeUrlUserInfo::~YoutubeUrlUserInfo() = default;

ServiceVideoCategoryType YoutubeUrlUserInfo::GetType() {
    return ServiceVideoCategoryType::YoutubeUrl;
}

std::string YoutubeUrlUserInfo::GetUserIdentifier() {
    return Url;
}

FetchReturnValue YoutubeUrlUserInfo::Fetch(size_t offset, bool flat) {
    throw "not implemented";
}
} // namespace VodArchiver
