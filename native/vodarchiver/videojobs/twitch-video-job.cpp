#include "twitch-video-job.h"

namespace VodArchiver {
bool TwitchVideoJob::IsWaitingForUserInput() const {
    throw "not implemented";
}
ResultType TwitchVideoJob::Run(const std::string& targetFolderPath,
                               const std::string& tempFolderPath,
                               TaskCancellation& cancellationToken) {
    throw "not implemented";
}
std::string TwitchVideoJob::GenerateOutputFilename() {
    throw "not implemented";
}
} // namespace VodArchiver
