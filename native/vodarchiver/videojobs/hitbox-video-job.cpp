#include "hitbox-video-job.h"

#include <format>

namespace VodArchiver {
bool HitboxVideoJob::IsWaitingForUserInput() const {
    return false;
}

static std::string GetFinalFilenameWithoutExtension(IVideoInfo& vi) {
    std::array<char, 256> buffer1;
    std::array<char, 256> buffer2;
    return std::format("hitbox_{}_{}", vi.GetUsername(buffer1), vi.GetVideoId(buffer2));
}

ResultType HitboxVideoJob::Run(JobConfig& jobConfig, TaskCancellation& cancellationToken) {
    // hitbox is dead
    return ResultType::Failure;
}

std::string HitboxVideoJob::GenerateOutputFilename() {
    return GetFinalFilenameWithoutExtension(*this->VideoInfo) + ".mp4";
}

std::unique_ptr<IVideoJob> HitboxVideoJob::Clone() const {
    auto clone = std::make_unique<HitboxVideoJob>();
    clone->TextStatus = this->TextStatus;
    clone->JobStatus = this->JobStatus;
    clone->HasBeenValidated = this->HasBeenValidated;
    clone->VideoInfo = this->VideoInfo ? this->VideoInfo->Clone() : nullptr;
    clone->JobStartTimestamp = this->JobStartTimestamp;
    clone->JobFinishTimestamp = this->JobFinishTimestamp;
    clone->Notes = this->Notes;
    return clone;
}
} // namespace VodArchiver
