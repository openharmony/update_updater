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
 
#include "sdcard_update_adapter.h"
#include <sys/mount.h>
#include "fs_manager/mount.h"

namespace Updater {
int32_t SdcardUpdateAdapter::MountSdcardPath(const std::string &path, const std::string &mountPoint)
{
    return MountSdcard(path, mountPoint);
}

int32_t SdcardUpdateAdapter::UmountPath(const std::string &path)
{
    return UmountForPath(path);
}

int32_t SdcardUpdateAdapter::MountPath(const std::string &path)
{
    return MountDataForUpdate(path);
}

const std::vector<std::string> SdcardUpdateAdapter::GetBlockDevices(const std::string &mountPoint)
{
    return GetBlockDevicesByMountPoint(mountPoint);
}

bool SdcardUpdateAdapter::IsMountPathSuccess(const std::string &path)
{
    if (GetMountStatusForPath(path) != MountStatus::MOUNT_MOUNTED) {
        LOG(ERROR) << "not mounted";
        return false;
    }
    return true;
}

std::string SdcardUpdateAdapter::GetVc()
{
    return "";
}

std::string SdcardUpdateAdapter::GetDevModel()
{
    return "";
}

std::string SdcardUpdateAdapter::GetOemMode()
{
    return "";
}
}