/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#ifndef DELEGATOR_SURFACE_TEST_UNITTEST_H
#define DELEGATOR_SURFACE_TEST_UNITTEST_H

#include <consumer_surface.h>
#include <gtest/gtest.h>
#include <message_option.h>
#include <message_parcel.h>
#include <native_window.h>
#include <securec.h>
#include <surface.h>

#include "buffer_consumer_listener.h"
#include "sync_fence.h"
#include "consumer_surface.h"
#include "producer_surface_delegator.h"
#include "buffer_queue_producer.h"

namespace OHOS::Rosen {
class IRemoteObjectMocker : public IRemoteObject {
public:
    IRemoteObjectMocker() : IRemoteObject{u"IRemoteObjectMocker"} {}
    ~IRemoteObjectMocker() {}

    int32_t GetObjectRefCount()
    {
        return 0;
    }

    int SendRequest(uint32_t code, MessageParcel& data, MessageParcel& reply, MessageOption& option)
    {
        return 0;
    }

    bool IsProxyObject() const
    {
        return true;
    }

    bool CheckObjectLegality() const
    {
        return true;
    }

    bool AddDeathRecipient(const sptr<DeathRecipient>& recipient)
    {
        return true;
    }

    bool RemoveDeathRecipient(const sptr<DeathRecipient>& recipient)
    {
        return true;
    }

    sptr<IRemoteBroker> AsInterface()
    {
        return nullptr;
    }

    int Dump(int fd, const std::vector<std::u16string>& args)
    {
        return 0;
    }

    GSError DequeueBuffer(const BufferRequestConfig& config, sptr<BufferExtraData>& bedata,
        struct IBufferProducer::RequestBufferReturnValue& retval)
    {
        return GSError::GSERROR_OK;
    }
};
}

#endif // DELEGATOR_SURFACE_TEST_UNITTEST_H