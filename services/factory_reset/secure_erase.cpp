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
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "log/dump.h"
#include "log/log.h"
#include "securec.h"
#include "fs_manager/mount.h"
#include "updater_ui_stub.h"
#include "updater/updater_const.h"
#include "utils.h"
#include "language/language_ui.h"
#include "secure_erase.h"

using namespace Updater;
using namespace std;

constexpr uint64_t OVERWRITE_SZIE = 1024 * 1024 * 1024;
constexpr uint8_t OVERWRITE_NUM = 0xFF;
constexpr int FULL_PERCENT_PROGRESS = 100;

static const std::unordered_map<uint32_t, uint32_t> BOOTDEV_TYPE_TO_ERASE_TIME = {
    {1, Updater::EMMC_ERASE_1T_TIME},
    {2, Updater::UFS_ERASE_1T_TIME},
    {4, Updater::SSD_ERASE_1T_TIME},
    {12, Updater::SSD_ERASE_1T_TIME}, // 12 : boot device is ssd when bit 2 and bit 3 are both 1
};

static uint32_t GetBootdevType()
{
    uint32_t ret = 0;
    std::ifstream fin(BOOTDEV_TYPE, std::ios::in);
    if (!fin.is_open()) {
        LOG(ERROR) << "open bootdev failed";
        return ret;
    }
    fin >> ret;
    fin.close();
    LOG(INFO) << "bootdev type is " << ret;
    return ret;
}

static uint64_t GetBootDeviceTime()
{
    uint32_t type = GetBootdevType();
    auto it = BOOTDEV_TYPE_TO_ERASE_TIME.find(type);
    if (it == BOOTDEV_TYPE_TO_ERASE_TIME.end()) {
        return Updater::UFS_ERASE_1T_TIME;
    }
    return it->second;
}

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

void SecureErase::SetSleepTime()
{
    sleepTime_ = GetBootDeviceTime() / 1024; // 1024 : calculate sleep time per 1G
    return;
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
                           to_string(remainTime.GetMinute()) + " " + TR(MINUTE_STRING);
    } else if (remainTime.GetMinute() > 0) {
        remainTimeText += " " + to_string(remainTime.GetMinute()) + " " + TR(MINUTE_STRING) + " " +
                           to_string(remainTime.GetSecond()) + " " + TR(SECOND_STRING);
    } else {
        remainTimeText += " " + to_string(remainTime.GetSecond()) + " " + TR(SECOND_STRING);
    }
    UPDATER_UI_INSTANCE.ShowLogRes(remainTimeText);
}

bool SecureErase::OverwriteSinglePartition(int fd, const PartInfo &partInfo)
{
    std::vector<uint8_t> buffer(OVERWRITE_SZIE, OVERWRITE_NUM);
    while (overwriteOffset_ < partInfo.partSize) {
        ShowRemainingTime(remainingOverWriteTime_);
        time_t start = time(nullptr);
        float value = CalcOverWriteProgress();
        ShowCurrentPercent(value);
        uint32_t writeSize = OVERWRITE_SZIE;
        if (overwriteOffset_ + writeSize > partInfo.partSize) {
            writeSize = partInfo.partSize - overwriteOffset_;
        }
        if (OverWritePartition(fd, writeSize, buffer) != 0) {
            LOG(ERROR) << "Overwrite error " << partInfo.devPath;
            UPDATER_LAST_WORD(OVERWRITE_FAILED, "overwrite failed");
            return false;
        }
        time_t end = time(nullptr);
        double eplapsed = difftime(end, start);
        uint64_t eplased_uint64 = static_cast<uint64_t>(eplapsed);
        uint64_t writeTime = CaclculateOverWriteTime(partInfo.partSize - overwriteOffset_, writeSize, eplased_uint64);
        remainingOverWriteTime_ = (remainingOverWriteTime_ == 0 || writeTime == 0) ? writeTime
        : (remainingOverWriteTime_ <= writeTime ? remainingOverWriteTime_ - 1 : writeTime);
        SyncOffsetInMisc(overwriteOffset_);
    }
    if (overwrteOffset_ == 0) {
        LOG(ERROR) << "overwrite failed, offset is 0, path: " << partInfo.devPath;
        UPDATER_LAST_WORD(OVERWRITE_FAILED, "overwrite failed, offset is 0");
        return false;
    }
    return true;
}

bool SecureErase::OverWritePartition()
{
    AddOverWritePartition("/dev/block/by-name/userdata");
    UPDATER_UI_INSTANCE.ShowProgressPage();
    if (overwritePartInfos_.empty()) {
        LOG(ERROR) << "no partition to overwrite";
        UPDATER_LAST_WORD(OVERWRITE_INVALID_INFOS, "empty partition infos");
        return false;
    }
    uint64_t totalSize = 0;
    for (const auto &partInfo : overwritePartInfos_) {
        totalSize += partInfo.partSize;
    }
    remainingOverWriteTime_ = GetEstimatedTime(totalSize);
    for (const auto &partInfo : overwritePartInfos_) {
        int fd = open(partInfo.devPath.c_str(), O_RDWR | O_LARGEFILE);
        if (fd < 0) {
            LOG(ERROR) << "open failed " << partInfo.devPath;
            UPDATER_LAST_WORD(OVERWRITE_OPEN_FAILED, "open failed");
            return false;
        }
        fdsan_exchange_owner_tag(fd, 0, FDSAN_UPDATER_TAG);
        if (!OverwriteSinglePartition(fd, partInfo)) {
            fsync(fd);
            fdsan_close_with_tag(fd, FDSAN_UPDATER_TAG);
            return false;
        }
        fsync(fd);
        fdsan_close_with_tag(fd, FDSAN_UPDATER_TAG);
    }
    LOG(INFO) << "Overwrite success";
    return true;
}

int SecureErase::OverWritePartition(int fd, const uint32_t writeSize, std::vector<uint8_t> &buffer)
{
    off64_t offset = static_cast<off64_t>(overwriteOffset_);
    int ret = lseek64(fd, offset, SEEK_SET);
    if (ret == -1) {
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

void SecureErase::AddOverWritePartitions(const std::string &factoryResetMode)
{
    if (factoryResetMode == "secure_erase") {
        AddOverWritePartition("/dev/block/by-name/userdata");
    } else if(factoryResetMode == "disk_erase") {
        AddOverWritePartition("/dev/block/by-name/userdata");
        type_ = SecureEraseType::ERASE_DATA_AND_OS;
    }
}

void SecureErase::AddOverWritePartition(const std::string &devPath)
{
    char realPath[PATH_MAX] = {0};
    if (realpath(devPath.c_str(), realPath) == nullptr) {
        LOG(ERROR) << "realpath failed " << devPath;
        return;
    }
    struct PartInfo partInfo {};
    int fd = open(realPath, O_RDONLY | O_LARGEFILE);
    if (fd < 0) {
        LOG(ERROR) << "open failed " << realPath;
        return;
    }
    uint64_t partSize = 0;
    int ret = ioctl(fd, BLKGETSIZE64, &partSize);
    close(fd);
    if (ret < 0) {
        LOG(ERROR) << "ioctl BLKGETSIZE64 failed " << realPath;
        return;
    }
    partInfo.partSize = partSize;
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
    cmdStream << "secure_erase";
    std::string cmd = cmdStream.str();
    Utils::AddUpdateInfoToMisc(cmd, offset);
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
    percent = (percent > 1.0) ? 1.0 : percent; // 1.0 : 100%
    float value = (UPDATER_UI_INSTANCE.GetCurrentPercent() > (percent * FULL_PERCENT_PROGRESS)) ?
        UPDATER_UI_INSTANCE.GetCurrentPercent() : (percent * FULL_PERCENT_PROGRESS);
    return value;
}
