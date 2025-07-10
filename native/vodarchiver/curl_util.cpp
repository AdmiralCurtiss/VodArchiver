#include "curl_util.h"

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

namespace VodArchiver {
bool InitCurl() {
    return curl_global_init(CURL_GLOBAL_ALL) == 0;
}

void DeinitCurl() {
    curl_global_cleanup();
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

std::optional<std::vector<char>> GetFromUrlToMemory(const std::string& url) {
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
    CURLcode ec = curl_easy_perform(handle);
    if (ec != CURLE_OK) {
        return std::nullopt;
    }

    return buffer;
}
} // namespace VodArchiver
