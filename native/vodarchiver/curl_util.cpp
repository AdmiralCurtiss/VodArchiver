#include "curl_util.h"

#include <cstddef>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "util/scope.h"

// this includes windows.h on windows for some stupid reason...
#define WIN32_LEAN_AND_MEAN
#include "curl/curl.h"
#ifdef CreateDirectory
#undef CreateDirectory
#endif
#ifdef CopyFile
#undef CopyFile
#endif
#ifdef MoveFile
#undef MoveFile
#endif
#ifdef DeleteFile
#undef DeleteFile
#endif
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

namespace VodArchiver::curl {
bool InitCurl() {
    return curl_global_init(CURL_GLOBAL_ALL) == 0;
}

void DeinitCurl() {
    curl_global_cleanup();
}

std::optional<std::string> UrlEscape(std::string_view str) {
    if (str.size() > static_cast<size_t>(std::numeric_limits<int>::max())) {
        return std::nullopt;
    }

    CURL* handle = curl_easy_init();
    if (handle == nullptr) {
        return std::nullopt;
    }
    auto handleScope = HyoutaUtils::MakeScopeGuard([&]() { curl_easy_cleanup(handle); });

    char* escaped = curl_easy_escape(handle, str.data(), str.size());
    if (escaped == nullptr) {
        return std::nullopt;
    }

    std::string result(escaped);
    curl_free(escaped);
    return result;
}

static size_t WriteToVectorCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    if (userdata == nullptr) {
        return CURL_WRITEFUNC_ERROR;
    }

    try {
        std::vector<char>* buffer = static_cast<std::vector<char>*>(userdata);
        buffer->insert(buffer->end(), ptr, ptr + (size * nmemb));
    } catch (...) {
        return CURL_WRITEFUNC_ERROR;
    }

    return size * nmemb;
}

std::optional<std::vector<char>> GetFromUrlToMemory(const std::string& url,
                                                    const std::vector<std::string>& headers) {
    CURL* handle = curl_easy_init();
    if (handle == nullptr) {
        return std::nullopt;
    }
    auto handleScope = HyoutaUtils::MakeScopeGuard([&]() { curl_easy_cleanup(handle); });

    std::vector<char> buffer;
    curl_easy_setopt(handle, CURLOPT_FAILONERROR, 1);
    curl_easy_setopt(handle, CURLOPT_HTTPGET, 1);
    curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteToVectorCallback);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, CURLFOLLOW_ALL);

    struct curl_slist* headerList = nullptr;
    auto headerListScope = HyoutaUtils::MakeScopeGuard([&]() {
        if (headerList != nullptr) {
            curl_slist_free_all(headerList);
            headerList = nullptr;
        }
    });
    if (!headers.empty()) {
        for (const std::string& h : headers) {
            headerList = curl_slist_append(headerList, h.c_str());
        }

        curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headerList);
        curl_easy_setopt(handle, CURLOPT_HEADEROPT, (long)CURLHEADER_SEPARATE);
    }

    CURLcode ec = curl_easy_perform(handle);
    if (ec != CURLE_OK) {
        return std::nullopt;
    }

    return buffer;
}

std::optional<std::vector<char>> PostFormFromUrlToMemory(const std::string& url,
                                                         std::string_view data) {
    CURL* handle = curl_easy_init();
    if (handle == nullptr) {
        return std::nullopt;
    }
    auto handleScope = HyoutaUtils::MakeScopeGuard([&]() { curl_easy_cleanup(handle); });

    std::vector<char> buffer;
    curl_easy_setopt(handle, CURLOPT_FAILONERROR, 1);
    curl_easy_setopt(handle, CURLOPT_HTTPPOST, 1);
    curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, data.size());
    curl_easy_setopt(handle, CURLOPT_POSTFIELDS, data.data());
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteToVectorCallback);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, CURLFOLLOW_ALL);
    CURLcode ec = curl_easy_perform(handle);
    if (ec != CURLE_OK) {
        return std::nullopt;
    }

    return buffer;
}
} // namespace VodArchiver::curl
