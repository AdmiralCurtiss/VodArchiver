#include "ffmpeg-reencode-job.h"

namespace VodArchiver {
bool FFMpegReencodeJob::IsWaitingForUserInput() const {
    throw "not implemented";
}
ResultType FFMpegReencodeJob::Run(TaskCancellation& cancellationToken) {
    throw "not implemented";
}
std::string FFMpegReencodeJob::GenerateOutputFilename() {
    throw "not implemented";
}
} // namespace VodArchiver
