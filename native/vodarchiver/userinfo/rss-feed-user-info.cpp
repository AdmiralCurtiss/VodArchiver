#include "rss-feed-user-info.h"

#include <string>
#include <utility>

namespace VodArchiver {
RssFeedUserInfo::RssFeedUserInfo() = default;

RssFeedUserInfo::RssFeedUserInfo(std::string url) : Url(std::move(url)) {}

RssFeedUserInfo::~RssFeedUserInfo() = default;

ServiceVideoCategoryType RssFeedUserInfo::GetType() {
    return ServiceVideoCategoryType::RssFeed;
}

std::string RssFeedUserInfo::GetUserIdentifier() {
    return Url;
}

FetchReturnValue RssFeedUserInfo::Fetch(size_t offset, bool flat) {
    throw "not implemented";
}
} // namespace VodArchiver
