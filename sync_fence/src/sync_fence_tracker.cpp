/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ctime>
#include <cinttypes>

#include "sync_fence_tracker.h"
#include "frame_sched.h"
#include "hilog/log.h"
#include "parameters.h"
#include "hisysevent.h"
#include "file_ex.h"

#ifndef ROSEN_TRACE_DISABLE
#include "hitrace_meter.h"
#define RS_TRACE_NAME_FMT(fmt, ...) HITRACE_METER_FMT(HITRACE_TAG_GRAPHIC_AGP, fmt, ##__VA_ARGS__)
#else
#define RS_TRACE_NAME_FMT(fmt, ...)
#endif //ROSEN_TRACE_DISABLE

#ifdef FENCE_SCHED_ENABLE
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace OHOS {
using namespace OHOS::HiviewDFX;
namespace {
#undef LOG_DOMAIN
#define LOG_DOMAIN 0xD001400
#undef LOG_TAG
#define LOG_TAG "SyncFence"

constexpr int FRAME_SET_BLUR_SIZE_ID = 100007;
constexpr int FRAME_SET_CONTAINER_NODE_ID = 100008;
const std::string ACQUIRE_FENCE_TASK = "Acquire Fence";

#ifdef FENCE_SCHED_ENABLE
constexpr unsigned int QOS_CTRL_IPC_MAGIC = 0xCC;

#define QOS_CTRL_BASIC_OPERATION \
    _IOWR(QOS_CTRL_IPC_MAGIC, 1, struct QosCtrlData)

#define QOS_APPLY 1

typedef enum {
    QOS_BACKGROUND = 0,
    QOS_UTILITY,
    QOS_DEFAULT,
    QOS_USER_INITIATED,
    QOS_DEADLINE_REQUEST,
    QOS_USER_INTERACTIVE,
    QOS_KEY_BACKGROUND,
} QosLevel;

struct QosCtrlData {
    int pid;
    unsigned int type;
    unsigned int level;
    int qos;
    int staticQos;
    int dynamicQos;
    bool tagSchedEnable = false;
};

static int TrivalOpenQosCtrlNode(void)
{
    char fileName[] = "/proc/thread-self/sched_qos_ctrl";
    int fd = open(fileName, O_RDWR);
    if (fd < 0) {
        HILOG_WARN(LOG_CORE, "open qos node failed");
    }
    return fd;
}

void QosApply(unsigned int level)
{
    int fd = TrivalOpenQosCtrlNode();
    if (fd < 0) {
        return;
    }
    fdsan_exchange_owner_tag(fd, 0, LOG_DOMAIN);

    int tid = gettid();
    struct QosCtrlData data;
    data.level = level;
    data.type = QOS_APPLY;
    data.pid = tid;
    int ret = ioctl(fd, QOS_CTRL_BASIC_OPERATION, &data);
    if (ret < 0) {
        HILOG_WARN(LOG_CORE, "qos apply failed");
    }
    fdsan_close_with_tag(fd, LOG_DOMAIN);
}
#endif
}

SyncFenceTracker::SyncFenceTracker(const std::string threadName)
    : threadName_(threadName),
    fencesQueued_(0),
    fencesSignaled_(0)
{
    runner_ = OHOS::AppExecFwk::EventRunner::Create(threadName_);
    handler_ = std::make_shared<OHOS::AppExecFwk::EventHandler>(runner_);

#ifdef FENCE_SCHED_ENABLE
    if (handler_) {
        handler_->PostTask([]() {
            QosApply(QosLevel::QOS_USER_INTERACTIVE);
        });
    }
#endif
    if (threadName_.compare(ACQUIRE_FENCE_TASK) == 0) {
        isGpuFence_ = true;
    }
    if (isGpuFence_) {
        isGpuEnable_ = OHOS::system::GetBoolParameter("persist.deadline.gpu_enable", false);
    }
    if (isGpuFence_) {
        isGpuFreq_ = OHOS::system::GetBoolParameter("persist.deadline.gpu_freq_enable", true);
    }
}

void SyncFenceTracker::TrackFence(const sptr<SyncFence>& fence, bool traceTag)
{
    if (fence == nullptr) {
        HILOG_DEBUG(LOG_CORE, "Trace fence failed, fence is null");
        return;
    }
    if (isGpuFence_) {
        bool ret = !traceTag && !isGpuEnable_ && !isGpuFreq_;
        if (ret) {
            return;
        }
    }
    if (fence->SyncFileReadTimestamp() != SyncFence::FENCE_PENDING_TIMESTAMP) {
        RS_TRACE_NAME_FMT("%s %d has signaled", threadName_.c_str(), fencesQueued_.load());
        fencesQueued_.fetch_add(1);
        fencesSignaled_.fetch_add(1);
        return;
    }

    RS_TRACE_NAME_FMT("%s %d", threadName_.c_str(), fencesQueued_.load());
    bool needSendFenceId = threadName_ == ACQUIRE_FENCE_TASK && isGpuFreq_;
    if (needSendFenceId) {
        Rosen::FrameSched::GetInstance().SendFenceId(fencesQueued_.load());
    }
    if (handler_) {
        handler_->PostTask([this, fence, traceTag]() {
            if (isGpuFence_ && isGpuEnable_) {
                Rosen::FrameSched::GetInstance().SetFrameParam(FRAME_SET_CONTAINER_NODE_ID, 0, 0, processedNodeNum_);
                processedNodeNum_ = 0;
            }
            Loop(fence, traceTag);
        });
        fencesQueued_.fetch_add(1);
    }
}

bool SyncFenceTracker::CheckGpuSubhealthEventLimit()
{
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm *tm = std::localtime(&t);
    if (tm != nullptr && (gpuSubhealthEventNum_ == 0 || tm->tm_yday > gpuSubhealthEventDay_)) {
        gpuSubhealthEventDay_ = tm->tm_yday;
        gpuSubhealthEventNum_ = 0;
        HILOG_DEBUG(LOG_CORE, "first event of %{public}" PRId32, gpuSubhealthEventDay_);
        gpuSubhealthEventNum_++;
        return true;
    } else if (gpuSubhealthEventNum_ < GPU_SUBHEALTH_EVENT_LIMIT) {
        gpuSubhealthEventNum_++;
        HILOG_DEBUG(LOG_CORE, "%{public}" PRId32 "event of %{public}" PRId32 "day",
            gpuSubhealthEventNum_, gpuSubhealthEventDay_);
        return true;
    }
    HILOG_DEBUG(LOG_CORE, "%{public}" PRId32 "event exceed %{public}" PRId32 "day",
        gpuSubhealthEventNum_, gpuSubhealthEventDay_);
    return false;
}

inline void SyncFenceTracker::UpdateFrameQueue(int32_t startTime)
{
    if (frameStartTimes_->size() >= FRAME_QUEUE_SIZE_LIMIT) {
        frameStartTimes_->pop();
    }
    frameStartTimes_->push(startTime);
}

int32_t SyncFenceTracker::GetFrameRate()
{
    int32_t frameRate = 0;
    int32_t frameNum = static_cast<int32_t>(frameStartTimes_->size());
    if (frameNum > 1) {
        int32_t interval = frameStartTimes_->back() - frameStartTimes_->front();
        if (interval > 0) {
            frameRate = FRAME_PERIOD * (frameNum - 1) / interval;
        }
    }
    return frameRate;
}

void SyncFenceTracker::ReportEventGpuSubhealth(int32_t duration)
{
    if (handler_) {
        handler_->PostTask([this, duration]() {
            RS_TRACE_NAME_FMT("report GPU_SUBHEALTH_MONITORING");
            auto reportName = "GPU_SUBHEALTH_MONITORING";
            HILOG_DEBUG(LOG_CORE, "report GPU_SUBHEALTH_MONITORING. duration : %{public}"
                PRId32, duration);
            HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::GRAPHIC, reportName,
                OHOS::HiviewDFX::HiSysEvent::EventType::STATISTIC, "WAIT_ACQUIRE_FENCE_TIME",
                duration, "FRAME_RATE", GetFrameRate());
        });
    }
}

void SyncFenceTracker::Loop(const sptr<SyncFence>& fence, bool traceTag)
{
    uint32_t fenceIndex = 0;
    fenceIndex = fencesSignaled_.load();
    {
        RS_TRACE_NAME_FMT("Waiting for %s %d", threadName_.c_str(), fenceIndex);
        int32_t result = 0;
        if (isGpuFence_ && traceTag) {
            int32_t startTimestamp = static_cast<int32_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count());
            UpdateFrameQueue(startTimestamp);
            result = WaitFence(fence);
            int32_t endTimestamp = static_cast<int32_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count());
            int32_t duration = endTimestamp - startTimestamp;
            HILOG_DEBUG(LOG_CORE, "Waiting for Acquire Fence: %{public}" PRId32 "ms", duration);
            if (duration > GPU_SUBHEALTH_EVENT_THRESHOLD && CheckGpuSubhealthEventLimit()) {
                ReportEventGpuSubhealth(duration);
            }
        } else {
            result = WaitFence(fence);
        }

        if (result < 0) {
            HILOG_DEBUG(LOG_CORE, "Error waiting for SyncFence: %s", strerror(result));
        }
    }
    fencesSignaled_.fetch_add(1);
}

int32_t SyncFenceTracker::WaitFence(const sptr<SyncFence>& fence)
{
    if (isGpuFence_ && isGpuFreq_) {
        Rosen::FrameSched::GetInstance().MonitorGpuStart(fencesSignaled_.load());
    }
    int32_t result = fence->Wait(SYNC_TIME_OUT);
    if (isGpuFence_ && isGpuFreq_) {
        Rosen::FrameSched::GetInstance().MonitorGpuEnd();
    }
    return result;
}

void SyncFenceTracker::SetBlurSize(int32_t blurSize)
{
    if (handler_) {
        handler_->PostTask([blurSize]() {
            Rosen::FrameSched::GetInstance().SetFrameParam(FRAME_SET_BLUR_SIZE_ID, 0, 0, blurSize);
        });
    }
}

void SyncFenceTracker::SetContainerNodeNum(int containerNodeNum)
{
    if (isGpuEnable_) {
        processedNodeNum_ += containerNodeNum;
    }
}
} // namespace OHOS