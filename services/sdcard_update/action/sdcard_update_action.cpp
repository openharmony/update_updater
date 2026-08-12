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

#include "sdcard_update_action.h"
#include <dirent.h>
#include <fcntl.h>
#include <memory>
#include <string>
#include <sys/mount.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <vector>
#include "sdcard_update_adapter.h"
#include "fs_manager/mount.h"
#include "log/dump.h"
#include "log/log.h"
#include "securec.h"
#include "updater/updater_const.h"
#include "utils.h"
#include <chrono>

#ifndef UPDATER_UT
#include "language/language_ui.h"
#include "ui/updater_ui_stub.h"
#endif
 
namespace Updater {

void ISdcardUpdateAction::SetAdapter(std::unique_ptr<ISdcardUpdateAdapter> &&adapter)
{
    adapter_ = std::move(adapter);
}

UpdaterStatus FindSdPkgAction::Execute(UpdaterParams &upParams)
{
    LOG(INFO) << "FindSdPkgAction Execute";
    if (FindAndMountSdcard(upParams) != UPDATE_SUCCESS) {
        LOG(ERROR) << "mount sdcard fail!";
        return UPDATE_ERROR;
    }
    return UPDATE_SUCCESS;
}

UpdaterStatus FindSdPkgAction::FindAndMountSdcard(UpdaterParams &upParams)
{
#ifndef UPDATER_UT
    std::string mountPoint = std::string(SDCARD_PATH);
    if (adapter_ == nullptr) {
        LOG(ERROR) << "adapter_ is nullptr";
        return UPDATE_ERROR;
    }
    std::vector<std::string> sdcardStr =
        adapter_->GetBlockDevices(mountPoint);
    if (sdcardStr.empty()) {
        UPDATER_UI_INSTANCE.ShowLog(
            (errno == ENOENT) ? TR(LOG_SDCARD_NOTFIND) : TR(LOG_SDCARD_ABNORMAL), true);
        return UPDATE_ERROR;
    }
    if (!MountAndGetPkgs(sdcardStr, mountPoint, upParams)) {
        LOG(ERROR) << "mount sdcard fail!";
        return UPDATE_ERROR;
    }
#endif
    return UPDATE_SUCCESS;
}

UpdaterStatus MountSdCardAction::Execute(UpdaterParams &upParams)
{
    LOG(INFO) << "MountSdCardAction Execute";
    if (adapter_ == nullptr) {
        LOG(ERROR) << "adapter_ is nullptr";
        return UPDATE_ERROR;
    }
    if (adapter_->MountSdcardPath(item_, mountPoint_) == 0) {
        LOG(INFO) << "mount " << item_ << " sdcard success!";
        return UPDATE_SUCCESS;
    }
    return UPDATE_ERROR;
}

UpdaterStatus MountPathAction::Execute(UpdaterParams &upParams)
{
    LOG(INFO) << "MountPathAction Execute";
    if (adapter_ == nullptr) {
        LOG(ERROR) << "adapter_ is nullptr";
        return UPDATE_ERROR;
    }
    if (adapter_->MountPath(mountPath_) != 0) {
        LOG(ERROR) << "mount fail, path = " << mountPath_;
        return UPDATE_ERROR;
    }
    return UPDATE_SUCCESS;
}

UpdaterStatus UmountPathAction::Execute(UpdaterParams &upParams)
{
    LOG(INFO) << "UmountPathAction Execute";
    if (adapter_ == nullptr) {
        LOG(ERROR) << "adapter_ is nullptr";
        return UPDATE_ERROR;
    }
    if (!adapter_->IsMountPathSuccess(umountPath_)) {
        LOG(ERROR) << "not mounted";
        return UPDATE_ERROR;
    }
    if (adapter_->UmountPath(umountPath_) != 0) {
        LOG(ERROR) << "Umount fail";
    }
    return UPDATE_ERROR;
}

bool FindSdPkgAction::MountAndGetPkgs(std::vector<std::string> &sdCardStr,
    const std::string &mountPoint, UpdaterParams &upParams)
{
#ifndef UPDATER_UT
    // wait time
    unsigned int retryTimes = (upParams.sdExtMode == SDCARD_MAINIMG ||
        upParams.sdExtMode == SDCARD_NORMAL_UPDATE) ? 60 : 20;
    if (adapter_ == nullptr) {
        LOG(ERROR) << "adapter_ is nullptr";
        return false;
    }
    for (unsigned int retryCount = 1; retryCount <= retryTimes; retryCount++) {
        LOG(INFO) << "the retry time is: " << retryCount;
        for (const auto &item : sdCardStr) {
            bool mountSuccess = false;
            if (adapter_->MountSdcardPath(item, mountPoint) == 0) {
                mountSuccess = true;
            }
            if (mountSuccess && (GetSdcardPkgsPath(upParams) == UPDATE_SUCCESS)) {
                return true;
            }
            if (!mountSuccess) {
                continue;
            }
            if (adapter_->UmountPath(mountPoint) == 0) {
                LOG(INFO) << "the mounted SD card does not contain the upgrade package "
                    << item << "; success to unmount " << mountPoint;
                mountSuccess = false;
            } else {
                LOG(ERROR) << "The current mount point " << mountPoint << " unmount failed.";
                return false;
            }
        }
        sleep(1); // sleep 1 second to wait for sd card recognition
    }
    return false;
#else
    return true;
#endif
}

UpdaterStatus FindSdPkgAction::GetSdcardPkgsPath(UpdaterParams &upParams)
{
    if (upParams.updatePackage.size() != 0) {
        LOG(INFO) << "get sdcard packages from misc";
        return UPDATE_SUCCESS;
    }
    LOG(INFO) << "get sdcard packages from sdcard path";
    std::vector<std::string> sdcardPkgs = Utils::SplitString(SDCARD_CARD_PKG_PATH, ", ");
    for (auto pkgPath : sdcardPkgs) {
        if (access(pkgPath.c_str(), 0) == 0) {
            LOG(INFO) << "find sdcard package : " << pkgPath;
            upParams.updatePackage.push_back(pkgPath);
        }
    }
    if (upParams.updatePackage.empty()) {
        return UPDATE_ERROR;
    }
    return UPDATE_SUCCESS;
}
}