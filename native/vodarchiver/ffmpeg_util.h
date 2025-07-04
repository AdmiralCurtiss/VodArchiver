#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "time_types.h"

namespace VodArchiver {
struct FFProbeStream {
    uint32_t Index;
    uint32_t CodecTag;
    std::string CodecType;
    std::optional<TimeSpan> Duration;
    float Framerate;
};

struct FFProbeResult {
    std::string Path;
    DateTime Timestamp;
    uint64_t Filesize;
    uint64_t Bitrate;
    TimeSpan Duration;
    std::vector<FFProbeStream> Streams;
};

std::optional<FFProbeResult> FFMpegProbe(const std::string& filename);
} // namespace VodArchiver
