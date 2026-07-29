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

#include "sdcard_update_group.h"
#include "log/log.h"
 
namespace Updater {
void UpdateGroup::AddStep(std::shared_ptr<UpdateStep> step)
{
    LOG(INFO) << "UpdateGroup AddStep";
    if (!steps_.empty()) {
        steps_.back()->SetNext(step);
    }
    steps_.push_back(step);
}

void UpdateGroup::SetNextGroup(std::shared_ptr<UpdateGroup> nextGroup)
{
    nextGroup_ = nextGroup;
}

UpdaterStatus UpdateGroup::Handle(UpdaterParams &upParams)
{
    LOG(INFO) << "UpdateGroup Handle";
    if (!steps_.empty()) {
        if (steps_.front()->Handle(upParams) == UPDATE_SUCCESS) {
            LOG(INFO) << "Handle success";
            return UPDATE_SUCCESS;
        }
        if (nextGroup_ == nullptr) {
            LOG(ERROR) << "nextGroup_ is nullptr";
            return UPDATE_ERROR;
        }
        LOG(INFO) << "next group handle";
        return nextGroup_->Handle(upParams);
    }
    if (nextGroup_ == nullptr) {
        LOG(ERROR) << "nextGroup_ is nullptr";
        return UPDATE_ERROR;
    }
    return nextGroup_->Handle(upParams);
}
}