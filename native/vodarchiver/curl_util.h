#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace VodArchiver::curl {
bool InitCurl();
void DeinitCurl();
std::optional<std::string> UrlEscape(std::string_view str);
std::optional<std::vector<char>>
    GetFromUrlToMemory(const std::string& url,
                       const std::vector<std::string>& headers = std::vector<std::string>());
std::optional<std::vector<char>> PostFormFromUrlToMemory(const std::string& url,
                                                         std::string_view data);
} // namespace VodArchiver::curl
