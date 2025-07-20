#include "generic-user-info.h"

namespace VodArchiver {
GenericUserInfo::~GenericUserInfo() = default;

ServiceVideoCategoryType GenericUserInfo::GetType() {
    return Service;
}

std::string GenericUserInfo::GetUserIdentifier() {
    return Username;
}

FetchReturnValue GenericUserInfo::Fetch(JobConfig& jobConfig, size_t offset, bool flat) {
    return FetchReturnValue{.Success = false, .HasMore = false};
}

std::unique_ptr<IUserInfo> GenericUserInfo::Clone() const {
    auto u = std::make_unique<GenericUserInfo>();
    u->Persistable = this->Persistable;
    u->AutoDownload = this->AutoDownload;
    u->LastRefreshedOn = this->LastRefreshedOn;
    u->Service = this->Service;
    u->UserID = this->UserID;
    u->Username = this->Username;
    u->AdditionalOptions = this->AdditionalOptions;
    return u;
}
} // namespace VodArchiver
