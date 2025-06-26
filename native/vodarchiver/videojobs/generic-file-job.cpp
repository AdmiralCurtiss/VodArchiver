#include "generic-file-job.h"

namespace VodArchiver {
bool GenericFileJob::IsWaitingForUserInput() const {
    throw "not implemented";
}
ResultType GenericFileJob::Run(TaskCancellation& cancellationToken) {
    throw "not implemented";
}
std::string GenericFileJob::GenerateOutputFilename() {
    throw "not implemented";
}
} // namespace VodArchiver
