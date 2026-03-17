/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include "log/log.h"
#include <atomic>
#include <chrono>
#include <cstdarg>
#include <memory>
#include <unordered_map>
#include <vector>
#include <sys/time.h>
#include <unistd.h>
#include <mutex>
#ifndef DIFF_PATCH_SDK
#include <log_base.h>
#endif
#include "securec.h"

namespace Updater {
static std::mutex& GetUpdaterLogLock()
{
    static std::mutex updaterLogLock;
    return updaterLogLock;
}

static std::ofstream g_updaterLog;
static std::ofstream g_updaterStage;
static std::ofstream g_errorCode;
static std::ofstream g_nullStream;
static std::string g_logTag;
static ReplaceLogType g_replaceMap;
static std::atomic<int> g_logLevel{ static_cast<int>(INFO) };
#ifndef DIFF_PATCH_SDK
static constexpr unsigned int UPDATER_DOMAIN = 0XD002E01;
#endif

namespace {
void OpenLogStream(std::ofstream &ofs, const std::string &file)
{
    if (ofs.is_open()) {
        ofs.close(); // close file
        ofs.clear(); // clear fail bit
    }
    ofs.open(file.c_str(), std::ios::app | std::ios::out);
}

std::string ExtractFileName(const std::string &fullPath)
{
    const std::string fileSeparator = "/\\";
    const auto pos = fullPath.find_last_of(fileSeparator);
    return (pos == std::string::npos) ? fullPath : fullPath.substr(pos + 1);
}
}

void SetReplaceMap(const ReplaceLogType &replaceMap)
{
    std::lock_guard<std::mutex> lock(GetUpdaterLogLock());
    g_replaceMap = replaceMap;
}

void ReplaceLog(std::string &str)
{
    if (str.empty() || g_replaceMap.empty()) {
        return;
    }
    for (const auto& [key, value] : g_replaceMap) {
        size_t pos = 0;
        if ((pos = str.find(key, pos)) != std::string::npos) {
            str.replace(pos, key.length(), value);
        }
    }
}

void InitUpdaterLogger(const std::string &tag, const std::string &logFile, const std::string &stageFile,
    const std::string &errorCodeFile)
{
    {
        std::lock_guard<std::mutex> lock(GetUpdaterLogLock());
        g_logTag = tag;
        OpenLogStream(g_updaterLog, logFile);
        OpenLogStream(g_updaterStage, stageFile);
        OpenLogStream(g_errorCode, errorCodeFile);
    }
    LOG(INFO) << "updater logger init success. tag: " << tag << ", logFile: " << ExtractFileName(logFile) <<
        ", stageFile: " << ExtractFileName(stageFile) << ", errorCodeFile: " << ExtractFileName(errorCodeFile);
}

UpdaterLogger::~UpdaterLogger()
{
    std::string str = oss_.str();
    if (g_logLevel > level_ || replaceFunc_ == nullptr) {
        return;
    }
    pid_t tid = 0;
    int cpu_id = 0;
#ifndef DIFF_PATCH_SDK
    {
        std::lock_guard<std::mutex> lock(GetUpdaterLogLock());
        HiLogBasePrint(LOG_CORE, (LogLevel)level_, UPDATER_DOMAIN, g_logTag.c_str(), "%{public}s", str.c_str());
    }
    tid = gettid();
    cpu_id = sched_getcpu();
#endif
    oss_.str("");
    oss_ << std::endl << std::flush;
    std::lock_guard<std::mutex> lock(GetUpdaterLogLock());
    if (g_updaterLog.is_open()) {
        replaceFunc_(str);
        std::string cpu_id_tag = "[" + std::to_string(cpu_id) + "]";
        g_updaterLog << realTime_ <<  " " << g_logTag << cpu_id_tag <<  tid
            << " " << logLevelMap_[level_] << " " << str << std::endl << std::flush;
    }
}

StageLogger::~StageLogger()
{
    std::lock_guard<std::mutex> lock(GetUpdaterLogLock());
    if (g_updaterStage.is_open()) {
        g_updaterStage << std::endl << std::flush;
    } else {
        std::cout << std::endl << std::flush;
    }
}

void SetLogLevel(int level)
{
    g_logLevel = level;
}

void GetFormatTime(char time[], int size)
{
#ifndef DIFF_PATCH_SDK
    struct timeval tv {};
    struct tm tm {};

    gettimeofday(&tv, nullptr);
    localtime_r(&tv.tv_sec, &tm);
    snprintf_s(time, size, size - 1, "%02d-%02d %02d:%02d:%02d.%03d",
        tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
        static_cast<int>(tv.tv_usec / 1000)); // need div 1000
#endif
}

std::ostream& UpdaterLogger::OutputUpdaterLog(const std::string &path, int line)
{
    GetFormatTime(realTime_, sizeof(realTime_));
    if (g_logLevel <= level_) {
        return oss_ << path << " " << line << " : ";
    }
    return g_nullStream;
}

std::ostream& UpdaterLoggerLite::OutputUpdaterLog()
{
    GetFormatTime(realTime_, sizeof(realTime_));
    if (g_logLevel <= level_) {
        return oss_;
    }
    return g_nullStream;
}

std::ostream& StageLogger::OutputUpdaterStage()
{
    std::unordered_map<int, std::string> updaterStageMap = {
        { UPDATE_STAGE_BEGIN, "BEGIN" },
        { UPDATE_STAGE_SUCCESS, "SUCCESS" },
        { UPDATE_STAGE_FAIL, "FAIL" },
        { UPDATE_STAGE_OUT, "OUT" }
    };
    char realTime[MAX_TIME_SIZE] = {0};
    GetFormatTime(realTime, sizeof(realTime));

    std::lock_guard<std::mutex> lock(GetUpdaterLogLock());
    if (g_updaterLog.is_open()) {
        if (stage_ == UPDATE_STAGE_OUT) {
            return g_updaterStage << realTime << "  " << g_logTag << " ";
        }
        return g_updaterStage << realTime << "  " << g_logTag << " status is : " <<
            updaterStageMap[stage_] << ", stage is ";
    }
    return std::cout;
}

void Logger(int level, const char* fileName, int32_t line, const char* format, ...)
{
    // 1024 : max length of buff
    char buff[1024] = {0};
    va_list list;
    va_start(list, format);
    int size = vsnprintf_s(buff, sizeof(buff), sizeof(buff) - 1, format, list);
    va_end(list);
    if (size < EOK) {
        UpdaterLogger(level).OutputUpdaterLog(fileName, line) << "vsnprintf_s failed";
        return;
    }
    UpdaterLogger(level).OutputUpdaterLog(fileName, line) << buff;
}

std::ostream& ErrorCode::OutputErrorCode(const std::string &path, int line, UpdaterErrorCode code)
{
    char realTime[MAX_TIME_SIZE] = {0};
    GetFormatTime(realTime, sizeof(realTime));
    std::lock_guard<std::mutex> lock(GetUpdaterLogLock());
    if (g_errorCode.is_open()) {
        return g_errorCode << realTime <<  "  " << path << " " << line << " , error code is : " << code << std::endl;
    }
    return std::cout;
}
} // Updater
