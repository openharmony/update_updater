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

#include "sdcard_update_process_manager.h"
#include "log/log.h"
#include "updater/updater_const.h"
#include "utils.h"
 
namespace Updater {

SdcardUpdateProcessManager &SdcardUpdateProcessManager::GetInstance()
{
    static SdcardUpdateProcessManager instance;
    return instance;
}

void SdcardUpdateProcessManager::SetDefaultSdcardUpdateFunc(StartFindPkgFunc func)
{
    defaultFunc_ = func;
}

void SdcardUpdateProcessManager::SetSdcardUpdateFunc(StartFindPkgFunc func)
{
    sdUpdateFunc_ = func;
}

void SdcardUpdateProcessManager::RegisterSdUpdateMap(const std::string &miscInfo, StartFindPkgFunc func)
{
    if (func == nullptr) {
        LOG(ERROR) << "sdcard update func is nullptr";
        return;
    }
    sdcardUpdateMap_[miscInfo] = func;
}

void SdcardUpdateProcessManager::RegisterSdUpdateExtMap(const std::string &miscInfo, StartFindPkgFunc func)
{
    if (func == nullptr) {
        LOG(ERROR) << "sdcard update ext func is nullptr";
        return;
    }
    sdcardUpdateExtMap_[miscInfo] = func;
}

void SdcardUpdateProcessManager::ProcessSdcardUpdateExtMap()
{
    for (const auto &it : sdcardUpdateExtMap_) {
        sdcardUpdateMap_[it.first] = it.second;
    }
}

bool SdcardUpdateProcessManager::InitSdUpdateFunc()
{
    ProcessSdcardUpdateExtMap();
    for (const auto &it : sdcardUpdateMap_) {
        if (Utils::CheckUpdateMode(it.first)) {
            sdUpdateFunc_ = it.second;
            break;
        }
    }
    if (sdUpdateFunc_ == nullptr) {
        if (defaultFunc_ == nullptr) {
            LOG(ERROR) << "defaultFunc_ is nullptr";
            return false;
        }
        sdUpdateFunc_ = defaultFunc_;
    }
    return true;
}

UpdaterStatus SdcardUpdateProcessManager::SdcardUpdateProcess(UpdaterParams &upParams)
{
    if (!InitSdUpdateFunc()) {
        LOG(ERROR) << "init sdUpdateFunc failed";
        return UPDATE_ERROR;
    }
    return sdUpdateFunc_(upParams);
}
}