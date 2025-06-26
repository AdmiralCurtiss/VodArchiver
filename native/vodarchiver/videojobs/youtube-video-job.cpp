#include "youtube-video-job.h"

namespace VodArchiver {
bool YoutubeVideoJob::IsWaitingForUserInput() const {
    throw "not implemented";
}
ResultType YoutubeVideoJob::Run(TaskCancellation& cancellationToken) {
    throw "not implemented";
}
std::string YoutubeVideoJob::GenerateOutputFilename() {
    throw "not implemented";
}
} // namespace VodArchiver
