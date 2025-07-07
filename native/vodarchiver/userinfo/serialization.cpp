#include "serialization.h"

#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <format>
#include <iterator>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_print.hpp"

#include "util/file.h"
#include "util/hash/crc32.h"
#include "util/number.h"
#include "util/text.h"

#include "archive-org-user-info.h"
#include "ffmpeg-job-user-info.h"
#include "generic-user-info.h"
#include "hitbox-user-info.h"
#include "i-user-info.h"
#include "rss-feed-user-info.h"
#include "twitch-user-info.h"
#include "youtube-channel-user-info.h"
#include "youtube-playlist-user-info.h"
#include "youtube-url-user-info.h"
#include "youtube-user-user-info.h"

namespace VodArchiver {
static bool
    ParseTwitchUserInfo(TwitchUserInfo& userInfo, rapidxml::xml_node<char>* node, bool highlights) {
    if (!node) {
        return false;
    }

    userInfo.Highlights = highlights;
    userInfo.Persistable = true;

    bool autoDownloadRead = false;
    bool userIDRead = false;
    bool lastRefreshedOnRead = false;
    bool usernameRead = false;
    for (auto* a = node->first_attribute(); a; a = a->next_attribute()) {
        std::string_view name(a->name(), a->name_size());
        std::string_view value(a->value(), a->value_size());
        if (!autoDownloadRead && name == "autoDownload") {
            userInfo.AutoDownload = (value == "true");
            autoDownloadRead = true;
        } else if (!userIDRead && name == "userID") {
            if (value == "?") {
                userInfo.UserID = std::nullopt;
            } else {
                auto p = HyoutaUtils::NumberUtils::ParseInt64(value);
                if (!p) {
                    return false;
                }
                userInfo.UserID = *p;
            }
            userIDRead = true;
        } else if (!lastRefreshedOnRead && name == "lastRefreshedOn") {
            auto p = HyoutaUtils::NumberUtils::ParseInt64(value);
            if (!p) {
                return false;
            }
            userInfo.LastRefreshedOn = DateTime::FromUnixTime(*p);
            lastRefreshedOnRead = true;
        } else if (!usernameRead && name == "username") {
            userInfo.Username = std::string(value);
            usernameRead = true;
        }
    }

    return autoDownloadRead && userIDRead && lastRefreshedOnRead && usernameRead;
}

static bool ParseHitboxUserInfo(HitboxUserInfo& userInfo, rapidxml::xml_node<char>* node) {
    if (!node) {
        return false;
    }

    userInfo.Persistable = true;

    bool autoDownloadRead = false;
    bool lastRefreshedOnRead = false;
    bool usernameRead = false;
    for (auto* a = node->first_attribute(); a; a = a->next_attribute()) {
        std::string_view name(a->name(), a->name_size());
        std::string_view value(a->value(), a->value_size());
        if (!autoDownloadRead && name == "autoDownload") {
            userInfo.AutoDownload = (value == "true");
            autoDownloadRead = true;
        } else if (!lastRefreshedOnRead && name == "lastRefreshedOn") {
            auto p = HyoutaUtils::NumberUtils::ParseInt64(value);
            if (!p) {
                return false;
            }
            userInfo.LastRefreshedOn = DateTime::FromUnixTime(*p);
            lastRefreshedOnRead = true;
        } else if (!usernameRead && name == "username") {
            userInfo.Username = std::string(value);
            usernameRead = true;
        }
    }

    return autoDownloadRead && lastRefreshedOnRead && usernameRead;
}

static bool ParseYoutubeUserUserInfo(YoutubeUserUserInfo& userInfo,
                                     rapidxml::xml_node<char>* node) {
    if (!node) {
        return false;
    }

    userInfo.Persistable = true;

    bool autoDownloadRead = false;
    bool lastRefreshedOnRead = false;
    bool usernameRead = false;
    for (auto* a = node->first_attribute(); a; a = a->next_attribute()) {
        std::string_view name(a->name(), a->name_size());
        std::string_view value(a->value(), a->value_size());
        if (!autoDownloadRead && name == "autoDownload") {
            userInfo.AutoDownload = (value == "true");
            autoDownloadRead = true;
        } else if (!lastRefreshedOnRead && name == "lastRefreshedOn") {
            auto p = HyoutaUtils::NumberUtils::ParseInt64(value);
            if (!p) {
                return false;
            }
            userInfo.LastRefreshedOn = DateTime::FromUnixTime(*p);
            lastRefreshedOnRead = true;
        } else if (!usernameRead && name == "username") {
            userInfo.Username = std::string(value);
            usernameRead = true;
        }
    }

    return autoDownloadRead && lastRefreshedOnRead && usernameRead;
}

static bool ParseYoutubeChannelUserInfo(YoutubeChannelUserInfo& userInfo,
                                        rapidxml::xml_node<char>* node) {
    if (!node) {
        return false;
    }

    userInfo.Persistable = true;

    bool autoDownloadRead = false;
    bool lastRefreshedOnRead = false;
    bool channelRead = false;
    bool commentRead = false;
    for (auto* a = node->first_attribute(); a; a = a->next_attribute()) {
        std::string_view name(a->name(), a->name_size());
        std::string_view value(a->value(), a->value_size());
        if (!autoDownloadRead && name == "autoDownload") {
            userInfo.AutoDownload = (value == "true");
            autoDownloadRead = true;
        } else if (!lastRefreshedOnRead && name == "lastRefreshedOn") {
            auto p = HyoutaUtils::NumberUtils::ParseInt64(value);
            if (!p) {
                return false;
            }
            userInfo.LastRefreshedOn = DateTime::FromUnixTime(*p);
            lastRefreshedOnRead = true;
        } else if (!channelRead && name == "channel") {
            userInfo.Channel = std::string(value);
            channelRead = true;
        } else if (!commentRead && name == "comment") {
            userInfo.Comment = std::string(value);
            commentRead = true;
        }
    }

    // comment is optional
    return autoDownloadRead && lastRefreshedOnRead && channelRead;
}

static bool ParseYoutubePlaylistUserInfo(YoutubePlaylistUserInfo& userInfo,
                                         rapidxml::xml_node<char>* node) {
    if (!node) {
        return false;
    }

    userInfo.Persistable = true;

    bool autoDownloadRead = false;
    bool lastRefreshedOnRead = false;
    bool playlistRead = false;
    bool commentRead = false;
    for (auto* a = node->first_attribute(); a; a = a->next_attribute()) {
        std::string_view name(a->name(), a->name_size());
        std::string_view value(a->value(), a->value_size());
        if (!autoDownloadRead && name == "autoDownload") {
            userInfo.AutoDownload = (value == "true");
            autoDownloadRead = true;
        } else if (!lastRefreshedOnRead && name == "lastRefreshedOn") {
            auto p = HyoutaUtils::NumberUtils::ParseInt64(value);
            if (!p) {
                return false;
            }
            userInfo.LastRefreshedOn = DateTime::FromUnixTime(*p);
            lastRefreshedOnRead = true;
        } else if (!playlistRead && name == "playlist") {
            userInfo.Playlist = std::string(value);
            playlistRead = true;
        } else if (!commentRead && name == "comment") {
            userInfo.Comment = std::string(value);
            commentRead = true;
        }
    }

    // comment is optional
    return autoDownloadRead && lastRefreshedOnRead && playlistRead;
}

static bool ParseRssFeedUserInfo(RssFeedUserInfo& userInfo, rapidxml::xml_node<char>* node) {
    if (!node) {
        return false;
    }

    userInfo.Persistable = true;

    bool autoDownloadRead = false;
    bool lastRefreshedOnRead = false;
    bool urlRead = false;
    for (auto* a = node->first_attribute(); a; a = a->next_attribute()) {
        std::string_view name(a->name(), a->name_size());
        std::string_view value(a->value(), a->value_size());
        if (!autoDownloadRead && name == "autoDownload") {
            userInfo.AutoDownload = (value == "true");
            autoDownloadRead = true;
        } else if (!lastRefreshedOnRead && name == "lastRefreshedOn") {
            auto p = HyoutaUtils::NumberUtils::ParseInt64(value);
            if (!p) {
                return false;
            }
            userInfo.LastRefreshedOn = DateTime::FromUnixTime(*p);
            lastRefreshedOnRead = true;
        } else if (!urlRead && name == "url") {
            userInfo.Url = std::string(value);
            urlRead = true;
        }
    }

    return autoDownloadRead && lastRefreshedOnRead && urlRead;
}

static bool ParseFFMpegJobUserInfo(FFMpegJobUserInfo& userInfo, rapidxml::xml_node<char>* node) {
    if (!node) {
        return false;
    }

    userInfo.Persistable = true;

    bool autoDownloadRead = false;
    bool lastRefreshedOnRead = false;
    bool pathRead = false;
    bool presetRead = false;
    for (auto* a = node->first_attribute(); a; a = a->next_attribute()) {
        std::string_view name(a->name(), a->name_size());
        std::string_view value(a->value(), a->value_size());
        if (!autoDownloadRead && name == "autoDownload") {
            userInfo.AutoDownload = (value == "true");
            autoDownloadRead = true;
        } else if (!lastRefreshedOnRead && name == "lastRefreshedOn") {
            auto p = HyoutaUtils::NumberUtils::ParseInt64(value);
            if (!p) {
                return false;
            }
            userInfo.LastRefreshedOn = DateTime::FromUnixTime(*p);
            lastRefreshedOnRead = true;
        } else if (!pathRead && name == "path") {
            userInfo.Path = std::string(value);
            pathRead = true;
        } else if (!presetRead && name == "preset") {
            userInfo.Preset = std::string(value);
            presetRead = true;
        }
    }

    return autoDownloadRead && lastRefreshedOnRead && pathRead && presetRead;
}

static bool ParseArchiveOrgUserInfo(ArchiveOrgUserInfo& userInfo, rapidxml::xml_node<char>* node) {
    if (!node) {
        return false;
    }

    userInfo.Persistable = true;

    bool autoDownloadRead = false;
    bool lastRefreshedOnRead = false;
    bool identifierRead = false;
    for (auto* a = node->first_attribute(); a; a = a->next_attribute()) {
        std::string_view name(a->name(), a->name_size());
        std::string_view value(a->value(), a->value_size());
        if (!autoDownloadRead && name == "autoDownload") {
            userInfo.AutoDownload = (value == "true");
            autoDownloadRead = true;
        } else if (!lastRefreshedOnRead && name == "lastRefreshedOn") {
            auto p = HyoutaUtils::NumberUtils::ParseInt64(value);
            if (!p) {
                return false;
            }
            userInfo.LastRefreshedOn = DateTime::FromUnixTime(*p);
            lastRefreshedOnRead = true;
        } else if (!identifierRead && name == "identifier") {
            userInfo.Identifier = std::string(value);
            identifierRead = true;
        }
    }

    return autoDownloadRead && lastRefreshedOnRead && identifierRead;
}

static bool ParseYoutubeUrlUserInfo(YoutubeUrlUserInfo& userInfo, rapidxml::xml_node<char>* node) {
    if (!node) {
        return false;
    }

    userInfo.Persistable = true;

    bool autoDownloadRead = false;
    bool lastRefreshedOnRead = false;
    bool urlRead = false;
    for (auto* a = node->first_attribute(); a; a = a->next_attribute()) {
        std::string_view name(a->name(), a->name_size());
        std::string_view value(a->value(), a->value_size());
        if (!autoDownloadRead && name == "autoDownload") {
            userInfo.AutoDownload = (value == "true");
            autoDownloadRead = true;
        } else if (!lastRefreshedOnRead && name == "lastRefreshedOn") {
            auto p = HyoutaUtils::NumberUtils::ParseInt64(value);
            if (!p) {
                return false;
            }
            userInfo.LastRefreshedOn = DateTime::FromUnixTime(*p);
            lastRefreshedOnRead = true;
        } else if (!urlRead && name == "url") {
            userInfo.Url = std::string(value);
            urlRead = true;
        }
    }

    return autoDownloadRead && lastRefreshedOnRead && urlRead;
}

std::optional<std::vector<std::unique_ptr<IUserInfo>>>
    ParseUserInfosFromFile(std::string_view filename) {
    HyoutaUtils::IO::File file(filename, HyoutaUtils::IO::OpenMode::Read);
    if (!file.IsOpen()) {
        return std::nullopt;
    }
    auto length = file.GetLength();
    if (!length) {
        return std::nullopt;
    }

    std::unique_ptr<char[]> xmlBuffer;
    size_t xmlLength = 0;
    {
        if (*length >= std::numeric_limits<size_t>::max()) {
            return std::nullopt;
        }
        if (!file.SetPosition(0)) {
            return std::nullopt;
        }
        xmlLength = static_cast<size_t>(*length);
        xmlBuffer = std::make_unique<char[]>(xmlLength + 1);
        if (file.Read(xmlBuffer.get(), xmlLength) != xmlLength) {
            return std::nullopt;
        }
        xmlBuffer[xmlLength] = '\0'; // make sure string is nullterminated
    }

    std::vector<std::unique_ptr<IUserInfo>> result;
    {
        rapidxml::xml_document<char> xml;
        xml.parse<rapidxml::parse_default>(xmlBuffer.get());
        auto root = xml.first_node("root");
        if (!root) {
            return std::nullopt;
        }
        for (auto* userInfoXml = root->first_node(); userInfoXml;
             userInfoXml = userInfoXml->next_sibling()) {
            std::string_view name(userInfoXml->name(), userInfoXml->name_size());
            if (name == "UserInfo") {
                if (auto* typeXml = userInfoXml->first_attribute("_type")) {
                    std::string_view value(typeXml->value(), typeXml->value_size());
                    if (value == "TwitchRecordings") {
                        auto userInfo = std::make_unique<TwitchUserInfo>();
                        if (ParseTwitchUserInfo(*userInfo, userInfoXml, false)) {
                            result.emplace_back(std::move(userInfo));
                        } else {
                            return std::nullopt;
                        }
                    } else if (value == "TwitchHighlights") {
                        auto userInfo = std::make_unique<TwitchUserInfo>();
                        if (ParseTwitchUserInfo(*userInfo, userInfoXml, true)) {
                            result.emplace_back(std::move(userInfo));
                        } else {
                            return std::nullopt;
                        }
                    } else if (value == "HitboxRecordings") {
                        auto userInfo = std::make_unique<HitboxUserInfo>();
                        if (ParseHitboxUserInfo(*userInfo, userInfoXml)) {
                            result.emplace_back(std::move(userInfo));
                        } else {
                            return std::nullopt;
                        }
                    } else if (value == "YoutubeUser") {
                        auto userInfo = std::make_unique<YoutubeUserUserInfo>();
                        if (ParseYoutubeUserUserInfo(*userInfo, userInfoXml)) {
                            result.emplace_back(std::move(userInfo));
                        } else {
                            return std::nullopt;
                        }
                    } else if (value == "YoutubeChannel") {
                        auto userInfo = std::make_unique<YoutubeChannelUserInfo>();
                        if (ParseYoutubeChannelUserInfo(*userInfo, userInfoXml)) {
                            result.emplace_back(std::move(userInfo));
                        } else {
                            return std::nullopt;
                        }
                    } else if (value == "YoutubePlaylist") {
                        auto userInfo = std::make_unique<YoutubePlaylistUserInfo>();
                        if (ParseYoutubePlaylistUserInfo(*userInfo, userInfoXml)) {
                            result.emplace_back(std::move(userInfo));
                        } else {
                            return std::nullopt;
                        }
                    } else if (value == "RssFeed") {
                        auto userInfo = std::make_unique<RssFeedUserInfo>();
                        if (ParseRssFeedUserInfo(*userInfo, userInfoXml)) {
                            result.emplace_back(std::move(userInfo));
                        } else {
                            return std::nullopt;
                        }
                    } else if (value == "FFMpegJob") {
                        auto userInfo = std::make_unique<FFMpegJobUserInfo>();
                        if (ParseFFMpegJobUserInfo(*userInfo, userInfoXml)) {
                            result.emplace_back(std::move(userInfo));
                        } else {
                            return std::nullopt;
                        }
                    } else if (value == "ArchiveOrg") {
                        auto userInfo = std::make_unique<ArchiveOrgUserInfo>();
                        if (ParseArchiveOrgUserInfo(*userInfo, userInfoXml)) {
                            result.emplace_back(std::move(userInfo));
                        } else {
                            return std::nullopt;
                        }
                    } else if (value == "YoutubeUrl") {
                        auto userInfo = std::make_unique<YoutubeUrlUserInfo>();
                        if (ParseYoutubeUrlUserInfo(*userInfo, userInfoXml)) {
                            result.emplace_back(std::move(userInfo));
                        } else {
                            return std::nullopt;
                        }
                    } else {
                        return std::nullopt;
                    }
                }
            }
        }
    }

    return result;
}

static int64_t DateTimeToUnixTime(const DateTime& dt) {
    return static_cast<int64_t>(dt.Data - 621355968000000000u) / DateTime::TICKS_PER_SECOND;
}

static rapidxml::xml_attribute<char>* AllocateAttribute(rapidxml::xml_document<char>& xml,
                                                        std::string_view name,
                                                        std::string_view value) {
    const char* allocName = xml.allocate_string(name.data(), name.size());
    const char* allocValue = xml.allocate_string(value.data(), value.size());
    return xml.allocate_attribute(allocName, allocValue, name.size(), value.size());
}

static bool SerializeUserInfo(rapidxml::xml_document<char>& xml,
                              rapidxml::xml_node<char>& node,
                              IUserInfo& userInfo) {
    if (auto* c = dynamic_cast<TwitchUserInfo*>(&userInfo)) {
        node.append_attribute(
            AllocateAttribute(xml, "_type", ServiceVideoCategoryTypeToString(c->GetType())));
        node.append_attribute(
            AllocateAttribute(xml, "autoDownload", c->AutoDownload ? "true" : "false"));
        if (c->UserID.has_value()) {
            node.append_attribute(AllocateAttribute(xml, "userID", std::format("{}", *c->UserID)));
        } else {
            node.append_attribute(AllocateAttribute(xml, "userID", "?"));
        }
        node.append_attribute(AllocateAttribute(
            xml, "lastRefreshedOn", std::format("{}", DateTimeToUnixTime(c->LastRefreshedOn))));
        node.append_attribute(AllocateAttribute(xml, "username", c->Username));
        return true;
    } else if (auto* c = dynamic_cast<HitboxUserInfo*>(&userInfo)) {
        node.append_attribute(
            AllocateAttribute(xml, "_type", ServiceVideoCategoryTypeToString(c->GetType())));
        node.append_attribute(
            AllocateAttribute(xml, "autoDownload", c->AutoDownload ? "true" : "false"));
        node.append_attribute(AllocateAttribute(
            xml, "lastRefreshedOn", std::format("{}", DateTimeToUnixTime(c->LastRefreshedOn))));
        node.append_attribute(AllocateAttribute(xml, "username", c->Username));
        return true;
    } else if (auto* c = dynamic_cast<YoutubeUserUserInfo*>(&userInfo)) {
        node.append_attribute(
            AllocateAttribute(xml, "_type", ServiceVideoCategoryTypeToString(c->GetType())));
        node.append_attribute(
            AllocateAttribute(xml, "autoDownload", c->AutoDownload ? "true" : "false"));
        node.append_attribute(AllocateAttribute(
            xml, "lastRefreshedOn", std::format("{}", DateTimeToUnixTime(c->LastRefreshedOn))));
        node.append_attribute(AllocateAttribute(xml, "username", c->Username));
        return true;
    } else if (auto* c = dynamic_cast<YoutubeChannelUserInfo*>(&userInfo)) {
        node.append_attribute(
            AllocateAttribute(xml, "_type", ServiceVideoCategoryTypeToString(c->GetType())));
        node.append_attribute(
            AllocateAttribute(xml, "autoDownload", c->AutoDownload ? "true" : "false"));
        node.append_attribute(AllocateAttribute(
            xml, "lastRefreshedOn", std::format("{}", DateTimeToUnixTime(c->LastRefreshedOn))));
        node.append_attribute(AllocateAttribute(xml, "channel", c->Channel));
        if (!c->Comment.empty()) {
            node.append_attribute(AllocateAttribute(xml, "comment", c->Comment));
        }
        return true;
    } else if (auto* c = dynamic_cast<YoutubePlaylistUserInfo*>(&userInfo)) {
        node.append_attribute(
            AllocateAttribute(xml, "_type", ServiceVideoCategoryTypeToString(c->GetType())));
        node.append_attribute(
            AllocateAttribute(xml, "autoDownload", c->AutoDownload ? "true" : "false"));
        node.append_attribute(AllocateAttribute(
            xml, "lastRefreshedOn", std::format("{}", DateTimeToUnixTime(c->LastRefreshedOn))));
        node.append_attribute(AllocateAttribute(xml, "playlist", c->Playlist));
        if (!c->Comment.empty()) {
            node.append_attribute(AllocateAttribute(xml, "comment", c->Comment));
        }
        return true;
    } else if (auto* c = dynamic_cast<RssFeedUserInfo*>(&userInfo)) {
        node.append_attribute(
            AllocateAttribute(xml, "_type", ServiceVideoCategoryTypeToString(c->GetType())));
        node.append_attribute(
            AllocateAttribute(xml, "autoDownload", c->AutoDownload ? "true" : "false"));
        node.append_attribute(AllocateAttribute(
            xml, "lastRefreshedOn", std::format("{}", DateTimeToUnixTime(c->LastRefreshedOn))));
        node.append_attribute(AllocateAttribute(xml, "url", c->Url));
        return true;
    } else if (auto* c = dynamic_cast<FFMpegJobUserInfo*>(&userInfo)) {
        node.append_attribute(
            AllocateAttribute(xml, "_type", ServiceVideoCategoryTypeToString(c->GetType())));
        node.append_attribute(
            AllocateAttribute(xml, "autoDownload", c->AutoDownload ? "true" : "false"));
        node.append_attribute(AllocateAttribute(
            xml, "lastRefreshedOn", std::format("{}", DateTimeToUnixTime(c->LastRefreshedOn))));
        node.append_attribute(AllocateAttribute(xml, "path", c->Path));
        node.append_attribute(AllocateAttribute(xml, "preset", c->Preset));
        return true;
    } else if (auto* c = dynamic_cast<ArchiveOrgUserInfo*>(&userInfo)) {
        node.append_attribute(
            AllocateAttribute(xml, "_type", ServiceVideoCategoryTypeToString(c->GetType())));
        node.append_attribute(
            AllocateAttribute(xml, "autoDownload", c->AutoDownload ? "true" : "false"));
        node.append_attribute(AllocateAttribute(
            xml, "lastRefreshedOn", std::format("{}", DateTimeToUnixTime(c->LastRefreshedOn))));
        node.append_attribute(AllocateAttribute(xml, "identifier", c->Identifier));
        return true;
    } else if (auto* c = dynamic_cast<YoutubeUrlUserInfo*>(&userInfo)) {
        node.append_attribute(
            AllocateAttribute(xml, "_type", ServiceVideoCategoryTypeToString(c->GetType())));
        node.append_attribute(
            AllocateAttribute(xml, "autoDownload", c->AutoDownload ? "true" : "false"));
        node.append_attribute(AllocateAttribute(
            xml, "lastRefreshedOn", std::format("{}", DateTimeToUnixTime(c->LastRefreshedOn))));
        node.append_attribute(AllocateAttribute(xml, "url", c->Url));
        return true;
    } else if (auto* c = dynamic_cast<GenericUserInfo*>(&userInfo)) {
        // this looks somewhat stupid but is to ensure that a type-specific deserializer can find
        // the attributes at logical names instead of having to check legacy stuff
        node.append_attribute(
            AllocateAttribute(xml, "_type", ServiceVideoCategoryTypeToString(c->GetType())));
        node.append_attribute(
            AllocateAttribute(xml, "autoDownload", c->AutoDownload ? "true" : "false"));
        if (c->UserID.has_value()) {
            node.append_attribute(AllocateAttribute(xml, "userID", std::format("{}", *c->UserID)));
        } else {
            node.append_attribute(AllocateAttribute(xml, "userID", "?"));
        }
        node.append_attribute(AllocateAttribute(
            xml, "lastRefreshedOn", std::format("{}", DateTimeToUnixTime(c->LastRefreshedOn))));
        node.append_attribute(AllocateAttribute(xml, "preset", c->AdditionalOptions));
        node.append_attribute(AllocateAttribute(xml, "username", c->Username));
        node.append_attribute(AllocateAttribute(xml, "channel", c->Username));
        node.append_attribute(AllocateAttribute(xml, "playlist", c->Username));
        node.append_attribute(AllocateAttribute(xml, "url", c->Username));
        node.append_attribute(AllocateAttribute(xml, "path", c->Username));
        return true;
    }
    return false;
}

bool WriteUserInfosToFile(const std::vector<std::unique_ptr<IUserInfo>>& userInfos,
                          std::string_view filename) {
    rapidxml::xml_document<char> xml;
    {
        auto* declaration = xml.allocate_node(rapidxml::node_type::node_declaration);
        declaration->append_attribute(AllocateAttribute(xml, "version", "1.0"));
        declaration->append_attribute(AllocateAttribute(xml, "encoding", "utf-8"));
        xml.append_node(declaration);
    }
    {
        auto* root = xml.allocate_node(rapidxml::node_type::node_element, "root");
        for (auto& userInfo : userInfos) {
            auto* xmlUserInfo = xml.allocate_node(rapidxml::node_type::node_element, "UserInfo");
            if (!SerializeUserInfo(xml, *xmlUserInfo, *userInfo)) {
                return false;
            }
            root->append_node(xmlUserInfo);
        }
        xml.append_node(root);
    }

    std::string str;
    rapidxml::print(std::back_inserter(str), xml, rapidxml::print_no_indenting);

    return HyoutaUtils::IO::WriteFileAtomic(filename, str.data(), str.size());
}
} // namespace VodArchiver
