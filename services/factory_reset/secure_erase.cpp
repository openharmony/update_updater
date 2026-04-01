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

#include <ctime>
#include <dlfcn.h>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include "log/dump.h"
#include "log/log.h"
#include "securec.h"
#include "fs_manager/mount.h"
#include "updater_ui_stub.h"
#include "utils.h"
#include "language/language_ui.h"

using namespace Updater;
using namespace std;

constexpr uint64_t OVERWRITE_SZIE = 1024 * 1024 * 1024;
constexpr uint8_t OVERWRITE_NUM = 0xFF;
constexpr int FULL_PERCENT_PROGRESS = 100;

static uint64_t CaculatePerSpeed(uint64_t offset, uint64_t time)
{
    uint64_t perSpeed = 0;
    if (time != 0) {
        perSpeed = offset / time;
    }
    return perSpeed;
}

static uint64_t CaculateOverWriteTime(uint64_t partSize, uint64_t offset, uint64_t passedTime)
{
    uint64_t seconds = 0;
    uint64_t perSpeed = CaculatePerSpeed(offset, passedTime);
    if (perSpeed != 0) {
        seconds = (partSize > offset ? partSize - offset : 0) / perSpeed;
    }
    return seconds;
}

SecureErase &SecureErase::GetInstance()
{
    static SecureErase secureEraseInstance;
    return secureEraseInstance;
}

SecureErase::SecureErase() {}

void SecureErase::LoadOffsetInRetry(uint64_t offset)
{
    overwriteOffset_ = offset;
}

void SecureErase::ShowCurrentPercent(float value)
{
    UPDATER_UI_INSTANCE.ShowProgress(value);
    std::string percentText = TR(LABEL_SECURE_ERASING);
    percentText += " " + to_string(static_cast<uint32_t>(floor(value))) + "%";
    UPDATER_UI_INSTANCE.ShowUpdInfo(percentText);
}

void SecureErase::ShowRemainingTime(uint64_t remainSeconds)
{
    Utils::Time remainTime(remainSeconds);
    std::string remainTimeText = TR(LABEL_REMAIN_TIME);
    if (remainTime.GetHour() > 0) {
        remainTimeText += " " + to_string(remainTime.GetHour()) + " " + TR(HOUR_STRING) + " " +
                           to_string(remainTime.GetMin()) + " " + TR(MINUTE_STRING);
    } else if (remainTime.GetMin() > 0) {
        remainTimeText += " " + to_string(remainTime.GetMin()) + " " + TR(MINUTE_STRING) + " " +
                           to_string(remainTime.GetSec()) + " " + TR(SECOND_STRING);
    } else {
        remainTimeText += " " + to_string(remainTime.GetSec()) + " " + TR(SECOND_STRING);
    }
    UPDATER_UI_INSTANCE.ShowLogRes(remainTimeText);
}

bool SecureErase::OverWritePartition()
{
    AddOverWritePartition("/dev/block/bootdevice/by-name/userdata");
    Updater_UI_INSTANCE.ShowProgressPage();
    if (overwritePartInfos_.empty()) {
        LOG(ERROR) << "no partition to overwrite";
        return false;
    }
    for (const auto &partInfo : overwritePartInfos_) {
        time_t start = time(nullptr);
        std::vector<uint8_t> buffer(OVERWRITE_SZIE, OVERWRITE_NUM);
        int fd = open(partInfo.devPath.c_str(), O_RDWR | O_LARGEFILE);
        if (fd < 0) {
            LOG(ERROR) << "open failed " << partInfo.devPath;
            return false;
        }
        while (overwriteOffset_ < partInfo.partSize) {
            ShowRemainingTime(remainingOverWriteTime_);
            float value = CalcOverWriteProgress();
            ShowCurrentPercent(value);
            uint32_t writeSize = OVERWRITE_SZIE;
            if (overwriteOffset_ + writeSize > partInfo.partSize) {
                writeSize = partInfo.partSize - overwriteOffset_;
            }
            if (OverWritePartition(fd, writeSize, buffer) != 0) {
                LOG(ERROR) << "Overwrite error " << partInfo.devPath;
                fsync(fd);
                close(fd);
                return false;
            }
            time_t end = time(nullptr);
            double eplapsed = difftime(end, start);
            uint64_t eplased_uint64 = static_cast<uint64_t>(eplapsed);
            remainingOverWriteTime_ = CaculateOverWriteTime(partInfo.partSize, overwriteOffset_, eplased_uint64);
            SyncOffsetInMisc(overwriteOffset_);
        }
        fsync(fd);
        close(fd);
        if (overwriteOffset_ == 0) {
            LOG(ERROR) << "Overwrite error, offset is 0 " << partInfo.devPath;
            return false;
        }
    }
    LOG(INFO) << "Overwrite success";
    return true;
}

int SecureErase::OverWritePartition(int fd, const uint32_t writeSize, std::vector<uint8_t> &buffer)
{
    off64_t offset = static_cast<off64_t>(overwriteOffset_);
    int ret = lseek64(fd, offset, SEEK_SET);
    if (ret < 0) {
        LOG(ERROR) << "lseek64 failed, offset: " << offset << ", error: " << strerror(errno);
        return -1;
    }
    if (!Utils::WriteFully(fd, buffer.data(), writeSize)) {
        LOG(ERROR) << "write fd failed";
        return -1;
    }
    overwriteOffset_ += writeSize;
    return 0;
}

void SecureErase::AddOverWritePartition(const std::string &devPath)
{
    char realPath[PATH_MAX] = {0};
    if (realpath(devPath.c_str(), realPath) == nullptr) {
        LOG(ERROR) << "realpath failed " << devPath;
        return;
    }
    struct stat st {};
    struct PartInfo partInfo {};
    if (stat(realPath, &st) != 0) {
        LOG(ERROR) << "stat failed " << realPath;
        return;
    }
    partInfo.partSize = static_cast<uint64_t>(st.st_size);
    partInfo.devPath = realPath;
    AddOverWritePartInfo(partInfo);
}

void SecureErase::AddOverWritePartInfo(const PartInfo &partInfo)
{
    if (partInfo.partSize == 0) {
        LOG(ERROR) << "partition size is 0, path: " << partInfo.devPath;
        return;
    }
    overwritePartInfos_.emplace_back(partInfo);
}

void SecureErase::SyncOffsetInMisc(uint64_t offset)
{
    std::ostringstream cmdStream;
    cmdStream << "--secure_erase=" << offset;
    std::string cmd = cmdStream.str();
    Utils::SetCmdToMisc(cmd);
}

float SecureErase::CalcOverWriteProgress()
{
    uint64_t totalSize = 0;
    for (const auto &partInfo : overwritePartInfos_) {
        totalSize += partInfo.partSize;
    }
    if (totalSize == 0) {
        return 0;
    }
    float percent = static_cast<double>(overwriteOffset_) / static_cast<double>(totalSize);
    float value = (UPDATER_UI_INSTANCE.GetCurrentPercent() > (percent * FULL_PERCENT_PROGRESS)) ?
        UPDATER_UI_INSTANCE.GetCurrentPercent() : (percent * FULL_PERCENT_PROGRESS);
    return value;
}
