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

#include "updater_hilog_helper.h"

namespace Updater {

/* Log level */
enum class UpdaterHiLogLevel {
    /* min log level */
    LOG_LEVEL_MIN = 0,
    /* Designates lower priority log. */
    LOG_DEBUG = 3,
    /* Designates useful information. */
    LOG_INFO = 4,
    /* Designates hazardous situations. */
    LOG_WARN = 5,
    /* Designates very serious errors. */
    LOG_ERROR = 6,
    /* Designates major fatal anomaly. */
    LOG_FATAL = 7,
    /* max log level */
    LOG_LEVEL_MAX,
};

int EnsureLogLevelVisible(int level)
{
    if (level == static_cast<int>(UpdaterHiLogLevel::LOG_INFO)) {
        return static_cast<int>(UpdaterHiLogLevel::LOG_WARN);
    }
    return level;
}
} // Updater
