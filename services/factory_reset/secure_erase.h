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

#ifndef SECURE_ERASE_H
#define SECURE_ERASE_H
#include <string>

namespace Updater {
class SecureErase {
    DISALLOW_COPY_AND_MOVE(SecureErase);
public:
    static SecureErase &GetInstance();
    struct PartInfo {
        uint64_t partSize;
        std::string devPath;
    };
    virtual ~SecureErase() = default;
    static SecureErase &GetInstance();
    bool OverWritePartition();
    void AddOverWritePartInfo(const std::string &devPath);
    void LoadOffsetInRetry(uint64_t offset);
    float CalcOverWriteProgress();
private:
    int OverWritePartition(int fd, const unit32_t writeSize, std::vector<uint8_t> &buffer);
    void AddOverWritePartInfo(const PartInfo &partInfo);
    void SyncOffsetInMisc(uint64_t offset);
    void ShowRemainingTime(uint64_t remainSeconds);
    void ShowCurrentSpeed(float value);
    uint64_t remainingOverWriteTime_ = 0;
    uint64_t overwriteOffset_ = 0;
    std::vector<PartInfo> overwritePartInfos_ {};
};
}

#endif