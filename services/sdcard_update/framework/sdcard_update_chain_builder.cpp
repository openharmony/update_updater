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

#include "sdcard_update_chain_builder.h"
#include "sdcard_update_action.h"
#include "log/log.h"
#include <memory>
 
namespace Updater {

UpdateChainBuilder &UpdateChainBuilder::Add(std::shared_ptr<ISdcardUpdateAction> action)
{
    InitGroup();
    currentGroup_->AddStep(std::make_unique<UpdateStep>(std::move(action)));
    return *this;
}

UpdateChainBuilder &UpdateChainBuilder::NextGroup()
{
    if (currentGroup_ != nullptr) {
        if (head_ == nullptr) {
            head_ = currentGroup_;
        } else {
            tail_->SetNextGroup(currentGroup_);
        }
        tail_ = currentGroup_;
        currentGroup_ = nullptr;
    }
    return *this;
}

std::shared_ptr<UpdateGroup> UpdateChainBuilder::Build()
{
    NextGroup();
    return head_;
}

void UpdateChainBuilder::InitGroup()
{
    if (currentGroup_ == nullptr) {
        currentGroup_ = std::make_unique<UpdateGroup>();
    }
}
}