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
#ifndef UPDATE_UPDATER_HILOG_HELPER_H
#define UPDATE_UPDATER_HILOG_HELPER_H

namespace Updater {

/**
 * Returns the converted log level. LOG_INFO will be converted to LOG_WARN to ensure
 * it can be printed in both log and nolog versions. Other levels remain unchanged.
 */
template <typename T>
constexpr T EnsureLogLevelVisible(const T level)
{
    if (level == T::LOG_INFO) {
        return T::LOG_WARN;
    }
    return level;
}
} // Updater

#endif /* UPDATE_UPDATER_HILOG_HELPER_H */
