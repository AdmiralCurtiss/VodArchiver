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
} // namespace VodArchiver
