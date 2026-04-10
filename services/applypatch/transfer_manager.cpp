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
#include "applypatch/transfer_manager.h"
#include <fcntl.h>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include "applypatch/command_function.h"
#include "applypatch/command_iterator.h"
#include "log/log.h"
#include "updater/updater_const.h"
#include "utils.h"
#include "applypatch/update_progress.h"
#include "thread_pool.h"

namespace Updater {
using namespace Updater::Utils;

constexpr static size_t TRANSFER_VERSION_IDX = 0;
constexpr static size_t TRANSFER_BLOCK_COUNT_IDX = 1;
constexpr static size_t TRANSFER_MAX_ENTRIES_IDX = 2;
constexpr static size_t TRANSFER_MAX_BLOCKS_IDX = 3;

TransferManager::TransferManager()
{
    transferParams_ = std::make_unique<TransferParams>();
    transferParams_->writerThreadInfo = std::make_unique<WriterThreadInfo>();
}

bool TransferManager::CommandsExecute(int sourceFd, int targetFd, Command &cmd)
{
    cmd.SetSrcFileDescriptor(sourceFd);
    cmd.SetTargetFileDescriptor(targetFd);
    CommandFunction* cf = CommandFunctionFactory::GetInstance().GetCommandFunction(cmd.GetCommandHead());
    if (cf == nullptr) {
        LOG(ERROR) << "Failed to get cmd exec";
        return false;
    }
    CommandResult ret = cf->Execute(cmd);
    if (!cmd.GetTransferParams()->canWrite) {
        return ret == SUCCESS;
    }
    if (!CheckResult(ret, cmd.GetCommandLine(), cmd.GetCommandType())) {
        return false;
    }
    return true;
}

static bool JudgeBlockVerifyCmdType(Command &cmd)
{
    if (cmd.GetCommandType() == CommandType::NEW ||
        cmd.GetCommandType() == CommandType::ERASE ||
        cmd.GetCommandType() == CommandType::FREE ||
        cmd.GetCommandType() == CommandType::ZERO) {
            return false;
        }
    return true;
}

bool TransferManager::InitCommandParser(const std::vector<std::string> &headers, std::string &retryCmd)
{
    if (headers.size() != TRANSFER_HEADERS_COUNT) {
        LOG(ERROR) << "header size invalid " << headers.size();
        return false;
    }
    transferParams_->version = Utils::String2Int<size_t>(headers[TRANSFER_VERSION_IDX], Utils::N_DEC);
    transferParams_->blockCount = Utils::String2Int<size_t>(headers[TRANSFER_BLOCK_COUNT_IDX], Utils::N_DEC);
    transferParams_->maxEntries = Utils::String2Int<size_t>(headers[TRANSFER_MAX_ENTRIES_IDX], Utils::N_DEC);
    transferParams_->maxBlocks = Utils::String2Int<size_t>(headers[TRANSFER_MAX_BLOCKS_IDX], Utils::N_DEC);
    if (transferParams_->env != nullptr && transferParams_->env->IsRetry()) {
        retryCmd = ReloadForRetry();
    }
    return true;
}

bool TransferManager::CommandParserPreCheck(CommandIterator &cmdIter)
{
    if (!cmdIter->Check()) {
        LOG(ERROR) << "command iterator check failed";
        return false;
    }
    if (transferParams_ == nullptr) {
        LOG(ERROR) << "transferParams_ is nullptr";
        return false;
    }
    return true;
}

bool CommandsParser(int fd, const std::vector<std::string> &context)
{
    return CommandsParser(fd, fd, context);
}

bool TransferManager::CommandsParser(int sourceFd, int targetFd, const std::vector<std::string> &context)
{
    CommandIterator cmdIter(context.cbegin() + TRANSFER_HEADERS_COUNT, context.cend());
    return CommandsParser(sourceFd, targetFd, std::vector<std::string> {context.cbegin(),
        context.cbegin() + TRANSFER_HEADERS_COUNT}, cmdIter);
}

bool TransferManager::CommandsParser(int sourceFd, int targetFd, const std::vector<std::string> &headers,
    CommandIterator &cmdIter, bool isStream)
{
    std::string retryCmd = "";
    if (!CommandParserPreCheck(cmdIter) || !InitCommandParser(headers, retryCmd)) {
        return false;
    }
    size_t totalSize = transferParams_->blockCount;
    size_t initBlock = 0;
    for (cmdIter.Start(); !cmdIter.Done(); cmdIter.Next()) {
        std::unique_ptr<Command> cmd = std::make_unique<Command>(transferParams_.get());
        if (cmd == nullptr) {
            LOG(ERROR) << "Failed to parse command line.";
            return false;
        }
        cmd->SetIsStreamCmd(isStream);
        if (!cmd->Init(*cmdIter) || transferParams_->env == nullptr) {
            continue;
        }
        if (!retryCmd.empty() && transferParams_->env->IsRetry()) {
            if (*cmdIter == retryCmd) {
                LOG(INFO) << "Start Retry from Command: " << retryCmd;
                retryCmd.clear();
            }
            if (cmd->GetCommandType() != CommandType::NEW) {
                LOG(DEBUG) << "Retry: Command " << *cmdIter << " passed";
                continue;
            }
        }
        if (!transferParams_->canWrite && !JudgeBlockVerifyCmdType(*cmd)) {
            continue;
        }
        if (!CommandsExecute(sourceFd, targetFd, *cmd)) {
            LOG(ERROR) << "Running command : " << cmd->GetCommandLine() << " fail";
            return false;
        }
        if (!transferParams_->canWrite) {
            continue;
        }
        if (initBlock == 0) {
            initBlock = transferParams_->written;
        }
        if (totalSize != 0 && (transferParams_->written - initBlock) > 0) {
            UpdateProgress(initBlock, totalSize);
        }
    }
    if (fabs(Uscript::GetScriptProportion() - 1.0f) < 1e-6) {
        FillUpdateProgress();
    }
    return true;
}

void TransferManager::UpdateProgress(size_t &initBlock, size_t totalSize)
{
    float p = static_cast<float>(transferParams_->written - initBlock) / totalSize\
        * Uscript::GetScriptProportion() * Uscript::GetTotalProportion();
    SetUpdateProgress(p);
    initBlock = transferParams_->written;
}

bool TransferManager::RegisterForRetry(const std::string &cmd)
{
    std::string path = transferParams_->retryFile;
    int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        LOG(ERROR) << "Failed to create";
        return false;
    }
    bool ret = Utils::WriteStringToFile(fd, cmd);
    if (ret == false) {
        LOG(ERROR) << "Write retry flag error";
    }
    fsync(fd);
    close(fd);
    return ret;
}

std::string TransferManager::ReloadForRetry() const
{
    std::string path = transferParams_->retryFile;
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        LOG(ERROR) << "Failed to open";
        return "";
    }
    (void)lseek(fd, 0, SEEK_SET);
    std::string cmd = "";
    if (!Utils::ReadFileToString(fd, cmd)) {
        LOG(ERROR) << "Error to read retry flag";
    }
    close(fd);
    return cmd;
}

bool TransferManager::CheckResult(const CommandResult result, const std::string &cmd, const CommandType &type)
{
    switch (result) {
        case SUCCESS:
            if (type != CommandType::NEW) {
                RegisterForRetry(cmd);
            }
            break;
        case NEED_RETRY:
            LOG(INFO) << "IO failed. Running command need retry!";
            if (transferParams_->env != nullptr) {
                transferParams_->env->PostMessage("retry_update", IO_FAILED_REBOOT);
            }
            return false;
        case FAILED:
            LOG(INFO) << "Block update failed. Running command need retry!";
            if (transferParams_->env != nullptr) {
                transferParams_->env->PostMessage("retry_update", BLOCK_UPDATE_FAILED_REBOOT);
            }
            return false;
        default:
            LOG(ERROR) << "Running command failed";
            return false;
    }
    return true;
}
} // namespace Updater
