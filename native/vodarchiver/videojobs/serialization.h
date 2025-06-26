#pragma once

#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include "i-video-job.h"

namespace VodArchiver {
std::optional<std::vector<std::unique_ptr<IVideoJob>>> ParseJobsFromFile(std::string_view filename);
bool WriteJobsToFile(const std::vector<std::unique_ptr<IVideoJob>>& jobs,
                     std::string_view filename);
} // namespace VodArchiver
