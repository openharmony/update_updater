/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
 * Description: cdc sd card update
 * Author: l00855057
 * Create: 2026-02-12
 */

#ifndef SDCARD_UPDATE_ADAPTER_INTERFACE_H
#define SDCARD_UPDATE_ADAPTER_INTERFACE_H

#include <string>
namespace Updater {
class ISdcardUpdateAdapter {
public:
    ISdcardUpdateAdapter() = default;
    virtual ~ISdcardUpdateAdapter() = default;
    virtual int32_t MountSdcardPath(const std::string &path, const std::string &mountPoint) = 0;
    virtual int32_t UmountPath(const std::string &path) = 0;
    virtual int32_t MountPath(const std::string &path = "/data") = 0;
    virtual const std::vector<std::string> GetBlockDevices(const std::string &mountPoint) = 0;
    virtual std::string GetVc() = 0;
    virtual std::string GetDevModel() = 0;
    virtual std::string GetOemMode() = 0;
    virtual bool IsMountPathSuccess(const std::string &path) = 0;
};
} // Updater
#endif // SDCARD_UPDATE_ADAPTER_INTERFACE_H