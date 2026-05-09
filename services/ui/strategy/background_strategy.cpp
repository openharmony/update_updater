/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "background_strategy.h"
#include <functional>
#include <unordered_map>
#include <vector>
#include "component/img_view_adapter.h"
#include "log/log.h"

namespace Updater {
void BackgroundStrategy::SetForegroundZIndex() const
{
    for (const auto &comId : foregroundComIds_) {
        ComInfo comInfo = {id_pageId, comId};
        if (pgMgr_.IsValid(comInfo)) {
            pgMgr_[comInfo]->SetZIndex(FOREGROUND_ZINDEX);
        }
    }
}

std::unique_ptr<BackgroundStrategy> BackgroundStrategy::Factory(const std::string &type, const ComInfo &id,
    const std::vector<std::string> &foregroundComIds)
{
    using Fun = std::function<std::unique_ptr<BackgroundStrategy>(const ComInfo &, const std::vector<std::string> &)>;
    const static std::unordered_map<std::string, Fun> bgMap {
        { "anim", [] (const ComInfo &id, const std::vector<std::string> &foregroundComIds) {
            return std::make_unique<AnimatorBackground>(id, foregroundComIds); }},
        { "img", [] (const ComInfo &id, const std::vector<std::string> &foregroundComIds) {
            return std::make_unique<StaticBackground>(id, foregroundComIds); }},
    };
    if (auto it = bgMap.find(type); it != bgMap.end()) {
        return it->second(id, foregroundComIds);
    }
    LOG(ERROR) << "not recognized background type " << type;
    return std::make_unique<StaticBackground>(id, foregroundComIds);
}

void AnimatorBackground::Show() const
{
    pgMgr_[id_]->SetVisible(true);
    pgMgr_[id_].As<ImgViewAdapter>()->Start();
    SetForegroundZIndex();
}

void AnimatorBackground::Hide() const
{
    pgMgr_[id_].As<ImgViewAdapter>()->Stop();
    pgMgr_[id_]->SetVisible(false);
}

void StaticBackground::Show() const
{
    pgMgr_[id_]->SetVisible(true);
    SetForegroundZIndex();
}

void StaticBackground::Hide() const
{
    pgMgr_[id_]->SetVisible(false);
}
} // namespace Updater