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

FetchReturnValue HitboxUserInfo::Fetch(JobConfig& jobConfig, size_t offset, bool flat) {
    return FetchReturnValue{.Success = false, .HasMore = false};
}

std::unique_ptr<IUserInfo> HitboxUserInfo::Clone() const {
    auto u = std::make_unique<HitboxUserInfo>();
    u->Persistable = this->Persistable;
    u->AutoDownload = this->AutoDownload;
    u->LastRefreshedOn = this->LastRefreshedOn;
    u->Username = this->Username;
    return u;
}
} // namespace VodArchiver
