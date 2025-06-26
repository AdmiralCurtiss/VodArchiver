#include "ffmpeg-split-job.h"

namespace VodArchiver {
bool FFMpegSplitJob::IsWaitingForUserInput() const {
    throw "not implemented";
}
ResultType FFMpegSplitJob::Run(TaskCancellation& cancellationToken) {
    throw "not implemented";
}
std::string FFMpegSplitJob::GenerateOutputFilename() {
    throw "not implemented";
}
} // namespace VodArchiver
