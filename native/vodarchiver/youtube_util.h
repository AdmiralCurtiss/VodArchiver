#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "vodarchiver/videoinfo/i-video-info.h"

namespace VodArchiver::Youtube {
std::optional<std::vector<std::unique_ptr<IVideoInfo>>>
    RetrieveVideosFromPlaylist(const std::string& playlist,
                               bool flat,
                               const std::string& usernameIfNotInJson);
std::optional<std::vector<std::unique_ptr<IVideoInfo>>>
    RetrieveVideosFromChannel(const std::string& channel,
                              bool flat,
                              const std::string& usernameIfNotInJson);
std::optional<std::vector<std::unique_ptr<IVideoInfo>>>
    RetrieveVideosFromUser(const std::string& user, bool flat);
std::optional<std::vector<std::unique_ptr<IVideoInfo>>>
    RetrieveVideosFromUrl(const std::string& url, bool flat);

enum class RetrieveVideoResult { Success, FetchFailure, ParseFailure };
struct RetrieveVideoResultStruct {
    RetrieveVideoResult result;
    std::unique_ptr<IVideoInfo> info;
};
RetrieveVideoResultStruct
    RetrieveVideo(const std::string& id, const std::string& usernameIfNotInJson, bool wantCookies);
} // namespace VodArchiver::Youtube
