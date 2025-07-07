#pragma once

#include <memory>
#include <string>
#include <vector>

#include "vodarchiver/videoinfo/i-video-info.h"
#include "vodarchiver/videojobs/i-video-job.h"

namespace VodArchiver {
bool CreateAndEnqueueJob(std::vector<std::unique_ptr<IVideoJob>>& jobs,
                         std::unique_ptr<IVideoInfo> info);
bool CreateAndEnqueueJob(std::vector<std::unique_ptr<IVideoJob>>& jobs,
                         StreamService service,
                         std::string id,
                         std::unique_ptr<IVideoInfo> info = nullptr);
bool EnqueueJob(std::vector<std::unique_ptr<IVideoJob>>& jobs, std::unique_ptr<IVideoJob> job);
} // namespace VodArchiver
