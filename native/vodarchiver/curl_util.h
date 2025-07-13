#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace VodArchiver::curl {
bool InitCurl();
void DeinitCurl();
std::optional<std::string> UrlEscape(std::string_view str);

struct HttpResult {
    long ResponseCode;
    std::vector<char> Data;
};
struct Range {
    size_t Start;
    size_t End;
};
std::optional<HttpResult>
    GetFromUrlToMemory(const std::string& url,
                       const std::vector<std::string>& headers = std::vector<std::string>(),
                       const std::vector<Range>& ranges = std::vector<Range>());
std::optional<HttpResult> PostFormFromUrlToMemory(const std::string& url, std::string_view data);
} // namespace VodArchiver::curl
