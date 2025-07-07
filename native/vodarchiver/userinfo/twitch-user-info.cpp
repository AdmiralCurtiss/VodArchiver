#include "twitch-user-info.h"

#include <string>
#include <utility>

namespace VodArchiver {
TwitchUserInfo::TwitchUserInfo() = default;

TwitchUserInfo::TwitchUserInfo(std::string username, bool highlights)
  : Username(std::move(username)), Highlights(highlights) {}

TwitchUserInfo::~TwitchUserInfo() = default;

ServiceVideoCategoryType TwitchUserInfo::GetType() {
    return Highlights ? ServiceVideoCategoryType::TwitchHighlights
                      : ServiceVideoCategoryType::TwitchRecordings;
}

std::string TwitchUserInfo::GetUserIdentifier() {
    return Username;
}

FetchReturnValue TwitchUserInfo::Fetch(size_t offset, bool flat) {
    throw "not implemented";
}
} // namespace VodArchiver
