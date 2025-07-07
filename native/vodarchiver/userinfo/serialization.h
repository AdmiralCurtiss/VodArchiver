#pragma once

#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include "i-user-info.h"

namespace VodArchiver {
std::optional<std::vector<std::unique_ptr<IUserInfo>>>
    ParseUserInfosFromFile(std::string_view filename);
bool WriteUserInfosToFile(const std::vector<std::unique_ptr<IUserInfo>>& userInfos,
                          std::string_view filename);
} // namespace VodArchiver
