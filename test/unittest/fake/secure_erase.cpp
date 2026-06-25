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

#include "secure_erase.h"

namespace Updater {
SecureErase &SecureErase::GetInstance()
{
    static SecureErase secureEraseInstance;
    return secureEraseInstance;
}

SecureErase::SecureErase() {}

void SecureErase::LoadOffsetInRetry(uint64_t offset)
{
}

void SecureErase::ShowCurrentPercent(float value)
{
}

void SecureErase::ShowRemainingTime(uint64_t remainSeconds)
{
    (void)remainingOverWriteTime_;
}

bool SecureErase::OverWritePartition(int number)
{
    return true;
}

int SecureErase::OverWritePartition(int fd, const uint32_t writeSize, std::vector<uint8_t> &buffer, int number)
{
    return 0;
}

void SecureErase::AddOverWritePartitions(const std::string &factoryResetType)
{
}

void SecureErase::AddOverWritePartition(const std::string &devPath)
{
}

void SecureErase::AddOverWritePartInfo(const PartInfo &partInfo)
{
}

void SecureErase::SyncOffsetInMisc(uint64_t offset)
{
}

float SecureErase::CalcOverWriteProgress()
{
    (void)overwriteOffset_;
    return 0;
}
}
