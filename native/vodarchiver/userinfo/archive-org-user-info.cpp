#include "archive-org-user-info.h"

#include <string>
#include <utility>

namespace VodArchiver {
ArchiveOrgUserInfo::ArchiveOrgUserInfo() = default;

ArchiveOrgUserInfo::ArchiveOrgUserInfo(std::string url) : Identifier(std::move(url)) {}

ArchiveOrgUserInfo::~ArchiveOrgUserInfo() = default;

ServiceVideoCategoryType ArchiveOrgUserInfo::GetType() {
    return ServiceVideoCategoryType::ArchiveOrg;
}

std::string ArchiveOrgUserInfo::GetUserIdentifier() {
    return Identifier;
}

FetchReturnValue ArchiveOrgUserInfo::Fetch(size_t offset, bool flat) {
    throw "not implemented";
}
} // namespace VodArchiver
