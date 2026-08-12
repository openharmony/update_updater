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

#include "sdcard_update.h"
#include "log/log.h"
#include "sdcard_update_action.h"
#include "sdcard_update_chain_builder.h"
#include "sdcard_update_group.h"

namespace Updater {
UpdaterStatus SdcardUpdate(UpdaterParams &upParams)
{
    LOG(INFO) << "SdcardUpdate StartFindPkg";
    std::shared_ptr<UpdateGroup> updateChain = UpdateChainBuilder()
        .Add(std::make_unique<FindSdPkgAction>())
        .Build();
    if (updateChain == nullptr) {
        LOG(ERROR) << "build update chain fail";
        return UPDATE_ERROR;
    }
    return updateChain->Handle(upParams);
}
} // Updater