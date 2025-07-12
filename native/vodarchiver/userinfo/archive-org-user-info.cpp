#include "archive-org-user-info.h"

#include <format>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "rapidxml/rapidxml.hpp"

#include "util/number.h"

#include "vodarchiver/curl_util.h"
#include "vodarchiver/time_types.h"
#include "vodarchiver/videoinfo/generic-video-info.h"
#include "vodarchiver/videoinfo/i-video-info.h"

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

static std::optional<std::vector<std::unique_ptr<IVideoInfo>>>
    ArchiveOrgGetFilesFromUrl(const std::string& identifier) {
    std::string url = std::format("https://archive.org/download/{0}/{0}_files.xml", identifier);

    auto xmlBuffer = VodArchiver::curl::GetFromUrlToMemory(url);
    if (!xmlBuffer) {
        return std::nullopt;
    }
    xmlBuffer->push_back('\0'); // make sure it's nullterminated

    std::vector<std::unique_ptr<IVideoInfo>> vi;
    rapidxml::xml_document<char> xml;
    xml.parse<rapidxml::parse_default>(xmlBuffer->data());
    auto files = xml.first_node("files");
    if (!files) {
        return std::nullopt;
    }
    for (auto* fileXml = files->first_node(); fileXml; fileXml = fileXml->next_sibling()) {
        std::string_view name(fileXml->name(), fileXml->name_size());
        if (name == "file") {
            std::string filename;
            std::string source;
            DateTime mtime = DateTime::FromUnixTime(0);
            bool filenameRead = false;
            bool sourceRead = false;
            bool mtimeRead = false;
            for (auto* a = fileXml->first_attribute(); a; a = a->next_attribute()) {
                std::string_view name(a->name(), a->name_size());
                std::string_view value(a->value(), a->value_size());
                if (!filenameRead && name == "name") {
                    filename = std::string(value);
                    filenameRead = true;
                } else if (!sourceRead && name == "source") {
                    source = std::string(value);
                    sourceRead = true;
                }
            }
            for (auto* n = fileXml->first_node(); n; n = n->next_sibling()) {
                std::string_view name(n->name(), n->name_size());
                std::string_view value(n->value(), n->value_size());
                if (!mtimeRead && name == "mtime") {
                    auto p = HyoutaUtils::NumberUtils::ParseInt64(value);
                    if (p) {
                        mtime = DateTime::FromUnixTime(*p);
                    }
                    mtimeRead = true;
                }
            }

            if (filenameRead && sourceRead) {
                auto v = std::make_unique<GenericVideoInfo>();
                v->Service = StreamService::RawUrl;
                v->Username = identifier;
                v->VideoId =
                    std::format("https://archive.org/download/{}/{}", identifier, filename);
                v->VideoTitle = std::move(filename);
                v->VideoGame = std::move(source);
                v->VideoTimestamp = mtime;
                v->VideoLength = TimeSpan{.Ticks = 0};
                v->VideoRecordingState = RecordingState::Unknown;
                v->VideoType = VideoFileType::Unknown;
                vi.push_back(std::move(v));
            }
        }
    }

    return vi;
}

FetchReturnValue ArchiveOrgUserInfo::Fetch(size_t offset, bool flat) {
    std::vector<std::unique_ptr<IVideoInfo>> videosToAdd;
    bool hasMore = true;
    long maxVideos = -1;
    int currentVideos = -1;

    auto rssFeedMedia = ArchiveOrgGetFilesFromUrl(Identifier);
    if (!rssFeedMedia) {
        return FetchReturnValue{.Success = false};
    }
    hasMore = false;
    currentVideos = rssFeedMedia->size();
    for (auto& m : *rssFeedMedia) {
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
