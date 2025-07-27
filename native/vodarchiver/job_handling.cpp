#include "job_handling.h"

#include <memory>
#include <string>
#include <vector>

#include "vodarchiver/videoinfo/generic-video-info.h"
#include "vodarchiver/videoinfo/i-video-info.h"
#include "vodarchiver/videojobs/ffmpeg-reencode-job.h"
#include "vodarchiver/videojobs/generic-file-job.h"
#include "vodarchiver/videojobs/hitbox-video-job.h"
#include "vodarchiver/videojobs/i-video-job.h"
#include "vodarchiver/videojobs/twitch-chat-replay-job.h"
#include "vodarchiver/videojobs/twitch-video-job.h"
#include "vodarchiver/videojobs/youtube-video-job.h"

namespace VodArchiver {
bool CreateAndEnqueueJob(JobList& jobs,
                         std::unique_ptr<IVideoInfo> info,
                         const std::function<void(IVideoJob* job)>& enqueueCallback) {
    if (info == nullptr) {
        return false;
    }

    auto service = info->GetService();
    auto id = info->GetVideoId();
    return CreateAndEnqueueJob(jobs, service, std::move(id), std::move(info), enqueueCallback);
}

bool CreateAndEnqueueJob(JobList& jobs,
                         StreamService service,
                         std::string id,
                         std::unique_ptr<IVideoInfo> info,
                         const std::function<void(IVideoJob* job)>& enqueueCallback) {
    std::unique_ptr<IVideoJob> job;
    switch (service) {
        case StreamService::Twitch: {
            auto j = std::make_unique<TwitchVideoJob>();
            auto vi = std::make_unique<GenericVideoInfo>();
            vi->Service = StreamService::Twitch;
            vi->VideoId = std::move(id);
            j->VideoInfo = std::move(vi);
            job = std::move(j);
            break;
        }
        case StreamService::TwitchChatReplay: {
            auto j = std::make_unique<TwitchChatReplayJob>();
            auto vi = std::make_unique<GenericVideoInfo>();
            vi->Service = StreamService::TwitchChatReplay;
            vi->VideoId = std::move(id);
            j->VideoInfo = std::move(vi);
            job = std::move(j);
            break;
        }
        case StreamService::Hitbox: {
            auto j = std::make_unique<HitboxVideoJob>();
            auto vi = std::make_unique<GenericVideoInfo>();
            vi->Service = StreamService::Hitbox;
            vi->VideoId = std::move(id);
            j->VideoInfo = std::move(vi);
            job = std::move(j);
            break;
        }
        case StreamService::Youtube: {
            auto j = std::make_unique<YoutubeVideoJob>();
            auto vi = std::make_unique<GenericVideoInfo>();
            vi->Service = StreamService::Youtube;
            vi->VideoId = std::move(id);
            j->VideoInfo = std::move(vi);
            job = std::move(j);
            break;
        }
        case StreamService::RawUrl: {
            auto j = std::make_unique<GenericFileJob>();
            auto vi = std::make_unique<GenericVideoInfo>();
            vi->Service = StreamService::RawUrl;
            vi->VideoId = std::move(id);
            j->VideoInfo = std::move(vi);
            job = std::move(j);
            break;
        }
        case StreamService::FFMpegJob: {
            auto j = std::make_unique<FFMpegReencodeJob>();
            auto vi = std::make_unique<GenericVideoInfo>();
            vi->Service = StreamService::FFMpegJob;
            vi->VideoId = std::move(id);
            j->VideoInfo = std::move(vi);
            job = std::move(j);
            break;
        }
        default: return false;
    }

    if (info != nullptr) {
        job->SetVideoInfo(std::move(info));
    }

    return EnqueueJob(jobs, std::move(job), enqueueCallback);
}

bool EnqueueJob(JobList& jobs,
                std::unique_ptr<IVideoJob> job,
                const std::function<void(IVideoJob* job)>& enqueueCallback) {
    auto newVideoInfo = job->GetVideoInfo();
    if (!newVideoInfo) {
        // invalid job
        return false;
    }

    {
        std::lock_guard lock(jobs.JobsLock);

        // TODO? C# used a Set here for faster lookup.
        // Not sure if this is actually needed though, we'll see...

        // see if this job is already in the list, if yes we don't do anything
        for (size_t i = 0; i < jobs.JobsVector.size(); ++i) {
            auto& j = jobs.JobsVector[i];
            auto vi = j->GetVideoInfo();
            if (vi && vi->GetService() == newVideoInfo->GetService()
                && vi->GetVideoId() == newVideoInfo->GetVideoId()) {
                // already in list
                return false;
            }
        }

        job->SetStatus("Waiting...");
        IVideoJob* jobptr = job.get();
        jobs.JobsVector.push_back(std::move(job));

        enqueueCallback(jobptr);
    }

    // InvokeSaveJobs();

    return true;
}

void AddJobToTaskGroupIfAutoenqueue(std::vector<std::unique_ptr<VideoTaskGroup>>& videoTaskGroups,
                                    IVideoJob* job) {
    for (size_t i = 0; i < videoTaskGroups.size(); ++i) {
        auto& g = videoTaskGroups[i];
        if (g && g->Service == job->GetVideoInfo()->GetService()) {
            if (g->AutoEnqueue) {
                g->Add(job);
            }
            return;
        }
    }
}
} // namespace VodArchiver
