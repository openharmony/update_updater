/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
 
#include "io_collect.h"
#include <sstream>
#include <string>
#include <vector>
#include "log.h"
#include "utils.h"
 
namespace Updater {
using namespace Utils;
thread_local static ProcessIo g_tmpProcessIo {};
thread_local static ProcessIo g_totalProcessIo {};
 
std::string GetCollectTotalIo(void)
{
    std::stringstream ss;
    ss << g_totalProcessIo;
    return ss.str();
}
 
void ResetCollectTotalIo(void)
{
    g_totalProcessIo = {};
}
 
void ResetCollectTmpIo(void)
{
    g_tmpProcessIo = {};
}
 
bool ParseTypeAndValue(const std::string &str, std::string &type, int64_t &value)
{
    std::string::size_type typePos = str.find(":");
    if (typePos != std::string::npos) {
        type = str.substr(0, typePos);
        std::string valueStr = str.substr(typePos + 1);
        std::string::size_type valuePos = valueStr.find("kB");
        if (valuePos == std::string::npos) {
            valuePos = valueStr.find("KB");
        }
        if (valuePos != std::string::npos) {
            valueStr.resize(valuePos);
            value = String2Int<int64_t>(valueStr, Utils::N_DEC);
            return true;
        } else {
            value = String2Int<int64_t>(valueStr, Utils::N_DEC);
            return true;
        }
    }
    return false;
}
 
void CollectTmpProcessIo(int32_t pid, bool forceCollect)
{
    static std::chrono::time_point<std::chrono::system_clock> lastCallTime = std::chrono::system_clock::now();
    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastCallTime);
    const int64_t minInterval = 300;
    if (duration.count() < minInterval && !forceCollect) {
        return;
    }
    lastCallTime = now;
    ProcessIo processIO {};
    std::string content;
    std::string cmdPath = "/proc/" + std::to_string(pid) + "/cmdline";
    if (!ReadStringFromProcFile(cmdPath, content)) {
        return;
    }
    if (content.find("updater_binary") == std::string::npos && content.find("ai_binary") == std::string::npos) {
        LOG(ERROR) << pid << " is not updater_binary program";
        return;
    }
    content = "";
    std::string filename = "/proc/" + std::to_string(pid) + "/io";
    if (!ReadStringFromProcFile(filename, content) || content.empty()) {
        return;
    }
    std::vector<std::string> vec = Utils::SplitString(content, "\n");
    processIO.pid = pid;
    std::string type;
    int64_t value = 0;
    for (const std::string &str : vec) {
        if (ParseTypeAndValue(str, type, value)) {
            if (type == "rchar") {
                processIO.rchar = static_cast<uint64_t>(value);
            } else if (type == "wchar") {
                processIO.wchar = static_cast<uint64_t>(value);
            } else if (type == "syscr") {
                processIO.syscr = static_cast<uint64_t>(value);
            } else if (type == "syscw") {
                processIO.syscw = static_cast<uint64_t>(value);
            } else if (type == "read_bytes") {
                processIO.readBytes = static_cast<uint64_t>(value);
            } else if (type == "cancelled_write_bytes") {
                processIO.cancelledWriteBytes = static_cast<uint64_t>(value);
            } else if (type == "write_bytes") {
                processIO.writeBytes = static_cast<uint64_t>(value);
            }
        }
    }
    g_tmpProcessIo = processIO;
}
 
void CollectTotalProcessIo(int32_t pid)
{
    CollectTmpProcessIo(pid, true);
    g_totalProcessIo += g_tmpProcessIo;
    LOG(INFO) << "io stat is " << GetCollectTotalIo();
}
}