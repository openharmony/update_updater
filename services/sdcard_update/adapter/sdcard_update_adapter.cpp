/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
 * Description: cdc sd card update
 * Author: l00855057
 * Create: 2026-02-12
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