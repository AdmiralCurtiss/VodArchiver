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

#include "zlib/zlib.h"

#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_print.hpp"

#include "util/file.h"
#include "util/hash/crc32.h"
#include "util/number.h"
#include "util/text.h"

#include "../videoinfo/ffmpeg-reencode-job-video-info.h"
#include "../videoinfo/generic-video-info.h"
#include "../videoinfo/hitbox-video-info.h"
#include "../videoinfo/i-video-info.h"
#include "../videoinfo/twitch-video-info.h"
#include "../videoinfo/youtube-video-info.h"

#include "ffmpeg-reencode-job.h"
#include "ffmpeg-split-job.h"
#include "generic-file-job.h"
#include "hitbox-video-job.h"
#include "i-video-job.h"
#include "twitch-chat-replay-job.h"
#include "twitch-video-job.h"
#include "youtube-video-job.h"

#ifdef _MSC_VER
#include <intrin.h>
#pragma intrinsic(_mul128)
#endif

namespace VodArchiver {
static std::optional<std::vector<char>> InflateFromFileUnknownSize(HyoutaUtils::IO::File& inputFile,
                                                                   size_t sizehint = 0) {
    // adapted from https://zlib.net/zpipe.c which is public domain
    static constexpr size_t CHUNK = 16384;

    int ret;
    unsigned have;
    z_stream strm;
    char in[CHUNK];
    unsigned char out[CHUNK];

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit2(&strm, -15);
    if (ret != Z_OK) {
        return std::nullopt;
    }

    /* decompress until deflate stream ends or end of file */
    std::vector<char> outputVector;
    if (sizehint > 0) {
        outputVector.reserve(sizehint);
    }
    do {
        strm.avail_in = static_cast<uInt>(inputFile.Read(in, CHUNK));
        if (strm.avail_in == 0)
            break;
        strm.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(in));

        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR); /* state not clobbered */
            switch (ret) {
                case Z_NEED_DICT:
                    ret = Z_DATA_ERROR; /* and fall through */
                    [[fallthrough]];
                case Z_DATA_ERROR:
                case Z_MEM_ERROR: (void)inflateEnd(&strm); return std::nullopt;
            }
            have = CHUNK - strm.avail_out;

            outputVector.insert(outputVector.end(), out, out + have);
        } while (strm.avail_out == 0);

        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    /* clean up and return */
    (void)inflateEnd(&strm);
    if (ret != Z_STREAM_END) {
        return std::nullopt;
    }
    return outputVector;
}

static TimeSpan TimeSpanFromSecondsAndSubsecondTicks(int64_t seconds, int64_t subseconds) {
#ifdef _MSC_VER
    int64_t high;
    int64_t ticks = _mul128(seconds, static_cast<int64_t>(TimeSpan::TICKS_PER_SECOND), &high);
    bool overflow = (high != 0);
#else
    int64_t ticks;
    bool overflow =
        __builtin_mul_overflow(seconds, static_cast<int64_t>(TimeSpan::TICKS_PER_SECOND), &ticks);
#endif
    if (overflow) {
        // overflow in the multiply, return max value
        if (seconds < 0) {
            return TimeSpan{.Ticks = std::numeric_limits<int64_t>::min()};
        } else {
            return TimeSpan{.Ticks = std::numeric_limits<int64_t>::max()};
        }
    }
    if (ticks < 0) {
        if (ticks < std::numeric_limits<int64_t>::min() + subseconds) {
            return TimeSpan{.Ticks = std::numeric_limits<int64_t>::min()};
        }
        return TimeSpan{.Ticks = ticks - subseconds};
    } else {
        if (ticks > std::numeric_limits<int64_t>::max() - subseconds) {
            return TimeSpan{.Ticks = std::numeric_limits<int64_t>::max()};
        }
        return TimeSpan{.Ticks = ticks + subseconds};
    }
}

static std::optional<TimeSpan> ParseTimeSpanFromSeconds(std::string_view value) {
    if (auto dotpos = value.find('.'); dotpos != std::string_view::npos) {
        // try to parse the seconds and subsections separately so we don't lose information from the
        // double conversion
        std::string_view left = value.substr(0, dotpos);
        std::string_view right = value.substr(dotpos + 1);
        if (right.size() == 0) {
            if (auto sec = HyoutaUtils::NumberUtils::ParseInt64(left)) {
                return TimeSpanFromSecondsAndSubsecondTicks(*sec, 0);
            }
        } else if (right.size() > 0 && right.size() <= 7) {
            if (auto sec = HyoutaUtils::NumberUtils::ParseInt64(left)) {
                if (auto subsec = HyoutaUtils::NumberUtils::ParseUInt64(right)) {
                    for (size_t i = 0; i < (7 - right.size()); ++i) {
                        *subsec *= 10;
                    }
                    return TimeSpanFromSecondsAndSubsecondTicks(*sec, *subsec);
                }
            }
        }
    }
    if (auto l = HyoutaUtils::NumberUtils::ParseInt64(value)) {
        return TimeSpanFromSecondsAndSubsecondTicks(*l, 0);
    }
    if (auto l = HyoutaUtils::NumberUtils::ParseDouble(value)) {
        return TimeSpan::FromSeconds(*l);
    }
    return std::nullopt;
}

static std::unique_ptr<IVideoInfo> DeserializeIVideoInfo(rapidxml::xml_node<char>* infoXml) {
    if (auto* typeXml = infoXml->first_attribute("_type")) {
        std::string_view value(typeXml->value(), typeXml->value_size());
        if (value == "GenericVideoInfo") {
            auto info = std::make_unique<GenericVideoInfo>();

            bool serviceRead = false;
            bool usernameRead = false;
            bool videoIdRead = false;
            bool videoTitleRead = false;
            bool videoTagsRead = false;
            bool videoTimestampRead = false;
            bool videoLengthRead = false;
            bool videoRecordingStateRead = false;
            bool videoTypeRead = false;
            for (auto* a = infoXml->first_attribute(); a; a = a->next_attribute()) {
                std::string_view name(a->name(), a->name_size());
                std::string_view value(a->value(), a->value_size());
                if (!serviceRead && name == "service") {
                    if (auto service = StreamServiceFromString(value)) {
                        info->Service = *service;
                        serviceRead = true;
                    }
                } else if (!usernameRead && name == "username") {
                    info->Username = std::string(value);
                    usernameRead = true;
                } else if (!videoIdRead && name == "videoId") {
                    info->VideoId = std::string(value);
                    videoIdRead = true;
                } else if (!videoTitleRead && name == "videoTitle") {
                    info->VideoTitle = std::string(value);
                    videoTitleRead = true;
                } else if (!videoTagsRead && name == "videoTags") {
                    info->VideoGame = std::string(value);
                    videoTagsRead = true;
                } else if (!videoTimestampRead && name == "videoTimestamp") {
                    if (auto ts = HyoutaUtils::NumberUtils::ParseInt64(value)) {
                        info->VideoTimestamp = DateTime::FromBinary(*ts);
                        videoTimestampRead = true;
                    }
                } else if (!videoLengthRead && name == "videoLength") {
                    if (auto l = ParseTimeSpanFromSeconds(value)) {
                        info->VideoLength = *l;
                        videoLengthRead = true;
                    }
                } else if (!videoRecordingStateRead && name == "videoRecordingState") {
                    if (auto r = RecordingStateFromString(value)) {
                        info->VideoRecordingState = *r;
                        videoRecordingStateRead = true;
                    }
                } else if (!videoTypeRead && name == "videoType") {
                    if (auto t = VideoFileTypeFromString(value)) {
                        info->VideoType = *t;
                        videoTypeRead = true;
                    }
                }
            }

            if (serviceRead && usernameRead && videoIdRead && videoTitleRead && videoTagsRead
                && videoTimestampRead && videoLengthRead && videoRecordingStateRead
                && videoTypeRead) {
                return info;
            }
        } else if (value == "HitboxVideoInfo") {
            if (auto* subXml = infoXml->first_node("VideoInfo")) {
                auto info = std::make_unique<HitboxVideoInfo>();

                bool mediaUserNameRead = false;
                bool mediaIdRead = false;
                bool mediaFileRead = false;
                bool mediaUserIdRead = false;
                bool mediaProfilesRead = false;
                bool mediaDateAddedRead = false;
                bool mediaTitleRead = false;
                bool mediaDescriptionRead = false;
                bool mediaGameRead = false;
                bool mediaDurationRead = false;
                bool mediaTypeIdRead = false;
                for (auto* a = subXml->first_attribute(); a; a = a->next_attribute()) {
                    std::string_view name(a->name(), a->name_size());
                    std::string_view value(a->value(), a->value_size());
                    if (!mediaUserNameRead && name == "mediaUserName") {
                        info->VideoInfo.MediaUserName = std::string(value);
                        mediaUserNameRead = true;
                    } else if (!mediaIdRead && name == "mediaId") {
                        if (auto id = HyoutaUtils::NumberUtils::ParseInt32(value)) {
                            info->VideoInfo.MediaId = *id;
                            mediaIdRead = true;
                        }
                    } else if (!mediaFileRead && name == "mediaFile") {
                        info->VideoInfo.MediaFile = std::string(value);
                        mediaFileRead = true;
                    } else if (!mediaUserIdRead && name == "mediaUserId") {
                        if (auto id = HyoutaUtils::NumberUtils::ParseInt32(value)) {
                            info->VideoInfo.MediaUserId = *id;
                            mediaUserIdRead = true;
                        }
                    } else if (!mediaDateAddedRead && name == "mediaDateAdded") {
                        if (auto id = HyoutaUtils::NumberUtils::ParseInt64(value)) {
                            info->VideoInfo.MediaDateAdded = DateTime::FromBinary(*id);
                            mediaDateAddedRead = true;
                        }
                    } else if (!mediaTitleRead && name == "mediaTitle") {
                        info->VideoInfo.MediaTitle = std::string(value);
                        mediaTitleRead = true;
                    } else if (!mediaDescriptionRead && name == "mediaDescription") {
                        info->VideoInfo.MediaDescription = std::string(value);
                        mediaDescriptionRead = true;
                    } else if (!mediaGameRead && name == "mediaGame") {
                        info->VideoInfo.MediaGame = std::string(value);
                        mediaGameRead = true;
                    } else if (!mediaDurationRead && name == "mediaDuration") {
                        if (auto id = HyoutaUtils::NumberUtils::ParseDouble(value)) {
                            info->VideoInfo.MediaDuration = *id;
                            mediaDurationRead = true;
                        }
                    } else if (!mediaTypeIdRead && name == "mediaTypeId") {
                        if (auto id = HyoutaUtils::NumberUtils::ParseInt32(value)) {
                            info->VideoInfo.MediaTypeId = *id;
                            mediaTypeIdRead = true;
                        }
                    }
                }

                for (auto* profileXml = subXml->first_node("MediaProfile"); profileXml;
                     profileXml = profileXml->next_sibling("MediaProfile")) {
                    bool urlRead = false;
                    bool heightRead = false;
                    bool bitrateRead = false;
                    HitboxMediaProfile profile;
                    for (auto* a = profileXml->first_attribute(); a; a = a->next_attribute()) {
                        std::string_view name(a->name(), a->name_size());
                        std::string_view value(a->value(), a->value_size());
                        if (!urlRead && name == "url") {
                            profile.Url = std::string(value);
                            urlRead = true;
                        } else if (!heightRead && name == "height") {
                            if (auto id = HyoutaUtils::NumberUtils::ParseInt32(value)) {
                                profile.Height = *id;
                                heightRead = true;
                            }
                        } else if (!bitrateRead && name == "bitrate") {
                            if (auto id = HyoutaUtils::NumberUtils::ParseInt32(value)) {
                                profile.Bitrate = *id;
                                bitrateRead = true;
                            }
                        }
                    }
                    if (urlRead && heightRead && bitrateRead) {
                        info->VideoInfo.MediaProfiles.emplace_back(std::move(profile));
                        mediaProfilesRead = true;
                    }
                }

                if (mediaUserNameRead && mediaIdRead && mediaFileRead && mediaUserIdRead
                    && mediaProfilesRead && mediaDateAddedRead && mediaTitleRead
                    && mediaDescriptionRead && mediaGameRead && mediaDurationRead
                    && mediaTypeIdRead) {
                    return info;
                }
            }
        } else if (value == "TwitchVideoInfo") {
            auto info = std::make_unique<TwitchVideoInfo>();
            bool serviceRead = false;
            bool idRead = false;
            bool userIdRead = false;
            bool usernameRead = false;
            bool titleRead = false;
            bool gameRead = false;
            bool descriptionRead = false;
            bool createdAtRead = false;
            bool publishedAtRead = false;
            bool durationRead = false;
            bool viewCountRead = false;
            bool typeRead = false;
            bool stateRead = false;
            for (auto* a = infoXml->first_attribute(); a; a = a->next_attribute()) {
                std::string_view name(a->name(), a->name_size());
                std::string_view value(a->value(), a->value_size());
                if (!serviceRead && name == "service") {
                    if (auto service = StreamServiceFromString(value)) {
                        info->_Service = *service;
                        serviceRead = true;
                    }
                }
            }
            if (auto* subXml = infoXml->first_node("VideoInfo")) {
                for (auto* a = subXml->first_attribute(); a; a = a->next_attribute()) {
                    std::string_view name(a->name(), a->name_size());
                    std::string_view value(a->value(), a->value_size());
                    if (!idRead && name == "id") {
                        if (auto id = HyoutaUtils::NumberUtils::ParseInt64(value)) {
                            info->_Video.ID = *id;
                            idRead = true;
                        }
                    } else if (!userIdRead && name == "userid") {
                        if (auto id = HyoutaUtils::NumberUtils::ParseInt64(value)) {
                            info->_Video.UserID = *id;
                            userIdRead = true;
                        }
                    } else if (!usernameRead && name == "username") {
                        info->_Video.Username = std::string(value);
                        usernameRead = true;
                    } else if (!titleRead && name == "title") {
                        info->_Video.Title = std::string(value);
                        titleRead = true;
                    } else if (!gameRead && name == "game") {
                        info->_Video.Game = std::string(value);
                        gameRead = true;
                    } else if (!descriptionRead && name == "description") {
                        info->_Video.Description = std::string(value);
                        descriptionRead = true;
                    } else if (!createdAtRead && name == "created_at") {
                        if (auto v = HyoutaUtils::NumberUtils::ParseInt64(value)) {
                            info->_Video.CreatedAt = DateTime::FromBinary(*v);
                            createdAtRead = true;
                        }
                    } else if (!publishedAtRead && name == "published_at") {
                        if (auto v = HyoutaUtils::NumberUtils::ParseInt64(value)) {
                            info->_Video.PublishedAt = DateTime::FromBinary(*v);
                            publishedAtRead = true;
                        }
                    } else if (!durationRead && name == "duration") {
                        if (auto v = HyoutaUtils::NumberUtils::ParseInt64(value)) {
                            info->_Video.Duration = *v;
                            durationRead = true;
                        }
                    } else if (!viewCountRead && name == "view_count") {
                        if (auto v = HyoutaUtils::NumberUtils::ParseInt64(value)) {
                            info->_Video.ViewCount = *v;
                            viewCountRead = true;
                        }
                    } else if (!typeRead && name == "type") {
                        if (auto v = TwitchVideoTypeFromString(value)) {
                            info->_Video.Type = *v;
                            typeRead = true;
                        }
                    } else if (!stateRead && name == "state") {
                        if (auto v = RecordingStateFromString(value)) {
                            info->_Video.State = *v;
                            stateRead = true;
                        }
                    }
                }
            }

            // C# code accepts this even if a few fields are missing, so let's mirror that...
            if (serviceRead && idRead && userIdRead && durationRead && viewCountRead && typeRead
                && stateRead) {
                return info;
            }
        } else if (value == "YoutubeVideoInfo") {
            auto info = std::make_unique<YoutubeVideoInfo>();
            bool usernameRead = false;
            bool videoIdRead = false;
            bool videoTitleRead = false;
            bool videoTagsRead = false;
            bool videoTimestampRead = false;
            bool videoLengthRead = false;
            bool videoRecordingStateRead = false;
            bool videoTypeRead = false;
            bool userDisplayNameRead = false;
            bool videoDescriptionRead = false;
            for (auto* a = infoXml->first_attribute(); a; a = a->next_attribute()) {
                std::string_view name(a->name(), a->name_size());
                std::string_view value(a->value(), a->value_size());
                if (!usernameRead && name == "username") {
                    info->Username = std::string(value);
                    usernameRead = true;
                } else if (!videoIdRead && name == "videoId") {
                    info->VideoId = std::string(value);
                    videoIdRead = true;
                } else if (!videoTitleRead && name == "videoTitle") {
                    info->VideoTitle = std::string(value);
                    videoTitleRead = true;
                } else if (!videoTagsRead && name == "videoTags") {
                    info->VideoGame = std::string(value);
                    videoTagsRead = true;
                } else if (!videoTimestampRead && name == "videoTimestamp") {
                    if (auto ts = HyoutaUtils::NumberUtils::ParseInt64(value)) {
                        info->VideoTimestamp = DateTime::FromBinary(*ts);
                        videoTimestampRead = true;
                    }
                } else if (!videoLengthRead && name == "videoLength") {
                    if (auto l = ParseTimeSpanFromSeconds(value)) {
                        info->VideoLength = *l;
                        videoLengthRead = true;
                    }
                } else if (!videoRecordingStateRead && name == "videoRecordingState") {
                    if (auto r = RecordingStateFromString(value)) {
                        info->VideoRecordingState = *r;
                        videoRecordingStateRead = true;
                    }
                } else if (!videoTypeRead && name == "videoType") {
                    if (auto t = VideoFileTypeFromString(value)) {
                        info->VideoType = *t;
                        videoTypeRead = true;
                    }
                } else if (!userDisplayNameRead && name == "userDisplayName") {
                    info->UserDisplayName = std::string(value);
                    userDisplayNameRead = true;
                } else if (!videoDescriptionRead && name == "videoDescription") {
                    info->VideoDescription = std::string(value);
                    videoDescriptionRead = true;
                }
            }

            if (usernameRead && videoIdRead && videoTitleRead && videoTagsRead && videoTimestampRead
                && videoLengthRead && videoRecordingStateRead && videoTypeRead
                && userDisplayNameRead && videoDescriptionRead) {
                return info;
            }
        } else if (value == "FFMpegReencodeJobVideoInfo") {
            auto info = std::make_unique<FFMpegReencodeJobVideoInfo>();
            info->OutputFormat = "mp4";
            bool usernameRead = false;
            bool videoIdRead = false;
            bool filesizeRead = false;
            bool bitrateRead = false;
            bool framerateRead = false;
            bool videoTimestampRead = false;
            bool videoLengthRead = false;
            bool postfixOldRead = false;
            bool postfixNewRead = false;
            bool outputFormatRead = false; // optional
            for (auto* a = infoXml->first_attribute(); a; a = a->next_attribute()) {
                std::string_view name(a->name(), a->name_size());
                std::string_view value(a->value(), a->value_size());
                if (!usernameRead && name == "username") {
                    info->VideoTitle = std::string(value);
                    usernameRead = true;
                } else if (!videoIdRead && name == "videoId") {
                    info->VideoId = std::string(value);
                    videoIdRead = true;
                } else if (!filesizeRead && name == "filesize") {
                    if (auto ts = HyoutaUtils::NumberUtils::ParseUInt64(value)) {
                        info->Filesize = *ts;
                        filesizeRead = true;
                    }
                } else if (!bitrateRead && name == "bitrate") {
                    if (auto ts = HyoutaUtils::NumberUtils::ParseUInt64(value)) {
                        info->Bitrate = *ts;
                        bitrateRead = true;
                    }
                } else if (!framerateRead && name == "framerate") {
                    if (auto ts = HyoutaUtils::NumberUtils::ParseFloat(value)) {
                        info->Framerate = *ts;
                        framerateRead = true;
                    }
                } else if (!videoTimestampRead && name == "videoTimestamp") {
                    if (auto ts = HyoutaUtils::NumberUtils::ParseInt64(value)) {
                        info->VideoTimestamp = DateTime::FromBinary(*ts);
                        videoTimestampRead = true;
                    }
                } else if (!videoLengthRead && name == "videoLength") {
                    if (auto l = ParseTimeSpanFromSeconds(value)) {
                        info->VideoLength = *l;
                        videoLengthRead = true;
                    }
                } else if (!postfixOldRead && name == "postfixOld") {
                    info->PostfixOld = std::string(value);
                    postfixOldRead = true;
                } else if (!postfixNewRead && name == "postfixNew") {
                    info->PostfixNew = std::string(value);
                    postfixNewRead = true;
                } else if (!outputFormatRead && name == "outputFormat") {
                    info->OutputFormat = std::string(value);
                    outputFormatRead = true;
                }
            }
            for (auto* optionXml = infoXml->first_node("FFMpegOption"); optionXml;
                 optionXml = optionXml->next_sibling("FFMpegOption")) {
                bool argRead = false;
                std::string arg;
                for (auto* a = optionXml->first_attribute(); a; a = a->next_attribute()) {
                    std::string_view name(a->name(), a->name_size());
                    std::string_view value(a->value(), a->value_size());
                    if (!argRead && name == "arg") {
                        arg = std::string(value);
                        argRead = true;
                    }
                }
                if (argRead) {
                    info->FFMpegOptions.emplace_back(std::move(arg));
                }
            }

            if (usernameRead && videoIdRead && filesizeRead && bitrateRead && framerateRead
                && videoTimestampRead && videoLengthRead && postfixOldRead && postfixNewRead) {
                return info;
            }
        }
    }
    return nullptr;
}

static bool ParseBaseIVideoJob(IVideoJob& job, rapidxml::xml_node<char>* node) {
    if (!node) {
        return false;
    }

    bool statusRead = false;
    bool jobStatusRead = false;
    bool videoInfoRead = false;
    bool hasBeenValidatedRead = false;
    bool notesRead = false;
    bool jobStartTimestampRead = false;
    bool jobFinishTimestampRead = false;
    for (auto* a = node->first_attribute(); a; a = a->next_attribute()) {
        std::string_view name(a->name(), a->name_size());
        std::string_view value(a->value(), a->value_size());
        if (!statusRead && name == "textStatus") {
            job._Status = std::string(value);
            statusRead = true;
        } else if (!jobStatusRead && name == "jobStatus") {
            auto s = VideoJobStatusFromString(value);
            if (!s.has_value()) {
                return false;
            }
            job.JobStatus = *s;
            jobStatusRead = true;
        } else if (!hasBeenValidatedRead && name == "hasBeenValidated") {
            std::string_view trimmed = HyoutaUtils::TextUtils::Trim(value);
            if (HyoutaUtils::TextUtils::CaseInsensitiveEquals(trimmed, "true")) {
                job.HasBeenValidated = true;
            } else if (HyoutaUtils::TextUtils::CaseInsensitiveEquals(trimmed, "false")) {
                job.HasBeenValidated = false;
            } else {
                return false;
            }
            hasBeenValidatedRead = true;
        } else if (!notesRead && name == "notes") {
            job.Notes = std::string(value);
            notesRead = true;
        } else if (!jobStartTimestampRead && name == "jobStartTimestamp") {
            auto v = HyoutaUtils::NumberUtils::ParseInt64(HyoutaUtils::TextUtils::Trim(value));
            if (!v.has_value()) {
                return false;
            }
            job.JobStartTimestamp = DateTime::FromBinary(*v);
            jobStartTimestampRead = true;
        } else if (!jobFinishTimestampRead && name == "jobFinishTimestamp") {
            auto v = HyoutaUtils::NumberUtils::ParseInt64(HyoutaUtils::TextUtils::Trim(value));
            if (!v.has_value()) {
                return false;
            }
            job.JobFinishTimestamp = DateTime::FromBinary(*v);
            jobFinishTimestampRead = true;
        }
    }

    auto* videoInfoXml = node->first_node("VideoInfo");
    if (!videoInfoXml) {
        return false;
    }
    auto vi = DeserializeIVideoInfo(videoInfoXml);
    if (vi) {
        job._VideoInfo = std::move(vi);
        videoInfoRead = true;
    }

    return statusRead && jobStatusRead && videoInfoRead && hasBeenValidatedRead && notesRead
           && jobStartTimestampRead && jobFinishTimestampRead;
}

static bool ParseTwitchVideoJob(TwitchVideoJob& job, rapidxml::xml_node<char>* node) {
    if (!node) {
        return false;
    }

    bool videoQualityRead = false;
    for (auto* a = node->first_attribute(); a; a = a->next_attribute()) {
        std::string_view name(a->name(), a->name_size());
        std::string_view value(a->value(), a->value_size());
        if (!videoQualityRead && name == "videoQuality") {
            job.VideoQuality = std::string(value);
            videoQualityRead = true;
        }
    }

    return videoQualityRead;
}

static bool ParseFFMpegSplitJob(FFMpegSplitJob& job, rapidxml::xml_node<char>* node) {
    if (!node) {
        return false;
    }

    bool splitTimesRead = false;
    for (auto* a = node->first_attribute(); a; a = a->next_attribute()) {
        std::string_view name(a->name(), a->name_size());
        std::string_view value(a->value(), a->value_size());
        if (!splitTimesRead && name == "splitTimes") {
            job.SplitTimes = std::string(value);
            splitTimesRead = true;
        }
    }

    if (splitTimesRead) {
        if (auto* gvi = dynamic_cast<GenericVideoInfo*>(job._VideoInfo.get())) {
            gvi->VideoTitle = job.SplitTimes;
        }
    }

    return splitTimesRead;
}

std::optional<std::vector<std::unique_ptr<IVideoJob>>>
    ParseJobsFromFile(std::string_view filename) {
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
    std::array<char, 10> gzHeader{};
    if (file.Read(gzHeader.data(), gzHeader.size()) == gzHeader.size() && gzHeader[0] == 0x1f
        && static_cast<uint8_t>(gzHeader[1]) == 0x8b && gzHeader[2] == 0x8) {
        // looks compressed
        size_t compressedDataStartPosition = 10;
        if ((gzHeader[3] & 4) != 0) {
            // has extra field, figure out how long that is
            std::array<char, 2> xlen{};
            if (file.Read(xlen.data(), xlen.size()) != xlen.size()) {
                return std::nullopt;
            }
            compressedDataStartPosition += 2;
            compressedDataStartPosition +=
                (static_cast<uint16_t>(static_cast<uint8_t>(xlen[0]))
                 | (static_cast<uint16_t>(static_cast<uint8_t>(xlen[1])) << 8));
        }
        if ((gzHeader[3] & 8) != 0) {
            // skip file name
            if (!file.SetPosition(compressedDataStartPosition)) {
                return std::nullopt;
            }
            while (true) {
                std::array<char, 1> tmp{};
                if (file.Read(tmp.data(), tmp.size()) != tmp.size()) {
                    return std::nullopt;
                }
                ++compressedDataStartPosition;
                if (tmp[0] == 0) {
                    break;
                }
            }
        }
        if ((gzHeader[3] & 16) != 0) {
            // skip file comment
            if (!file.SetPosition(compressedDataStartPosition)) {
                return std::nullopt;
            }
            while (true) {
                std::array<char, 1> tmp{};
                if (file.Read(tmp.data(), tmp.size()) != tmp.size()) {
                    return std::nullopt;
                }
                ++compressedDataStartPosition;
                if (tmp[0] == 0) {
                    break;
                }
            }
        }
        if ((gzHeader[3] & 2) != 0) {
            // skip header checksum
            compressedDataStartPosition += 2;
        }
        // get footer
        std::array<char, 8> gzFooter;
        if (!file.SetPosition(*length - 8)) {
            return std::nullopt;
        }
        if (file.Read(gzFooter.data(), gzFooter.size()) != gzFooter.size()) {
            return std::nullopt;
        }
        const uint32_t checksum =
            (static_cast<uint32_t>(static_cast<uint8_t>(gzFooter[0]))
             | (static_cast<uint32_t>(static_cast<uint8_t>(gzFooter[1])) << 8)
             | (static_cast<uint32_t>(static_cast<uint8_t>(gzFooter[2])) << 16)
             | (static_cast<uint32_t>(static_cast<uint8_t>(gzFooter[3])) << 24));
        const uint32_t modsize =
            (static_cast<uint32_t>(static_cast<uint8_t>(gzFooter[4]))
             | (static_cast<uint32_t>(static_cast<uint8_t>(gzFooter[5])) << 8)
             | (static_cast<uint32_t>(static_cast<uint8_t>(gzFooter[6])) << 16)
             | (static_cast<uint32_t>(static_cast<uint8_t>(gzFooter[7])) << 24));

        // go to start of compressed data
        if (!file.SetPosition(compressedDataStartPosition)) {
            return std::nullopt;
        }

        // unfortunately the .gz format does not store the output size completely, only modulo 2^32,
        // so we have to figure this out as we go...
        auto decompressed = InflateFromFileUnknownSize(file, modsize);
        if (!decompressed) {
            return std::nullopt;
        }
        if ((decompressed->size() & static_cast<uint32_t>(0xffff'ffffu)) != modsize) {
            return std::nullopt;
        }
        auto crc = crc_init();
        crc = crc_update(crc, decompressed->data(), decompressed->size());
        crc = crc_finalize(crc);
        if (crc != checksum) {
            return std::nullopt;
        }
        if (decompressed->size() == std::numeric_limits<size_t>::max()) {
            return std::nullopt;
        }

        xmlLength = decompressed->size();
        xmlBuffer = std::make_unique<char[]>(xmlLength + 1);
        std::memcpy(xmlBuffer.get(), decompressed->data(), xmlLength);
        xmlBuffer[xmlLength] = '\0'; // make sure string is nullterminated
    } else {
        // probably not compressed
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

    std::vector<std::unique_ptr<IVideoJob>> result;
    {
        rapidxml::xml_document<char> xml;
        xml.parse<rapidxml::parse_default>(xmlBuffer.get());
        auto root = xml.first_node("root");
        if (!root) {
            return std::nullopt;
        }
        for (auto* jobXml = root->first_node(); jobXml; jobXml = jobXml->next_sibling()) {
            std::string_view name(jobXml->name(), jobXml->name_size());
            if (name == "Job") {
                if (auto* typeXml = jobXml->first_attribute("_type")) {
                    std::string_view value(typeXml->value(), typeXml->value_size());
                    if (value == "GenericFileJob") {
                        auto job = std::make_unique<GenericFileJob>();
                        if (ParseBaseIVideoJob(*job, jobXml)) {
                            result.emplace_back(std::move(job));
                        } else {
                            return std::nullopt;
                        }
                    } else if (value == "HitboxVideoJob") {
                        auto job = std::make_unique<HitboxVideoJob>();
                        if (ParseBaseIVideoJob(*job, jobXml)) {
                            result.emplace_back(std::move(job));
                        } else {
                            return std::nullopt;
                        }
                    } else if (value == "TwitchChatReplayJob") {
                        auto job = std::make_unique<TwitchChatReplayJob>();
                        if (ParseBaseIVideoJob(*job, jobXml)) {
                            result.emplace_back(std::move(job));
                        } else {
                            return std::nullopt;
                        }
                    } else if (value == "TwitchVideoJob") {
                        auto job = std::make_unique<TwitchVideoJob>();
                        if (ParseBaseIVideoJob(*job, jobXml) && ParseTwitchVideoJob(*job, jobXml)) {
                            result.emplace_back(std::move(job));
                        } else {
                            return std::nullopt;
                        }
                    } else if (value == "YoutubeVideoJob") {
                        auto job = std::make_unique<YoutubeVideoJob>();
                        if (ParseBaseIVideoJob(*job, jobXml)) {
                            result.emplace_back(std::move(job));
                        } else {
                            return std::nullopt;
                        }
                    } else if (value == "FFMpegReencodeJob") {
                        auto job = std::make_unique<FFMpegReencodeJob>();
                        if (ParseBaseIVideoJob(*job, jobXml)) {
                            result.emplace_back(std::move(job));
                        } else {
                            return std::nullopt;
                        }
                    } else if (value == "FFMpegSplitJob") {
                        auto job = std::make_unique<FFMpegSplitJob>();
                        if (ParseBaseIVideoJob(*job, jobXml) && ParseFFMpegSplitJob(*job, jobXml)) {
                            result.emplace_back(std::move(job));
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

static std::string GetTotalSecondsString(const TimeSpan& timeSpan) {
    int64_t ticks = timeSpan.Ticks;
    uint64_t t;
    bool neg;
    if (ticks >= 0) {
        t = static_cast<uint64_t>(ticks);
        neg = false;
    } else if (ticks == std::numeric_limits<int64_t>::min()) {
        t = static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + static_cast<uint64_t>(1);
        neg = true;
    } else {
        t = static_cast<uint64_t>(std::abs(ticks));
        neg = true;
    }
    uint64_t subseconds = t % static_cast<uint64_t>(TimeSpan::TICKS_PER_SECOND);
    uint64_t seconds = t / static_cast<uint64_t>(TimeSpan::TICKS_PER_SECOND);
    std::string r = std::format("{}{}.{:07}", neg ? "-" : "", seconds, subseconds);
    while (r.ends_with('0')) {
        r.pop_back();
    }
    if (r.ends_with('.')) {
        r.pop_back();
    }
    return r;
}

static std::string GetDateTimeBinaryString(const DateTime& dt) {
    return std::format("{}", static_cast<int64_t>(dt.Data));
}

static rapidxml::xml_attribute<char>* AllocateAttribute(rapidxml::xml_document<char>& xml,
                                                        std::string_view name,
                                                        std::string_view value) {
    const char* allocName = xml.allocate_string(name.data(), name.size());
    const char* allocValue = xml.allocate_string(value.data(), value.size());
    return xml.allocate_attribute(allocName, allocValue, name.size(), value.size());
}

static bool SerializeVideoInfo(rapidxml::xml_document<char>& xml,
                               rapidxml::xml_node<char>& node,
                               IVideoInfo& info) {
    if (auto* c = dynamic_cast<GenericVideoInfo*>(&info)) {
        node.append_attribute(AllocateAttribute(xml, "_type", "GenericVideoInfo"));
        node.append_attribute(AllocateAttribute(xml, "service", StreamServiceToString(c->Service)));
        node.append_attribute(AllocateAttribute(xml, "username", c->Username));
        node.append_attribute(AllocateAttribute(xml, "videoId", c->VideoId));
        node.append_attribute(AllocateAttribute(xml, "videoTitle", c->VideoTitle));
        node.append_attribute(AllocateAttribute(xml, "videoTags", c->VideoGame));
        node.append_attribute(
            AllocateAttribute(xml, "videoTimestamp", GetDateTimeBinaryString(c->VideoTimestamp)));
        node.append_attribute(
            AllocateAttribute(xml, "videoLength", GetTotalSecondsString(c->VideoLength)));
        node.append_attribute(AllocateAttribute(
            xml, "videoRecordingState", RecordingStateToString(c->VideoRecordingState)));
        node.append_attribute(
            AllocateAttribute(xml, "videoType", VideoFileTypeToString(c->VideoType)));
        return true;
    } else if (auto* c = dynamic_cast<HitboxVideoInfo*>(&info)) {
        node.append_attribute(AllocateAttribute(xml, "_type", "HitboxVideoInfo"));
        auto* subnode = xml.allocate_node(rapidxml::node_type::node_element, "VideoInfo");
        subnode->append_attribute(AllocateAttribute(xml, "_type", "HitboxVideo"));
        subnode->append_attribute(
            AllocateAttribute(xml, "mediaUserName", c->VideoInfo.MediaUserName));
        subnode->append_attribute(
            AllocateAttribute(xml, "mediaId", std::format("{}", c->VideoInfo.MediaId)));
        subnode->append_attribute(AllocateAttribute(xml, "mediaFile", c->VideoInfo.MediaFile));
        subnode->append_attribute(
            AllocateAttribute(xml, "mediaUserId", std::format("{}", c->VideoInfo.MediaUserId)));
        for (auto& profile : c->VideoInfo.MediaProfiles) {
            auto* xmlProfile = xml.allocate_node(rapidxml::node_type::node_element, "MediaProfile");
            xmlProfile->append_attribute(AllocateAttribute(xml, "url", profile.Url));
            xmlProfile->append_attribute(
                AllocateAttribute(xml, "height", std::format("{}", profile.Height)));
            xmlProfile->append_attribute(
                AllocateAttribute(xml, "bitrate", std::format("{}", profile.Bitrate)));
            subnode->append_node(xmlProfile);
        }
        subnode->append_attribute(AllocateAttribute(
            xml, "mediaDateAdded", std::format("{}", c->VideoInfo.MediaDateAdded.Data)));
        subnode->append_attribute(AllocateAttribute(xml, "mediaTitle", c->VideoInfo.MediaTitle));
        subnode->append_attribute(
            AllocateAttribute(xml, "mediaDescription", c->VideoInfo.MediaDescription));
        subnode->append_attribute(AllocateAttribute(xml, "mediaGame", c->VideoInfo.MediaGame));
        subnode->append_attribute(
            AllocateAttribute(xml, "mediaDuration", std::format("{}", c->VideoInfo.MediaDuration)));
        subnode->append_attribute(
            AllocateAttribute(xml, "mediaTypeId", std::format("{}", c->VideoInfo.MediaTypeId)));
        node.append_node(subnode);
        return true;
    } else if (auto* c = dynamic_cast<TwitchVideoInfo*>(&info)) {
        node.append_attribute(AllocateAttribute(xml, "_type", "TwitchVideoInfo"));
        auto* subnode = xml.allocate_node(rapidxml::node_type::node_element, "VideoInfo");
        subnode->append_attribute(AllocateAttribute(xml, "_type", "TwitchVideo"));
        subnode->append_attribute(AllocateAttribute(xml, "id", std::format("{}", c->_Video.ID)));
        subnode->append_attribute(
            AllocateAttribute(xml, "userid", std::format("{}", c->_Video.UserID)));
        if (c->_Video.Username.has_value()) {
            subnode->append_attribute(AllocateAttribute(xml, "username", *c->_Video.Username));
        }
        if (c->_Video.Title.has_value()) {
            subnode->append_attribute(AllocateAttribute(xml, "title", *c->_Video.Title));
        }
        if (c->_Video.Game.has_value()) {
            subnode->append_attribute(AllocateAttribute(xml, "game", *c->_Video.Game));
        }
        if (c->_Video.Description.has_value()) {
            subnode->append_attribute(
                AllocateAttribute(xml, "description", *c->_Video.Description));
        }
        if (c->_Video.CreatedAt.has_value()) {
            subnode->append_attribute(AllocateAttribute(
                xml, "created_at", GetDateTimeBinaryString(*c->_Video.CreatedAt)));
        }
        if (c->_Video.PublishedAt.has_value()) {
            subnode->append_attribute(AllocateAttribute(
                xml, "published_at", GetDateTimeBinaryString(*c->_Video.PublishedAt)));
        }
        subnode->append_attribute(
            AllocateAttribute(xml, "duration", std::format("{}", c->_Video.Duration)));
        subnode->append_attribute(
            AllocateAttribute(xml, "view_count", std::format("{}", c->_Video.ViewCount)));
        subnode->append_attribute(
            AllocateAttribute(xml, "type", TwitchVideoTypeToString(c->_Video.Type)));
        subnode->append_attribute(
            AllocateAttribute(xml, "state", RecordingStateToString(c->_Video.State)));
        node.append_node(subnode);
        node.append_attribute(
            AllocateAttribute(xml, "service", StreamServiceToString(c->_Service)));
        return true;
    } else if (auto* c = dynamic_cast<YoutubeVideoInfo*>(&info)) {
        node.append_attribute(AllocateAttribute(xml, "_type", "YoutubeVideoInfo"));
        node.append_attribute(AllocateAttribute(xml, "username", c->Username));
        node.append_attribute(AllocateAttribute(xml, "videoId", c->VideoId));
        node.append_attribute(AllocateAttribute(xml, "videoTitle", c->VideoTitle));
        node.append_attribute(AllocateAttribute(xml, "videoTags", c->VideoGame));
        node.append_attribute(
            AllocateAttribute(xml, "videoTimestamp", GetDateTimeBinaryString(c->VideoTimestamp)));
        node.append_attribute(
            AllocateAttribute(xml, "videoLength", GetTotalSecondsString(c->VideoLength)));
        node.append_attribute(AllocateAttribute(
            xml, "videoRecordingState", RecordingStateToString(c->VideoRecordingState)));
        node.append_attribute(
            AllocateAttribute(xml, "videoType", VideoFileTypeToString(c->VideoType)));
        node.append_attribute(AllocateAttribute(xml, "userDisplayName", c->UserDisplayName));
        node.append_attribute(AllocateAttribute(xml, "videoDescription", c->VideoDescription));
        return true;
    } else if (auto* c = dynamic_cast<FFMpegReencodeJobVideoInfo*>(&info)) {
        node.append_attribute(AllocateAttribute(xml, "_type", "FFMpegReencodeJobVideoInfo"));
        node.append_attribute(AllocateAttribute(xml, "username", c->VideoTitle));
        node.append_attribute(AllocateAttribute(xml, "videoId", c->VideoId));
        node.append_attribute(AllocateAttribute(xml, "filesize", std::format("{}", c->Filesize)));
        node.append_attribute(AllocateAttribute(xml, "bitrate", std::format("{}", c->Bitrate)));
        node.append_attribute(AllocateAttribute(xml, "framerate", std::format("{}", c->Framerate)));
        node.append_attribute(
            AllocateAttribute(xml, "videoTimestamp", GetDateTimeBinaryString(c->VideoTimestamp)));
        node.append_attribute(
            AllocateAttribute(xml, "videoLength", GetTotalSecondsString(c->VideoLength)));
        node.append_attribute(AllocateAttribute(xml, "postfixOld", c->PostfixOld));
        node.append_attribute(AllocateAttribute(xml, "postfixNew", c->PostfixNew));
        node.append_attribute(AllocateAttribute(xml, "outputFormat", c->OutputFormat));
        for (auto& option : c->FFMpegOptions) {
            auto* ffmpegOptionsNode =
                xml.allocate_node(rapidxml::node_type::node_element, "FFMpegOption");
            ffmpegOptionsNode->append_attribute(AllocateAttribute(xml, "arg", option));
            node.append_node(ffmpegOptionsNode);
        }
        return true;
    }
    return false;
}

static bool SerializeBaseIVideoJob(rapidxml::xml_document<char>& xml,
                                   rapidxml::xml_node<char>& node,
                                   IVideoJob& job) {
    node.append_attribute(AllocateAttribute(xml, "textStatus", job._Status));
    node.append_attribute(
        AllocateAttribute(xml, "jobStatus", VideoJobStatusToString(job.JobStatus)));
    {
        auto* vi = xml.allocate_node(rapidxml::node_type::node_element, "VideoInfo");
        if (!SerializeVideoInfo(xml, *vi, *job._VideoInfo)) {
            return false;
        }
        node.append_node(vi);
    }
    node.append_attribute(
        AllocateAttribute(xml, "hasBeenValidated", job.HasBeenValidated ? "True" : "False"));
    node.append_attribute(AllocateAttribute(xml, "notes", job.Notes));
    node.append_attribute(AllocateAttribute(
        xml, "jobStartTimestamp", GetDateTimeBinaryString(job.JobStartTimestamp)));
    node.append_attribute(AllocateAttribute(
        xml, "jobFinishTimestamp", GetDateTimeBinaryString(job.JobFinishTimestamp)));
    return true;
}

static bool SerializeJob(rapidxml::xml_document<char>& xml,
                         rapidxml::xml_node<char>& node,
                         IVideoJob& job) {
    if (auto* c = dynamic_cast<GenericFileJob*>(&job)) {
        node.append_attribute(AllocateAttribute(xml, "_type", "GenericFileJob"));
        return SerializeBaseIVideoJob(xml, node, *c);
    } else if (auto* c = dynamic_cast<HitboxVideoJob*>(&job)) {
        node.append_attribute(AllocateAttribute(xml, "_type", "HitboxVideoJob"));
        return SerializeBaseIVideoJob(xml, node, *c);
    } else if (auto* c = dynamic_cast<TwitchChatReplayJob*>(&job)) {
        node.append_attribute(AllocateAttribute(xml, "_type", "TwitchChatReplayJob"));
        return SerializeBaseIVideoJob(xml, node, *c);
    } else if (auto* c = dynamic_cast<TwitchVideoJob*>(&job)) {
        node.append_attribute(AllocateAttribute(xml, "_type", "TwitchVideoJob"));
        if (!SerializeBaseIVideoJob(xml, node, *c)) {
            return false;
        }
        node.append_attribute(AllocateAttribute(xml, "videoQuality", c->VideoQuality));
        return true;
    } else if (auto* c = dynamic_cast<YoutubeVideoJob*>(&job)) {
        node.append_attribute(AllocateAttribute(xml, "_type", "YoutubeVideoJob"));
        return SerializeBaseIVideoJob(xml, node, *c);
    } else if (auto* c = dynamic_cast<FFMpegReencodeJob*>(&job)) {
        node.append_attribute(AllocateAttribute(xml, "_type", "FFMpegReencodeJob"));
        return SerializeBaseIVideoJob(xml, node, *c);
    } else if (auto* c = dynamic_cast<FFMpegSplitJob*>(&job)) {
        node.append_attribute(AllocateAttribute(xml, "_type", "FFMpegSplitJob"));
        if (!SerializeBaseIVideoJob(xml, node, *c)) {
            return false;
        }
        node.append_attribute(AllocateAttribute(xml, "splitTimes", c->SplitTimes));
        return true;
    }
    return false;
}

bool WriteJobsToFile(const std::vector<std::unique_ptr<IVideoJob>>& jobs,
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
        for (auto& job : jobs) {
            auto* xmlJob = xml.allocate_node(rapidxml::node_type::node_element, "Job");
            if (!SerializeJob(xml, *xmlJob, *job)) {
                return false;
            }
            root->append_node(xmlJob);
        }
        xml.append_node(root);
    }

    std::string str;
    rapidxml::print(std::back_inserter(str), xml, rapidxml::print_no_indenting);

    return HyoutaUtils::IO::WriteFileAtomic(filename, str.data(), str.size());
}
} // namespace VodArchiver
