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