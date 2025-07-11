#include "hitbox-video-job.h"

#include <format>

namespace VodArchiver {
bool HitboxVideoJob::IsWaitingForUserInput() const {
    return false;
}

ResultType HitboxVideoJob::Run(const std::string& targetFolderPath,
                               const std::string& tempFolderPath,
                               TaskCancellation& cancellationToken) {
    // hitbox is dead
    return ResultType::Failure;
}

std::string HitboxVideoJob::GetTempFilenameWithoutExtension() const {
    auto vi = GetVideoInfo();
    return std::format("hitbox_{}_{}", vi->GetUsername(), vi->GetVideoId());
}

std::string HitboxVideoJob::GetFinalFilenameWithoutExtension() const {
    return GetTempFilenameWithoutExtension();
}

std::string HitboxVideoJob::GenerateOutputFilename() {
    return GetFinalFilenameWithoutExtension() + ".mp4";
}
} // namespace VodArchiver
