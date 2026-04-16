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
#ifndef USCRIPT_TRANSFERLIST_H
#define USCRIPT_TRANSFERLIST_H

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include "applypatch/command_const.h"
#include "applypatch/command.h"
#include "script_instruction.h"
#include "script_manager.h"


namespace Updater {
struct WriterThreadInfo {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    BlockSet bs;
    std::unique_ptr<BlockWriter> writer;
    bool readyToWrite;
    Uscript::UScriptEnv *env;
    Hpackage::PkgManager::FileInfoPtr fileInfo;
    std::string newPatch;
};

constexpr static size_t TRANSFER_HEADERS_COUNT = 4;

struct TransferParams {
    size_t version;
    size_t blockCount;
    size_t maxEntries;
    size_t maxBlocks;
    size_t written;
    pthread_t thread;
    Uscript::UScriptEnv *env;
    std::unique_ptr<WriterThreadInfo> writerThreadInfo;
    int storeCreated;
    std::string storeBase;
    std::string freeStash;
    std::string retryFile;
    std::string devPath;
    std::string patchDatFile;
    uint8_t *dataBuffer;
    size_t dataBufferSize;
    size_t offset {0};
    bool canWrite;
    bool isUpdaterMode;
    bool noStash {false};
};

class TransferManager;
class CommandIterator;
using TransferManagerPtr = TransferManager *;
class TransferManager {
public:
    TransferManager();
    virtual ~TransferManager() {};

    bool CommandsParser(int fd, const std::vector<std::string> &context);
    bool CommandsParser(int sourceFd, int targetFd, const std::vector<std::string> &context);
    bool CommandsParser(int sourceFd, int targetFd, const std::vector<std::string> &headers,
        CommandIterator &cmdIter, bool isStream = false);
    TransferParams* GetTransferParams()
    {
        return transferParams_.get();
    }
    std::string ReloadForRetry() const;
    bool CheckResult(const CommandResult result, const std::string &cmd, const CommandType &type);

private:
    void UpdateProgress(size_t &initBlock, size_t totalSize);
    bool RegisterForRetry(const std::string &cmd);
    bool CommandsExecute(int sourceFd, int targetFd, Command &cmd);
    bool CommandParserPreCheck(CommandIterator &cmdIter);
    bool InitCommandParser(const std::vector<std::string> &headers, std::string &retryCmd);
    std::unique_ptr<TransferParams> transferParams_;
};
} // namespace Updater
#endif
