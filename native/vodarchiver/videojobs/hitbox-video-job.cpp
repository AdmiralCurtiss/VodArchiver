#include "hitbox-video-job.h"

namespace VodArchiver {
bool HitboxVideoJob::IsWaitingForUserInput() const {
    throw "not implemented";
}
ResultType HitboxVideoJob::Run(const std::string& targetFolderPath,
                               const std::string& tempFolderPath,
                               TaskCancellation& cancellationToken) {
    throw "not implemented";
}
std::string HitboxVideoJob::GenerateOutputFilename() {
    throw "not implemented";
}
} // namespace VodArchiver
