#include "generic-user-info.h"

namespace VodArchiver {
GenericUserInfo::~GenericUserInfo() = default;

ServiceVideoCategoryType GenericUserInfo::GetType() {
    return Service;
}

std::string GenericUserInfo::GetUserIdentifier() {
    return Username;
}

FetchReturnValue GenericUserInfo::Fetch(size_t offset, bool flat) {
    throw "not implemented";
}
} // namespace VodArchiver
