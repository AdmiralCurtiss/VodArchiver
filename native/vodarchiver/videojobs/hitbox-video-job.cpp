#include "hitbox-video-job.h"

#include <format>

namespace VodArchiver {
bool HitboxVideoJob::IsWaitingForUserInput() const {
    return false;
}

static std::string GetFinalFilenameWithoutExtension(IVideoInfo& vi) {
    return std::format("hitbox_{}_{}", vi.GetUsername(), vi.GetVideoId());
}

ResultType HitboxVideoJob::Run(JobConfig& jobConfig, TaskCancellation& cancellationToken) {
    // hitbox is dead
    return ResultType::Failure;
}

std::string HitboxVideoJob::GenerateOutputFilename() {
    return GetFinalFilenameWithoutExtension(*this->VideoInfo) + ".mp4";
}
} // namespace VodArchiver
