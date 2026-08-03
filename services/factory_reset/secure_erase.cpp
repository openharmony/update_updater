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

#include <algorithm>
#include <cctype>
#include <ctime>
#include <dirent.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <linux/fs.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "log/dump.h"
#include "log/log.h"
#include "ptable.h"
#include "ptable_manager.h"
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
constexpr const char *USERDATA_PATH = "/dev/block/by-name/userdata";
constexpr const char *SECURE_ERASE_PARTITION_PATH = "/vendor/etc/secure_erase_partition.json";
constexpr const size_t MIN_NVME_NAME_LENGTH = 7;
constexpr const size_t NVME_BASE_LEN = 4;


static const std::unordered_map<uint32_t, uint64_t> bootDeviceTimeMap = {
    {1, Updater::UFS_ERASE_1T_TIME},
    {2, Updater::EMMC_ERASE_1T_TIME},
    {4, Updater::SSD_ERASE_1T_TIME},
    {12, Updater::SSD_ERASE_1T_TIME}
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

static uint64_t CalculatePerSpeed(uint64_t offset, uint64_t time)
{
    uint64_t perSpeed = 0;
    if (time != 0) {
        perSpeed = offset / time;
    }
    return perSpeed;
}

static uint64_t CalculateOverWriteTime(uint64_t partSize, uint64_t offset, uint64_t passedTime)
{
    uint64_t seconds = 0;
    uint64_t perSpeed = CalculatePerSpeed(offset, passedTime);
    if (perSpeed != 0) {
        seconds = partSize / perSpeed;
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

static uint64_t GetBootDeviceTime()
{
    uint32_t type = GetBootdevType();
    auto it = bootDeviceTimeMap.find(type);
    if (it == bootDeviceTimeMap.end()) {
        return Updater::UFS_ERASE_1T_TIME;
    }
    return it->second;
}

static uint64_t GetEstimatedTime(uint64_t partSize, int eraseTimes)
{
    uint64_t bootDevicetTime = GetBootDeviceTime();
    double sizeRatio = static_cast<double>(partSize) / Updater::DEFAULT_1T_SIZE;
    sizeRatio = sizeRatio * eraseTimes;
    uint64_t estimatedTime = static_cast<uint64_t>(sizeRatio * bootDevicetTime);
    LOG(INFO) << "secure erase estimated time is " << estimatedTime;
    return estimatedTime;
}

void SecureErase::SetSleepTime(int eraseTimes)
{
    /* set sleepTime to limit overwrite speed */
    sleepTime_ = GetBootDeviceTime() * static_cast<uint64_t>(eraseTimes) / 1024; // 1T = 1024 * 1G
    return;
}

void SecureErase::ShowCurrentPercent(float value)
{
    UPDATER_UI_INSTANCE.ShowProgress(value);
    std::string percentText = TR(LABEL_SECURE_ERASING);
    if (type_ == SecureEraseType::ERASE_DATA_AND_OS) {
        percentText = TR(LABEL_DISK_ERASING);
    }
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

bool SecureErase::OverwriteSinglePartition(int fd, const PartInfo &partInfo, int eraseTimes, uint64_t totalSize)
{
    std::vector<uint8_t> buffer(OVERWRITE_SZIE, OVERWRITE_NUM);
    while (currentOffset_ < partInfo.partSize) {
        ShowRemainingTime(remainingOverWriteTime_);
        time_t start = time(nullptr);
        float value = CalcOverWriteProgress();
        ShowCurrentPercent(value);
        uint32_t writeSize = OVERWRITE_SZIE;
        if (currentOffset_ + writeSize > partInfo.partSize) {
            writeSize = partInfo.partSize - currentOffset_;
        }
        if (OverWritePartition(fd, writeSize, buffer, eraseTimes) != 0) {
            LOG(ERROR) << "Overwrite error " << partInfo.devPath;
            UPDATER_LAST_WORD(OVERWRITE_FAILED, "overwrite failed");
            return false;
        }

        time_t end = time(nullptr);
        double eplapsed = difftime(end, start);
        uint64_t eplasedUint64 = static_cast<uint64_t>(eplapsed);
        uint64_t writeTime =
            CalculateOverWriteTime(totalSize - overwriteOffset_, writeSize, eplasedUint64);

        remainingOverWriteTime_ = (remainingOverWriteTime_ == 0 || writeTime == 0) ? writeTime
            : (remainingOverWriteTime_ <= writeTime ? remainingOverWriteTime_ - 1 : writeTime);
        SyncOffsetInMisc(overwriteOffset_);
    }
    if (currentOffset_ == 0) {
        LOG(ERROR) << "Overwrite error, offset is 0 " << partInfo.devPath;
        UPDATER_LAST_WORD(OVERWRITE_INVALID_OFFSET, "overwrite offset is 0");
        return false;
    }
    return true;
}

bool SecureErase::OverWritePartition(int eraseTimes)
{
    UPDATER_UI_INSTANCE.ShowProgressPage();
    if (type_ == SecureEraseType::ERASE_DATA_AND_OS) {
        eraseTimes = 3; // 3 : disk overwrite
    }
    SetSleepTime(eraseTimes);
    if (overwritePartInfos_.empty()) {
        LOG(ERROR) << "no partition to overwrite";
        UPDATER_LAST_WORD(OVERWRITE_INVALID_INFOS, "empty partition infos");
        return false;
    }
    uint64_t totalSize = 0;
    for (const auto &partInfo : overwritePartInfos_) {
        totalSize += partInfo.partSize;
    }
    remainingOverWriteTime_ = GetEstimatedTime(totalSize, eraseTimes);
    currentOffset_ = overwriteOffset_;
    for (const auto &partInfo : overwritePartInfos_) {
        LOG(INFO) << "open " << partInfo.devPath << "size " << partInfo.partSize
            << "currentOffset_: " << currentOffset_;
        if (currentOffset_ >= partInfo.partSize) {
            currentOffset_ -= partInfo.partSize;
            continue;
        }
        int fd = open(partInfo.devPath.c_str(), O_RDWR | O_LARGEFILE);
        if (fd < 0) {
            LOG(ERROR) << "open failed " << partInfo.devPath;
            UPDATER_LAST_WORD(OVERWRITE_OPEN_FAILED, "open failed");
            return false;
        }
        fdsan_exchange_owner_tag(fd, 0, FDSAN_UPDATER_TAG);
        if (!OverwriteSinglePartition(fd, partInfo, eraseTimes, totalSize)) {
            fsync(fd);
            fdsan_close_with_tag(fd, FDSAN_UPDATER_TAG);
            return false;
        }
        currentOffset_ = 0;
        fsync(fd);
        fdsan_close_with_tag(fd, FDSAN_UPDATER_TAG);
    }
    LOG(INFO) << "Overwrite success";
    return true;
}

bool CheckNvmeFormat(const std::string& deviceName)
{
    if (deviceName.find("nvme0") != std::string::npos) {
        return false;
    }
    if (deviceName.size() < MIN_NVME_NAME_LENGTH) {
        LOG(ERROR) << "Ignore nvme name too short: " << deviceName;
        return false;
    }
    size_t pos = NVME_BASE_LEN;
    if (!std::isdigit(static_cast<unsigned char>(deviceName[pos]))) {
        LOG(ERROR) << "Ignore invalid nvme format: missing controller ID in " << deviceName;
        return false;
    }
    while (pos < deviceName.size() && std::isdigit(static_cast<unsigned char>(deviceName[pos]))) {
        ++pos;
    }
    if (pos >= deviceName.size() || deviceName[pos] != 'n') {
        LOG(ERROR) << "Ignore invalid nvme format: missing 'n' in " << deviceName;
        return false;
    }
    ++pos;
    if (pos >= deviceName.size() || !std::isdigit(static_cast<unsigned char>(deviceName[pos]))) {
        LOG(ERROR) << "Ignore invalid nvme format: missing partition ID in " << deviceName;
        return false;
    }
    return true;
}

bool IsDataDisk(const std::string &deviceName)
{
    if (deviceName.find("nvme") != std::string::npos) {
        if (!CheckNvmeFormat(deviceName)) {
            return false;
        }
        return true;
    }
    if (deviceName.find("sd") == std::string::npos) {
        return false;
    }
    std::string removablePath = "/sys/block/" + deviceName + "/removable";
    if (access(removablePath.c_str(), F_OK) == 0) {
        std::string removable = Utils::ReadFile(removablePath);
        if (removable == "1") {
            LOG(ERROR) << "removable: " << removable;
            return false;
        }
    }
    std::string devicePath = "/sys/block/" + deviceName;
    char linkTarget[PATH_MAX];
    ssize_t len = readlink(devicePath.c_str(), linkTarget, sizeof(linkTarget) - 1);
    if (len != -1) {
        linkTarget[len] = '\0';
        std::string targetPath(linkTarget);
        if (targetPath.find("ufs") != std::string::npos) {
            LOG(ERROR) << "UFS " << targetPath;
            return false;
        }
    } else {
        LOG(ERROR) << "readlink failed": << devicePath
        return false;
    }
    return true;
}

std::vector<std::string> ScanBlockDevices(const char *path)
{
    std::vector<std::string> blockDevices;
    if (access(path, F_OK) == -1) {
        LOG(ERROR) << "The path does not exist. " << path;
        return blockDevices;
    }
    DIR *dir = opendir(path);
    if (!dir) {
        LOG(ERROR) << "opendir ERROR: " << path;
        return blockDevices;
    }
    struct dirent *ent;
    while ((ent = readdir(dir)) != nullptr) {
        std::string deviceName = ent->d_name;
        if (deviceName == "." || deviceName == "..") {
            LOG(INFO) << "Ignore: " << deviceName;
            continue;
        }
        if (deviceName.find("nvme") != std::string::npos || deviceName.find("sd") != std::string::npos) {
            if (IsDataDisk(deviceName)) {
                LOG(INFO) << "deviceName: " << deviceName;
                blockDevices.push_back("/dev/block/" + deviceName);
            }
        }
    }
    closedir(dir);
    return blockDevices;
}

bool ParseSecureErasePartitionNode(const cJSON* node, SecureErase::SecureErasePartitionData& data)
{
    if (!cJSON_IsArray(node)) {
        LOG(ERROR) << "secureErasePartition is not an array";
        return false;
    }

    cJSON* item = nullptr;
    cJSON_ArrayForEach(item, node) {
        if (!cJSON_IsString(item)) {
            LOG(ERROR) << "Non-string item found in secureErasePartition";
            continue;
        }
        data.partitionList.push_back(item->valuestring);
    }

    data.dataValid = true;
    return true;
}

std::vector<std::string> ExtractSecureErasePartition(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        LOG(ERROR) << "Failed to open file! " << filename;
        return {};
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    cJSONPtr root(cJSON_Parse(content.c_str()), cJSON_Delete);
    if (root == nullptr) {
        LOG(ERROR) << "JSON parse error";
        return {};
    }

    cJSON* ptableNode = nullptr;
    if (cJSON_HasObjectItem(root.get(), "secureErasePartition")) {
        ptableNode = cJSON_GetObjectItem(root.get(), "secureErasePartition");
    }
    if (ptableNode == nullptr) {
        LOG(ERROR) << "secureErasePartition not found in JSON";
        return {};
    }

    SecureErase::SecureErasePartitionData data;
    bool ret = ParseSecureErasePartitionNode(ptableNode, data);
    if (ret) {
        return data.partitionList;
    } else {
        LOG(ERROR) << "Failed to parse secureErasePartition";
        return {};
    }
}

std::vector<std::string> SecureErase::PartitionErase(const std::string &factoryResetType)
{
    std::vector<std::string> partitionErasePatch;
    DevicePtable& devicePtb = DevicePtable::GetInstance();
    if (!devicePtb.LoadPartitionInfo()) {
        LOG(ERROR) << "LoadPartitionInfo error";
        return partitionErasePatch;
    }
    std::vector<Ptable::PtnInfo> ptableData = devicePtb.pPtable_->GetPtablePartitionInfo();
    bool foundCust = false;
    std::string devPath;
    partitionErasePatch.push_back(USERDATA_PATH);
    if (factoryResetType == "disk_erase") {
        for (const auto &ptnInfo : ptableData) {
            std::string lowerDispName = ptnInfo.dispName;
            std::transform(lowerDispName.begin(), lowerDispName.end(), lowerDispName.begin(),
                [](unsigned char c) { return std::tolower(c); });
            if (lowerDispName == "cust") {
                foundCust = true;
            }

            if (lowerDispName == "userdata") {
                continue;
            }

            if (foundCust) {
                devPath = "/dev/block/by-name/" + lowerDispName;
                LOG(INFO) << "ptable dispName CUST later " << devPath;
                partitionErasePatch.push_back(devPath);
            }
        }
        if (!foundCust) {
            LOG(ERROR) << "cust partition not found";
            return partitionErasePatch;
        }

        std::vector<std::string> secureErasePartition = ExtractSecureErasePartition(SECURE_ERASE_PARTITION_PATH);
        for (const auto &partitionPath : secureErasePartition) {
            LOG(INFO) << "secureErasePartition PATH " << partitionPath;
            partitionErasePatch.push_back("/dev/block/by-name/" + partitionPath);
        }
    }

    AddBlockDevices(partitionErasePatch);
    return partitionErasePatch;
}

void SecureErase::AddBlockDevices(std::vector<std::string> &partitionErasePatch)
{
    std::vector<std::string> blockDevices;
    const char *path = "/sys/block";
    blockDevices = ScanBlockDevices(path);
    for (const auto &blockpath : blockDevices) {
        LOG(INFO) << "Data Disk Path " << blockpath;
        partitionErasePatch.push_back(blockpath);
    }
}

int SecureErase::OverWritePartition(int fd, const uint32_t writeSize, std::vector<uint8_t> &buffer, int number)
{
    time_t overwriteStart = time(nullptr);
    off64_t offset = static_cast<off64_t>(currentOffset_);
    int ret = 0;
    for (int i = 0; i < number; i++) {
        ret = lseek64(fd, offset, SEEK_SET);
        if (ret == -1) {
            LOG(ERROR) << "lseek64 failed, offset: " << offset << ", error: " << strerror(errno);
            return -1;
        }
        if (!Utils::WriteFully(fd, buffer.data(), writeSize)) {
            LOG(ERROR) << "write fd failed";
            return -1;
        }
    }
    currentOffset_ += writeSize;
    overwriteOffset_ += writeSize;
    time_t overwriteEnd = time(nullptr);
    double elapsed = difftime(overwriteEnd, overwriteStart);
    uint64_t elapsedUint64 = static_cast<uint64_t>(elapsed);
    if (elapsedUint64 < sleepTime_) {
        sleep(sleepTime_ - elapsedUint64);
    }
    return 0;
}

void SecureErase::AddOverWritePartitions(const std::string &factoryResetType)
{
    if (factoryResetType == "disk_erase") {
        type_ = SecureEraseType::ERASE_DATA_AND_OS;
    }
    if (factoryResetType == "disk_erase" || factoryResetType == "secure_erase") {
        std::vector<std::string> partitionErase = PartitionErase(factoryResetType);
        for (const auto &part : partitionErase) {
            LOG(INFO) << "AddOverWritePartition " << part;
            AddOverWritePartition(part);
        }
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
    fdsan_exchange_owner_tag(fd, 0, FDSAN_UPDATER_TAG);
    uint64_t partSize = 0;
    int ret = ioctl(fd, BLKGETSIZE64, &partSize);
    fdsan_close_with_tag(fd, FDSAN_UPDATER_TAG);
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
    LOG(INFO) << "partInfo.devPath " << partInfo.devPath << " partInfo.partSize " << partInfo.partSize;
    overwritePartInfos_.emplace_back(partInfo);
}

void SecureErase::SyncOffsetInMisc(uint64_t offset)
{
    std::ostringstream cmdStream;
    std::string eraseCmd = "secure_erase";
    if (type_ == SecureEraseType::ERASE_DATA_AND_OS) {
        eraseCmd = "disk_erase";
    }
    cmdStream << eraseCmd;
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
