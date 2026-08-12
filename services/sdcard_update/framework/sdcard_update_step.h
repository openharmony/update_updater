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

#ifndef SDCARD_UPDATE_STEP_H
#define SDCARD_UPDATE_STEP_H

#include <unordered_map>
#include "sdcard_update_action.h"
#include "updater/updater.h"

namespace Updater {

class UpdateStep {
public:
    explicit UpdateStep(std::shared_ptr<ISdcardUpdateAction> action) : action_(action) {}
    UpdaterStatus Handle(UpdaterParams &upParams);
    void SetNext(std::shared_ptr<UpdateStep> next);

private:
    std::shared_ptr<UpdateStep> Next() const;
    std::shared_ptr<ISdcardUpdateAction> action_;
    std::shared_ptr<UpdateStep> nextStep_;
};
}
#endif // SDCARD_UPDATE_STEP_H