#include "twitch-chat-replay-job.h"

namespace VodArchiver {
bool TwitchChatReplayJob::IsWaitingForUserInput() const {
    throw "not implemented";
}
ResultType TwitchChatReplayJob::Run(const std::string& targetFolderPath,
                                    const std::string& tempFolderPath,
                                    TaskCancellation& cancellationToken) {
    throw "not implemented";
}
std::string TwitchChatReplayJob::GenerateOutputFilename() {
    throw "not implemented";
}
} // namespace VodArchiver
