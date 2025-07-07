#include "job_handling.h"

#include <memory>
#include <string>
#include <vector>

#include "vodarchiver/statusupdate/null-status-update.h"
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
bool CreateAndEnqueueJob(std::vector<std::unique_ptr<IVideoJob>>& jobs,
                         std::unique_ptr<IVideoInfo> info) {
    if (info == nullptr) {
        return false;
    }

    auto service = info->GetService();
    auto id = info->GetVideoId();
    return CreateAndEnqueueJob(jobs, service, std::move(id), std::move(info));
}

bool CreateAndEnqueueJob(std::vector<std::unique_ptr<IVideoJob>>& jobs,
                         StreamService service,
                         std::string id,
                         std::unique_ptr<IVideoInfo> info) {
    std::unique_ptr<IVideoJob> job;
    switch (service) {
        case StreamService::Twitch: {
            auto j = std::make_unique<TwitchVideoJob>();
            auto vi = std::make_unique<GenericVideoInfo>();
            vi->Service = StreamService::Twitch;
            vi->VideoId = std::move(id);
            j->_VideoInfo = std::move(vi);
            job = std::move(j);
            break;
        }
        case StreamService::TwitchChatReplay: {
            auto j = std::make_unique<TwitchChatReplayJob>();
            auto vi = std::make_unique<GenericVideoInfo>();
            vi->Service = StreamService::TwitchChatReplay;
            vi->VideoId = std::move(id);
            j->_VideoInfo = std::move(vi);
            job = std::move(j);
            break;
        }
        case StreamService::Hitbox: {
            auto j = std::make_unique<HitboxVideoJob>();
            auto vi = std::make_unique<GenericVideoInfo>();
            vi->Service = StreamService::Hitbox;
            vi->VideoId = std::move(id);
            j->_VideoInfo = std::move(vi);
            job = std::move(j);
            break;
        }
        case StreamService::Youtube: {
            auto j = std::make_unique<YoutubeVideoJob>();
            auto vi = std::make_unique<GenericVideoInfo>();
            vi->Service = StreamService::Youtube;
            vi->VideoId = std::move(id);
            j->_VideoInfo = std::move(vi);
            job = std::move(j);
            break;
        }
        case StreamService::RawUrl: {
            auto j = std::make_unique<GenericFileJob>();
            auto vi = std::make_unique<GenericVideoInfo>();
            vi->Service = StreamService::RawUrl;
            vi->VideoId = std::move(id);
            j->_VideoInfo = std::move(vi);
            job = std::move(j);
            break;
        }
        case StreamService::FFMpegJob: {
            auto j = std::make_unique<FFMpegReencodeJob>();
            auto vi = std::make_unique<GenericVideoInfo>();
            vi->Service = StreamService::FFMpegJob;
            vi->VideoId = std::move(id);
            j->_VideoInfo = std::move(vi);
            job = std::move(j);
            break;
        }
        default: return false;
    }

    if (info != nullptr) {
        job->SetVideoInfo(std::move(info));
    }

    return EnqueueJob(jobs, std::move(job));
}

bool EnqueueJob(std::vector<std::unique_ptr<IVideoJob>>& jobs, std::unique_ptr<IVideoJob> job) {
    // if (JobSet.Contains(job)) {
    //     return false;
    // }

    job->StatusUpdater = std::make_unique<NullStatusUpdate>();
    // .StatusUpdater =
    //     new StatusUpdate.ObjectListViewStatusUpdate(ObjectListViewUpdater, job);
    job->Index = jobs.size();
    job->SetStatus("Waiting...");
    jobs.push_back(std::move(job));
    // JobSet.Add(job);
    // if (VideoTaskGroups[job.VideoInfo.Service].AutoEnqueue) {
    //     VideoTaskGroups[job.VideoInfo.Service].Add(new WaitingVideoJob(job));
    // }

    // InvokeSaveJobs();

    return true;
}
} // namespace VodArchiver
