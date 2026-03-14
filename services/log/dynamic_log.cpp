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
#include "log/dynamic_log.h"

namespace Updater {
// 每个线程独立的动态日志级别，避免线程间干扰
static thread_local int g_dynamicLogLevel{ static_cast<int>(INFO) };

void SetDynamicLogLevel(int level)
{
    if (level < Updater::DEBUG || level > Updater::FATAL) {
        LOG(WARNING) << "invalid dynamic log level: " << level;
        return;
    }
    g_dynamicLogLevel = level;
}

int GetDynamicLogLevel()
{
    return g_dynamicLogLevel;
}

DynamicLoggerGuard::DynamicLoggerGuard(int targetLevel)
    : DynamicLoggerGuard(targetLevel, false)
{
}

DynamicLoggerGuard::DynamicLoggerGuard(int targetLevel, bool silent)
    : silent_(silent)
{
    if (targetLevel < Updater::DEBUG || targetLevel > Updater::FATAL) {
        LOG(WARNING) << "invalid target log level: " << targetLevel << ", use default level: " << Updater::INFO;
        targetLevel_ = Updater::INFO;
    } else {
        targetLevel_ = targetLevel;
    }

    oldLevel_ = GetDynamicLogLevel();
    SetDynamicLogLevel(targetLevel_);
    if (!silent_) {
        LOG(INFO) << "dynamic log level changed from " << oldLevel_ << " to " << GetDynamicLogLevel();
    }
}

DynamicLoggerGuard::~DynamicLoggerGuard()
{
    SetDynamicLogLevel(oldLevel_);
    if (!silent_) {
        LOG(INFO) << "dynamic log level restored from " << targetLevel_ << " to " << oldLevel_;
    }
}
} // Updater
