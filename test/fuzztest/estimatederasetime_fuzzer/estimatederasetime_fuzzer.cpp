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

#include "estimatederasetime_fuzzer.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include "log/log.h"
#include "updaterkits/updaterkits.h"
#include "securec.h"

using namespace Updater;
namespace OHOS {
    void FuzzEstimatedEraseTime(const uint8_t* data, size_t size)
    {
        std::string eraseType(reinterpret_cast<const char*>(data), size);
        EstimatedEraseTime(eraseType);
    }
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    OHOS::FuzzEstimatedEraseTime(data, size);
    return 0;
}