/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "sdcard_update_step.h"
#include "log/log.h"
 
namespace Updater {
void UpdateStep::SetNext(std::shared_ptr<UpdateStep> next)
{
    nextStep_ = next;
}

std::shared_ptr<UpdateStep> UpdateStep::Next() const
{
    return nextStep_;
}

UpdaterStatus UpdateStep::Handle(UpdaterParams &upParams)
{
    UpdaterStatus status = action_->Execute(upParams);
    LOG(INFO) << "current step status = " << static_cast<int>(status);
    if (status == UPDATE_SUCCESS && nextStep_ != nullptr) {
        return nextStep_->Handle(upParams);
    }
    return status;
}
}