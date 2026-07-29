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

#ifndef SDCARD_UPDATE_PROCESS_MANAGER_H
#define SDCARD_UPDATE_PROCESS_MANAGER_H

#include <unordered_map>
#include "macros_updater.h"
#include "sdcard_update.h"
#include "updater/updater.h"

namespace Updater {

using StartFindPkgFunc = UpdaterStatus(*)(UpdaterParams &upParams);

class SdcardUpdateProcessManager {
    DISALLOW_COPY_MOVE(SdcardUpdateProcessManager);
public:
    SdcardUpdateProcessManager() = default;
    ~SdcardUpdateProcessManager() = default;
    static SdcardUpdateProcessManager &GetInstance();
    void SetSdcardUpdateFunc(StartFindPkgFunc func);
    void SetDefaultSdcardUpdateFunc(StartFindPkgFunc func);
    void RegisterSdUpdateMap(const std::string &miscInfo, StartFindPkgFunc func);
    void RegisterSdUpdateExtMap(const std::string &miscInfo, StartFindPkgFunc func);
    void ProcessSdcardUpdateExtMap();
    bool InitSdUpdateFunc();
    UpdaterStatus SdcardUpdateProcess(UpdaterParams &upParams);
private:
    std::unordered_map<std::string, StartFindPkgFunc> sdcardUpdateMap_;
    std::unordered_map<std::string, StartFindPkgFunc> sdcardUpdateExtMap_;
    StartFindPkgFunc defaultFunc_ = SdcardUpdate;
    StartFindPkgFunc sdUpdateFunc_ { nullptr };
};

#define DEFINE_EXT_FIND_PKG_FUNC(misc, funcName, ...)                                                          \
    extern "C" __attribute__((constructor)) void Register##funcName##misc(void)                               \
    {                                                                                                         \
        Updater::SdcardUpdateProcessManager::GetInstance().RegisterSdUpdateMap(misc, funcName);               \
    }                                                                                                         \
    UpdaterStatus funcName(UpdaterParams &upParams) __VA_ARGS__

#define DEFINE_PRODUCT_FIND_PKG_FUNC(misc, funcName, ...)                                                      \
    extern "C" __attribute__((constructor)) void Register##funcName##misc(void)                               \
    {                                                                                                         \
        Updater::SdcardUpdateProcessManager::GetInstance().RegisterSdUpdateExtMap(misc, funcName);            \
    }                                                                                                         \
    UpdaterStatus funcName(UpdaterParams &upParams) __VA_ARGS__

#define REGISTER_EXT_FIND_PKG_FUNC(misc, funcName)                                                             \
    extern "C" __attribute__((constructor)) void Register##funcName##misc(void)                               \
    {                                                                                                         \
        Updater::SdcardUpdateProcessManager::GetInstance().RegisterSdUpdateMap(misc, funcName);               \
    }

#define REGISTER_PRODUCT_FIND_PKG_FUNC(misc, funcName)                                                         \
    extern "C" __attribute__((constructor)) void Register##funcName##misc(void)                               \
    {                                                                                                         \
        Updater::SdcardUpdateProcessManager::GetInstance().RegisterSdUpdateExtMap(misc, funcName);            \
    }
}
#endif // SDCARD_UPDATE_PROCESS_MANAGER_H