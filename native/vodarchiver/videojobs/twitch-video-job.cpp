#include "twitch-video-job.h"

namespace VodArchiver {
bool TwitchVideoJob::IsWaitingForUserInput() const {
    throw "not implemented";
}
ResultType TwitchVideoJob::Run(TaskCancellation& cancellationToken) {
    throw "not implemented";
}
std::string TwitchVideoJob::GenerateOutputFilename() {
    throw "not implemented";
}
} // namespace VodArchiver
