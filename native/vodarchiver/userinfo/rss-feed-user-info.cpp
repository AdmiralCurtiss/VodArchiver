#include "rss-feed-user-info.h"

#include <chrono>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "rapidxml/rapidxml.hpp"

#include "util/number.h"

#include "vodarchiver/curl_util.h"
#include "vodarchiver/videoinfo/generic-video-info.h"

namespace VodArchiver {
RssFeedUserInfo::RssFeedUserInfo() = default;

RssFeedUserInfo::RssFeedUserInfo(std::string url) : Url(std::move(url)) {}

RssFeedUserInfo::~RssFeedUserInfo() = default;

ServiceVideoCategoryType RssFeedUserInfo::GetType() {
    return ServiceVideoCategoryType::RssFeed;
}

std::string RssFeedUserInfo::GetUserIdentifier() {
    return Url;
}

static std::optional<std::vector<std::unique_ptr<IVideoInfo>>>
    GetMediaFromFeed(const std::string& url) {
    auto response = VodArchiver::curl::GetFromUrlToMemory(url);
    if (!response || response->ResponseCode >= 400) {
        return std::nullopt;
    }
    response->Data.push_back('\0'); // make sure it's nullterminated

    rapidxml::xml_document<char> xml;
    xml.parse<rapidxml::parse_default>(response->Data.data());
    auto rss = xml.first_node("rss");
    if (!rss) {
        return std::nullopt;
    }
    auto channel = rss->first_node("channel");
    if (!channel) {
        return std::nullopt;
    }

    std::vector<std::unique_ptr<IVideoInfo>> media;
    std::string feedTitle;
    bool feedTitleRead = false;
    for (auto* item = channel->first_node(); item; item = item->next_sibling()) {
        std::string_view name(item->name(), item->name_size());
        if (name == "item") {
            std::string title;
            std::string description;
            std::string fileurl;
            DateTime pubDate;
            TimeSpan duration;
            bool titleRead = false;
            bool enclosureRead = false;
            bool durationRead = false;
            bool descriptionRead = false;
            bool pubDateRead = false;
            for (auto* subitem = item->first_node(); subitem; subitem = subitem->next_sibling()) {
                std::string_view subname(subitem->name(), subitem->name_size());
                if (!titleRead && subname == "title") {
                    title = std::string(subitem->value(), subitem->value_size());
                    titleRead = true;
                } else if (!enclosureRead && subname == "enclosure") {
                    auto* urlitem = subitem->first_attribute("url");
                    if (urlitem) {
                        fileurl = std::string(urlitem->value(), urlitem->value_size());
                        enclosureRead = true;
                    }
                } else if (!durationRead && subname == "itunes:duration") {
                    std::string_view durationstring(subitem->value(), subitem->value_size());
                    auto durationvalue = HyoutaUtils::NumberUtils::ParseDouble(durationstring);
                    if (durationvalue) {
                        duration = TimeSpan::FromSeconds(*durationvalue);
                        durationRead = true;
                    }
                } else if (!descriptionRead && subname == "description") {
                    description = std::string(subitem->value(), subitem->value_size());
                    descriptionRead = true;
                } else if (!pubDateRead && subname == "pubDate") {
                    std::string datestring(subitem->value(), subitem->value_size());
                    std::istringstream stream(datestring);
                    stream.imbue(std::locale::classic());
                    std::chrono::sys_time<std::chrono::seconds> time;
                    stream >> std::chrono::parse("%A, %d %B %Y %H:%M:%S %Z", time);
                    if (!stream.fail()) {
                        pubDate = DateTime::FromUnixTime(time.time_since_epoch().count());
                        pubDateRead = true;
                    }
                }
            }

            if (enclosureRead) {
                auto info = std::make_unique<GenericVideoInfo>();
                info->Service = StreamService::RawUrl;
                info->VideoTitle = std::move(title);
                info->VideoGame = std::move(description);
                info->VideoTimestamp = pubDate;
                info->VideoType = VideoFileType::Unknown;
                info->VideoRecordingState = RecordingState::Recorded;
                info->VideoId = std::move(fileurl);
                info->VideoLength = duration;
                media.push_back(std::move(info));
            }
        } else if (!feedTitleRead && name == "title") {
            feedTitle = std::string(item->value(), item->value_size());
            feedTitleRead = true;
        }
    }

    for (auto& vi : media) {
        vi->SetUsername(feedTitle);
    }

    return media;
}

FetchReturnValue RssFeedUserInfo::Fetch(JobConfig& jobConfig, size_t offset, bool flat) {
    std::vector<std::unique_ptr<IVideoInfo>> videosToAdd;
    bool hasMore = true;
    int64_t maxVideos = -1;
    int64_t currentVideos = -1;

    auto videos = GetMediaFromFeed(Url);
    if (!videos.has_value()) {
        return FetchReturnValue{.Success = false, .HasMore = false};
    }
    hasMore = false;
    currentVideos = videos->size();
    for (auto& m : *videos) {
        videosToAdd.push_back(std::move(m));
    }

    if (videosToAdd.size() <= 0) {
        return FetchReturnValue{.Success = true,
                                .HasMore = false,
                                .TotalVideos = maxVideos,
                                .VideoCountThisFetch = 0,
                                .Videos = std::move(videosToAdd)};
    }

    return FetchReturnValue{.Success = true,
                            .HasMore = hasMore,
                            .TotalVideos = maxVideos,
                            .VideoCountThisFetch = currentVideos,
                            .Videos = std::move(videosToAdd)};
}
} // namespace VodArchiver
