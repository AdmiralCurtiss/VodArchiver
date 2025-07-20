#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "vodarchiver/tasks/video-task-group.h"
#include "vodarchiver/videoinfo/i-video-info.h"
#include "vodarchiver/videojobs/i-video-job.h"

namespace VodArchiver {
struct JobList {
    std::recursive_mutex JobsLock;

    // this is a bit of a threading mess and C# handled this by being garbage collected, so DO NOT
    // DELETE anything in this vector, jobs have to stay alive once they're in here for the rest of
    // the program. we'll see later if we can fix this...
    std::vector<std::unique_ptr<IVideoJob>> JobsVector;
};

bool CreateAndEnqueueJob(JobList& jobs,
                         std::unique_ptr<IVideoInfo> info,
                         const std::function<void(IVideoJob* job)>& enqueueCallback);
bool CreateAndEnqueueJob(JobList& jobs,
                         StreamService service,
                         std::string id,
                         std::unique_ptr<IVideoInfo> info,
                         const std::function<void(IVideoJob* job)>& enqueueCallback);
bool EnqueueJob(JobList& jobs,
                std::unique_ptr<IVideoJob> job,
                const std::function<void(IVideoJob* job)>& enqueueCallback);

void AddJobToTaskGroupIfAutoenqueue(std::vector<std::unique_ptr<VideoTaskGroup>>& videoTaskGroups,
                                    IVideoJob* job);
} // namespace VodArchiver
