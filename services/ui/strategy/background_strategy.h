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

#ifndef BACKGROUND_STRATEGY_H
#define BACKGROUND_STRATEGY_H

#include <memory>
#include <vector>
#include "page/page_manager.h"
#include "updater_ui.h"

namespace Updater {
class BackgroundStrategy {
public:
    explicit BackgroundStrategy(const ComInfo &id, const std::vector<std::string> &foregroundComIds = {}) : id_ {id},
        pgMgr_ {PageManager::GetInstance(), foregroundComIds_ (foregroundComIds)} {}
    virtual ~BackgroundStrategy() = default;
    virtual void Show() const = 0;
    virtual void Hide() const = 0;
    static std::unique_ptr<BackgroundStrategy> Factory(const std::string &type, const ComInfo &id,
        const std::vector<std::string> &foregroundComIds = {});
protected:
    void SetForegroundZIndex() const;
    ComInfo id_;
    PageManager &pgMgr_;
    std::vector<std::string> foregroundComIds_;
};

class AnimatorBackground final : public BackgroundStrategy {
public:
    explicit AnimatorBackground(const ComInfo &id,
        const std::vector<std::string> &foregroundComIds = {}) : BackgroundStrategy(id, foregroundComIds) { }
    ~AnimatorBackground() override = default;
    void Show() const override;
    void Hide() const override;
};

class StaticBackground final : public BackgroundStrategy {
public:
    explicit StaticBackground(const ComInfo &id,
        const std::vector<std::string> &foregroundComIds = {}) : BackgroundStrategy(id, foregroundComIds) { }
    ~StaticBackground() override = default;
    void Show() const override;
    void Hide() const override;
};
} // namespace Updater
#endif
