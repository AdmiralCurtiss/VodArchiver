#include "hitbox-user-info.h"

#include <string>
#include <utility>

namespace VodArchiver {
HitboxUserInfo::HitboxUserInfo() = default;

HitboxUserInfo::HitboxUserInfo(std::string username) : Username(std::move(username)) {}

HitboxUserInfo::~HitboxUserInfo() = default;

ServiceVideoCategoryType HitboxUserInfo::GetType() {
    return ServiceVideoCategoryType::HitboxRecordings;
}

std::string HitboxUserInfo::GetUserIdentifier() {
    return Username;
}

FetchReturnValue HitboxUserInfo::Fetch(size_t offset, bool flat) {
    return FetchReturnValue{.Success = false, .HasMore = false};
}
} // namespace VodArchiver
