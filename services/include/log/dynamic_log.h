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
#ifndef UPDATE_DYNAMIC_LOG_H
#define UPDATE_DYNAMIC_LOG_H

#include "log.h"
#include "macros_updater.h"

namespace Updater {

#define LOGDYN(format, ...) Updater::Logger(Updater::GetDynamicLogLevel(), (UPDATER_LOG_FILE_NAME), (__LINE__), \
    (format), ##__VA_ARGS__)
#define LOGDYN_SEN(format, ...) Updater::LoggerSensitiveDynamic((UPDATER_LOG_FILE_NAME), (__LINE__), \
    (format), ##__VA_ARGS__)

void SetDynamicLogLevel(int level);
int GetDynamicLogLevel();

// 使用 RAII 模式管理日志级别，作用域结束时自动恢复原级别。支持静默模式，避免频繁切换时产生过多日志。
class DynamicLoggerGuard final {
    DISALLOW_COPY_MOVE(DynamicLoggerGuard);

public:
    explicit DynamicLoggerGuard(int targetLevel);
    DynamicLoggerGuard(int targetLevel, bool silent);
    ~DynamicLoggerGuard();

private:
    int targetLevel_{INFO};
    int oldLevel_{INFO};
    bool silent_{false};
};

// for sensitive information
void LoggerSensitiveDynamic(const char* fileName, int32_t line, const char* format, ...);
} // Updater

#endif /* UPDATE_DYNAMIC_LOG_H */
