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
 
#ifndef IO_COLLECT_H
#define IO_COLLECT_H
 
#include <string>
#include <cstdint>
#include <iostream>
 
namespace Updater {
struct ProcessIo {
    int32_t pid;
    uint64_t rchar = 0;
    uint64_t wchar = 0;
    uint64_t syscr = 0;
    uint64_t syscw = 0;
    uint64_t readBytes = 0;
    uint64_t writeBytes = 0;
    uint64_t cancelledWriteBytes = 0;
    ProcessIo &operator += (const ProcessIo &io)
    {
        rchar += io.rchar;
        wchar += io.wchar;
        syscr += io.syscr;
        syscw += io.syscw;
        readBytes += io.readBytes;
        writeBytes += io.writeBytes;
        cancelledWriteBytes += io.cancelledWriteBytes;
        return *this;
    }
    friend std::ostream &operator<<(std::ostream &os, const ProcessIo &io)
    {
        os << io.rchar << ":" << io.wchar << ":" << io.syscr << ":" << io.syscw << ":" << io.readBytes <<
            ":" << io.writeBytes << ":" << io.cancelledWriteBytes;
        return os;
    }
};
 
std::string GetCollectTotalIo(void);
void CollectTotalProcessIo(int32_t pid);
void CollectTmpProcessIo(int32_t pid, bool forceCollect = false);
void ResetCollectTotalIo(void);
void ResetCollectTmpIo(void);
}
 
#endif