/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
 * Description: cdc sd card update
 * Author: l00855057
 * Create: 2026-02-12
 */

#ifndef SDCARD_UPDATE_ADAPTER_H
#define SDCARD_UPDATE_ADAPTER_H

#include "sdcard_update_adapter_interface.h"
#include <vector>
#include "log/log.h"

namespace Updater {

class SdcardUpdateAdapter : public ISdcardUpdateAdapter {
public:
    SdcardUpdateAdapter() = default;
    ~SdcardUpdateAdapter() override {};
    int32_t MountSdcardPath(const std::string &path, const std::string &mountPoint) override;
    int32_t UmountPath(const std::string &path) override;
    int32_t MountPath(const std::string &path = "/data") override;
    const std::vector<std::string> GetBlockDevices(const std::string &mountPoint) override;
    std::string GetVc() override;
    std::string GetDevModel() override;
    std::string GetOemMode() override;
    bool IsMountPathSuccess(const std::string &path) override;
};

} // namespace Updater
#endif // SDCARD_UPDATE_ADAPTER_H