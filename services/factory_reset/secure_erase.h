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
#include "macros_updater.h"

namespace Updater {

enum {
    OVERWRITE_BASE_FAIL = 200,
    OVERWRITE_INVALID_INFOS,
    OVERWRITE_OPEN_FAILED,
    OVERWRITE_FAILED,
    OVERWRITE_INVALID_OFFSET,
};

enum SecureEraseType {
    ERASE_DATA = 0,
    ERASE_DATA_AND_OS,
};

class SecureErase {
    DISALLOW_COPY_MOVE(SecureErase);
public:
    SecureErase();
    static SecureErase &GetInstance();
    struct PartInfo {
        uint64_t partSize;
        std::string devPath;
    };

    struct SecureErasePartitionData {
        std::vector<std::string> partitionList;
        bool dataValid = false;
    };
    virtual ~SecureErase() = default;
    bool OverWritePartition(int eraseTimes);
    void AddOverWritePartitions(const std::string &factoryResetType);
    void AddOverWritePartition(const std::string &devPath);
    void LoadOffsetInRetry(uint64_t offset);
    float CalcOverWriteProgress();
    std::vector<std::string> PartitionErase();
private:
    int OverWritePartition(int fd, const uint32_t writeSize, std::vector<uint8_t> &buffer, int eraseTimes);
    bool OverwriteSinglePartition(int fd, const PartInfo &partInfo, int eraseTimes, uint64_t totalSize);
    void AddOverWritePartInfo(const PartInfo &partInfo);
    void SyncOffsetInMisc(uint64_t offset);
    void ShowRemainingTime(uint64_t remainSeconds);
    void ShowCurrentPercent(float value);
    void SetSleepTime(int eraseTimes);
    uint64_t remainingOverWriteTime_ = 0;
    uint64_t overwriteOffset_ = 0;
    uint64_t currentOffset_ = 0;
    std::vector<PartInfo> overwritePartInfos_ {};
    SecureEraseType type_ = SecureEraseType::ERASE_DATA;
    uint64_t sleepTime_ = 0;
};
}

#endif