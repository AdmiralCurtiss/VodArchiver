#pragma once

#include <optional>
#include <string>
#include <vector>

namespace VodArchiver {
bool InitCurl();
void DeinitCurl();
std::optional<std::vector<char>> GetFromUrlToMemory(const std::string& url);
} // namespace VodArchiver
