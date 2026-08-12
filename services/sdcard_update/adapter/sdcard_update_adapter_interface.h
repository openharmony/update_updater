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