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

#ifndef SDCARD_UPDATE_CHAIN_BUILDER_H
#define SDCARD_UPDATE_CHAIN_BUILDER_H

#include "sdcard_update_action.h"
#include "sdcard_update_group.h"
#include "updater/updater.h"

namespace Updater {
class UpdateChainBuilder {
public:
    UpdateChainBuilder &Add(std::shared_ptr<ISdcardUpdateAction> action);
    UpdateChainBuilder &NextGroup();
    std::shared_ptr<UpdateGroup> Build();
private:
    void InitGroup();
    std::shared_ptr<UpdateGroup> head_ { nullptr };
    std::shared_ptr<UpdateGroup> tail_ { nullptr };
    std::shared_ptr<UpdateGroup> currentGroup_ { nullptr };
};
}
#endif // SDCARD_UPDATE_CHAIN_BUILDER_H