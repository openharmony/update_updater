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

#include "blockset_unittest.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <vector>
#include <openssl/sha.h>
#include "applypatch/block_set.h"
#include "applypatch/command.h"
#include "applypatch/store.h"
#include "log/log.h"
#include "securec.h"
#include "utils.h"

using namespace testing::ext;
using namespace UpdaterUt;
using namespace Updater;
using namespace std;

namespace UpdaterUt {
void BlockSetUnitTest::SetUp(void)
{
    cout << "SetUpTestCase" << endl;
}

void BlockSetUnitTest::TearDown(void)
{
    cout << "TearDownTestCase" << endl;
}

HWTEST_F(BlockSetUnitTest, blockset_test_001, TestSize.Level1)
{
    cout << "Blockset ut start";
    BlockSet block(std::vector<BlockPair> {BlockPair{0, 1}});
    cout << "Blockset ut init end";
    size_t countOfRanges = block.CountOfRanges();
    cout << "Blockset ranges: " << countOfRanges;
    auto itBegin = block.Begin();
    auto itEnd = block.End();
    auto itCBegin = block.CBegin();
    auto itCEnd = block.CEnd();
    auto itCrBegin = block.CrBegin();
    auto itCrEnd = block.CrEnd();
    if (itBegin != itEnd)
    cout << "Right iterator";
    if (itCBegin != itCEnd)
    cout << "Right iterator";
    if (itCrBegin != itCrEnd)
    cout << "Right iterator";
    std::vector<uint8_t> buffer;
    buffer.resize(H_BLOCK_SIZE);
    std::fill(buffer.begin(), buffer.end(), 0);
    string sha256 = "fdfasdf";
    std::string hexDigest;
    auto ret = block.VerifySha256(buffer, block.TotalBlockSize(), sha256, hexDigest);
    EXPECT_EQ(ret, -1);
}

HWTEST_F(BlockSetUnitTest, blockset_test_002, TestSize.Level1)
{
    cout << "Blockset ut two blocks overlap";
    BlockSet block(std::vector<BlockPair> {BlockPair{0, 1}});
    BlockSet block2(std::vector<BlockPair> {BlockPair{0, 1}});
    BlockSet block3(std::vector<BlockPair> {BlockPair{2, 3}});
    bool ret = BlockSet::IsTwoBlocksOverlap(block, block2);
    EXPECT_EQ(ret, true);
    ret = BlockSet::IsTwoBlocksOverlap(block, block3);
    EXPECT_EQ(ret, false);
}

HWTEST_F(BlockSetUnitTest, blockset_test_003, TestSize.Level1)
{
    cout << "Blockset ut two blocks overlap";
    std::vector<uint8_t> buffer;
    buffer.resize(H_BLOCK_SIZE);
    BlockSet blk(std::vector<BlockPair> {BlockPair{0, 1}});
    std::fill(buffer.begin(), buffer.end(), 0);
    std::string filename = "/tmp/ut_blockset";
    int fd = open(filename.c_str(), O_RDWR);
    if (fd < 0) {
        printf("Open file failed");
        return;
    }
    size_t ret = blk.WriteDataToBlock(fd, buffer);
    close(fd);
    EXPECT_NE(ret, 0);
}

HWTEST_F(BlockSetUnitTest, blockset_test_004, TestSize.Level1)
{
    cout << "Blockset ut two blocks overlap";
    std::vector<uint8_t> srcBuffer;
    srcBuffer.resize(H_BLOCK_SIZE);
    std::vector<uint8_t> tgtBuffer;
    tgtBuffer.resize(H_BLOCK_SIZE);
    BlockSet blk(std::vector<BlockPair> {BlockPair{0, 1}});
    std::fill(srcBuffer.begin(), srcBuffer.end(), 0);
    std::fill(tgtBuffer.begin(), tgtBuffer.end(), 0);
    BlockSet::MoveBlock(srcBuffer, blk, tgtBuffer);
    EXPECT_EQ(tgtBuffer.size(), H_BLOCK_SIZE);
}

HWTEST_F(BlockSetUnitTest, blockset_test_005, TestSize.Level1)
{
    std::string hashValue = "5aa246ebe8e817740f12cc0f6e536c5ea22e5db177563a1caea5a86614275546";
    std::string blockInfo = "2,20755,21031 276 2,20306,20582";
    std::string cmdLine = std::string("move ") + hashValue + " " + blockInfo;
    int fd = open("/data/updater/updater/blocksetTest.txt", O_CREAT | O_WRONLY, S_IRWXU | S_IRWXG | S_IRWXO);
    if (fd < 0) {
        printf("Open file failed");
        return;
    }
    std::unique_ptr<TransferParams> transferParams = std::make_unique<TransferParams>();
    transferParams->writerThreadInfo = std::make_unique<WriterThreadInfo>();
    Command *cmd = new Command(transferParams.get());
    cmd->Init(cmdLine);
    cmd->SetSrcFileDescriptor(fd);
    cmd->SetTargetFileDescriptor(fd);
    BlockSet targetBlock;
    size_t blockSize = H_BLOCK_SIZE;
    std::vector<uint8_t> srcBuffer(blockSize);
    std::vector<uint8_t> patchBuffer(blockSize);
    bool isImgDiff = true;
    int ret = targetBlock.WriteDiffToBlock(const_cast<const Command &>(*cmd),
                                           srcBuffer, patchBuffer.data(), blockSize, isImgDiff);
    EXPECT_EQ(ret, -1);
    isImgDiff = false;
    ret = targetBlock.WriteDiffToBlock(const_cast<const Command &>(*cmd),
                                       srcBuffer, patchBuffer.data(), blockSize, isImgDiff);
    EXPECT_EQ(ret, -1);
    close(fd);
    delete cmd;
}

HWTEST_F(BlockSetUnitTest, blockset_test_006, TestSize.Level1)
{
    int fd = open("/data/updater/updater/blocksetTest.txt", O_CREAT | O_WRONLY, S_IRWXU | S_IRWXG | S_IRWXO);
    if (fd < 0) {
        printf("Open file failed");
        return;
    }
    std::vector<uint8_t> buffer;
    buffer.resize(H_BLOCK_SIZE);
    BlockSet myBlock;
    size_t ret = myBlock.ReadDataFromBlock(fd, buffer);
    EXPECT_EQ(ret, 0);
    BlockSet myBlock2({std::vector<BlockPair>{}});
    BlockSet myBlock3({std::vector<BlockPair>{BlockPair{0, 1}}});
    BlockSet myBlock4({std::vector<BlockPair>{BlockPair{-1, 0}}});
    ret = myBlock3.ReadDataFromBlock(fd, buffer);
    EXPECT_EQ(ret, 0);
    ret = myBlock3.WriteDataToBlock(fd, buffer);
    EXPECT_EQ(ret, 4096);
    ret = myBlock4.ReadDataFromBlock(fd, buffer);
    EXPECT_EQ(ret, 0);
    close(fd);
}

HWTEST_F(BlockSetUnitTest, blockset_test_007, TestSize.Level1)
{
    std::vector<BlockPair> emptyPairs;
    BlockSet emptyBlock(std::move(emptyPairs));
    EXPECT_EQ(emptyBlock.CountOfRanges(), 0);
    EXPECT_EQ(emptyBlock.TotalBlockSize(), 0);
}

HWTEST_F(BlockSetUnitTest, blockset_test_008, TestSize.Level1)
{
    std::vector<BlockPair> invalidPair {BlockPair{5, 3}};
    BlockSet invalidBlock(std::move(invalidPair));
    EXPECT_EQ(invalidBlock.CountOfRanges(), 0);
}

HWTEST_F(BlockSetUnitTest, blockset_test_009, TestSize.Level1)
{
    std::vector<BlockPair> overflowPair;
    size_t maxSize = 100;
    overflowPair.push_back({maxSize - 10, maxSize});
    BlockSet overflowBlock(std::move(overflowPair));
    EXPECT_EQ(overflowBlock.CountOfRanges(), 1);
}

HWTEST_F(BlockSetUnitTest, blockset_test_010, TestSize.Level1)
{
    BlockSet block;
    bool ret = block.ParserAndInsert("");
    EXPECT_EQ(ret, false);
    ret = block.ParserAndInsert("2,0,1,1,2");
    EXPECT_EQ(ret, false);
    ret = block.ParserAndInsert("1,0,1");
    EXPECT_EQ(ret, false);
}

HWTEST_F(BlockSetUnitTest, blockset_test_011, TestSize.Level1)
{
    BlockSet block;
    std::vector<std::string> tokens1;
    bool ret = block.ParserAndInsert(tokens1);
    EXPECT_EQ(ret, false);
    std::vector<std::string> tokens2 = {"1"};
    ret = block.ParserAndInsert(tokens2);
    EXPECT_EQ(ret, false);
    std::vector<std::string> tokens3 = {"0", "0", "1"};
    ret = block.ParserAndInsert(tokens3);
    EXPECT_EQ(ret, false);
    std::vector<std::string> tokens4 = {"3", "0", "1"};
    ret = block.ParserAndInsert(tokens4);
    EXPECT_EQ(ret, false);
    std::vector<std::string> tokens5 = {"4", "0", "1", "2", "3"};
    ret = block.ParserAndInsert(tokens5);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(block.CountOfRanges(), 2);
}

HWTEST_F(BlockSetUnitTest, blockset_test_012, TestSize.Level1)
{
    BlockSet block;
    std::string blockInfo = "4,0,1,2,3";
    bool ret = block.ParserAndInsert(blockInfo);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(block.TotalBlockSize(), 2);
}

HWTEST_F(BlockSetUnitTest, blockset_test_013, TestSize.Level1)
{
    std::string filename = "/tmp/ut_blockset_seek";
    int fd = open(filename.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRWXU);
    if (fd < 0) {
        printf("Open file failed");
        return;
    }
    std::vector<uint8_t> buffer(H_BLOCK_SIZE * 2, 0xAB);
    BlockSet block(std::vector<BlockPair> {BlockPair{0, 1}, BlockPair{1, 2}});
    size_t ret = block.WriteDataToBlock(fd, buffer);
    EXPECT_EQ(ret, H_BLOCK_SIZE * 2);
    close(fd);
    fd = open(filename.c_str(), O_RDONLY);
    if (fd < 0) {
        printf("Open file failed 2");
        return;
    }
    std::vector<uint8_t> readBuffer(H_BLOCK_SIZE * 2, 0);
    ret = block.ReadDataFromBlock(fd, readBuffer);
    EXPECT_EQ(ret, H_BLOCK_SIZE * 2);
    close(fd);
}

HWTEST_F(BlockSetUnitTest, blockset_test_014, TestSize.Level1)
{
    std::string filename = "/tmp/ut_blockset_seek_err";
    int fd = open(filename.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRWXU);
    if (fd < 0) {
        printf("Open file failed");
        return;
    }
    std::vector<uint8_t> buffer(H_BLOCK_SIZE, 0xCD);
    BlockSet block;
    size_t ret = block.ReadDataFromBlock(fd, buffer);
    EXPECT_EQ(ret, 0);
    close(fd);
}

HWTEST_F(BlockSetUnitTest, blockset_test_015, TestSize.Level1)
{
    BlockSet block(std::vector<BlockPair> {BlockPair{0, 1}});
    std::vector<uint8_t> buffer(H_BLOCK_SIZE, 0);
    uint8_t digest[SHA256_DIGEST_LENGTH];
    SHA256(buffer.data(), buffer.size(), digest);
    char hashStr[SHA256_DIGEST_LENGTH * 2 + 1] = {0};
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        snprintf_s(hashStr + i * 2, 3, 3, "%02x", digest[i]);
    }
    std::string sha256Hash(hashStr);
    std::string hexDigest;
    int32_t ret = block.VerifySha256(buffer, 1, sha256Hash, hexDigest);
    EXPECT_EQ(ret, 0);
    ret = block.VerifySha256(buffer, 1, "wronghash", hexDigest);
    EXPECT_EQ(ret, -1);
}

HWTEST_F(BlockSetUnitTest, blockset_test_016, TestSize.Level1)
{
    BlockSet source(std::vector<BlockPair> {BlockPair{0, 5}});
    BlockSet target1(std::vector<BlockPair> {BlockPair{3, 8}});
    BlockSet target2(std::vector<BlockPair> {BlockPair{10, 15}});
    bool ret = BlockSet::IsTwoBlocksOverlap(source, target1);
    EXPECT_EQ(ret, true);
    ret = BlockSet::IsTwoBlocksOverlap(source, target2);
    EXPECT_EQ(ret, false);
}

HWTEST_F(BlockSetUnitTest, blockset_test_017, TestSize.Level1)
{
    std::vector<uint8_t> target(4096, 0x11);
    std::vector<uint8_t> source(4096, 0x22);
    BlockSet location(std::vector<BlockPair> {BlockPair{0, 1}});
    BlockSet::MoveBlock(target, location, source);
    EXPECT_EQ(target.size(), 4096);
}

HWTEST_F(BlockSetUnitTest, blockset_test_018, TestSize.Level1)
{
    std::string filename = "/tmp/ut_blockset_zero";
    int fd = open(filename.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRWXU);
    if (fd < 0) {
        printf("Open file failed");
        return;
    }
    BlockSet block(std::vector<BlockPair> {BlockPair{0, 1}});
    int32_t ret = block.WriteZeroToBlock(fd, false);
    EXPECT_EQ(ret, 0);
    close(fd);
}

HWTEST_F(BlockSetUnitTest, blockset_test_019, TestSize.Level1)
{
    std::string filename = "/tmp/ut_blockset_zero_erase";
    int fd = open(filename.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRWXU);
    if (fd < 0) {
        printf("Open file failed");
        return;
    }
    BlockSet block(std::vector<BlockPair> {BlockPair{0, 1}});
    int32_t ret = block.WriteZeroToBlock(fd, true);
    EXPECT_EQ(ret, 0);
    close(fd);
}

HWTEST_F(BlockSetUnitTest, blockset_test_020, TestSize.Level1)
{
    BlockSet block;
    block.ParserAndInsert("4,0,1,2,3");
    EXPECT_EQ(block.CountOfRanges(), 2);
    const BlockPair& range0 = block[0];
    const BlockPair& range1 = block[1];
    EXPECT_EQ(range0.first, 0);
    EXPECT_EQ(range0.second, 1);
    EXPECT_EQ(range1.first, 2);
    EXPECT_EQ(range1.second, 3);
}

HWTEST_F(BlockSetUnitTest, blockset_test_021, TestSize.Level1)
{
    std::string storePath = "/data/updater/ut_blockset_test";
    std::vector<uint8_t> buffer(H_BLOCK_SIZE, 0x5A);
    std::string testHash = "testhash021";
    Utils::MkdirRecursive("/data/updater/ut_blockset_test", 0755);
    int32_t ret = Store::WriteDataToStore(storePath, testHash, buffer, buffer.size());
    EXPECT_EQ(ret, 0);
    std::vector<uint8_t> loadBuffer;
    ret = Store::LoadDataFromStore(storePath, testHash, loadBuffer);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(loadBuffer.size(), H_BLOCK_SIZE);
    Store::DoFreeSpace(storePath);
}

HWTEST_F(BlockSetUnitTest, blockset_test_022, TestSize.Level1)
{
    BlockSet source(std::vector<BlockPair> {BlockPair{0, 2}, BlockPair{5, 7}});
    BlockSet target(std::vector<BlockPair> {BlockPair{1, 3}, BlockPair{8, 10}});
    bool ret = BlockSet::IsTwoBlocksOverlap(source, target);
    EXPECT_EQ(ret, true);
    ret = BlockSet::IsTwoBlocksOverlap(source, source);
    EXPECT_EQ(ret, true);
}

HWTEST_F(BlockSetUnitTest, blockset_test_023, TestSize.Level1)
{
    std::string storePath = "/data/updater/ut_blockset_test";
    Store::CreateNewSpace(storePath, true);
    std::vector<uint8_t> buffer(H_BLOCK_SIZE, 0);
    std::string hash1 = "hash1";
    int32_t ret = Store::WriteDataToStore(storePath, hash1, buffer, buffer.size());
    EXPECT_EQ(ret, 0);
    std::vector<uint8_t> loadBuf;
    ret = Store::LoadDataFromStore(storePath, hash1, loadBuf);
    EXPECT_EQ(ret, 0);
    ret = Store::FreeStore(storePath, hash1);
    EXPECT_EQ(ret, 0);
    Store::DoFreeSpace(storePath);
}

HWTEST_F(BlockSetUnitTest, blockset_test_024, TestSize.Level1)
{
    std::vector<uint8_t> largeBuffer(H_BLOCK_SIZE * 10, 0xAA);
    std::vector<uint8_t> targetBuffer(H_BLOCK_SIZE * 10, 0);
    BlockSet location(std::vector<BlockPair> {BlockPair{0, 5}, BlockPair{5, 10}});
    BlockSet::MoveBlock(targetBuffer, location, largeBuffer);
    EXPECT_EQ(targetBuffer.size(), H_BLOCK_SIZE * 10);
}

HWTEST_F(BlockSetUnitTest, blockset_test_025, TestSize.Level1)
{
    BlockSet block;
    block.ParserAndInsert("4,0,1,2,3");
    EXPECT_EQ(block.TotalBlockSize(), 2);
    EXPECT_EQ(block.CountOfRanges(), 2);
    auto iter = block.Begin();
    EXPECT_EQ(iter->first, 0);
    EXPECT_EQ(iter->second, 1);
    ++iter;
    EXPECT_EQ(iter->first, 2);
    EXPECT_EQ(iter->second, 3);
}

HWTEST_F(BlockSetUnitTest, blockset_test_026, TestSize.Level1)
{
    std::vector<uint8_t> buffer(H_BLOCK_SIZE, 0);
    uint8_t digest[SHA256_DIGEST_LENGTH];
    memset_s(digest, SHA256_DIGEST_LENGTH, 0xAB, SHA256_DIGEST_LENGTH);
    std::string hexDigest = Utils::ConvertSha256Hex(digest, SHA256_DIGEST_LENGTH);
    EXPECT_EQ(hexDigest.size(), SHA256_DIGEST_LENGTH * 2);
    std::string testDigest;
    int32_t ret = BlockSet::VerifySha256(buffer, 1, hexDigest, testDigest);
    EXPECT_EQ(ret, -1);
}

HWTEST_F(BlockSetUnitTest, blockset_test_027, TestSize.Level1)
{
    BlockSet block(std::vector<BlockPair> {BlockPair{0, 1}, BlockPair{1, 2}, BlockPair{2, 3}});
    EXPECT_EQ(block.CountOfRanges(), 3);
    EXPECT_EQ(block.TotalBlockSize(), 3);
    const BlockPair& p0 = block[0];
    const BlockPair& p1 = block[1];
    const BlockPair& p2 = block[2];
    EXPECT_EQ(p0.first, 0); EXPECT_EQ(p0.second, 1);
    EXPECT_EQ(p1.first, 1); EXPECT_EQ(p1.second, 2);
    EXPECT_EQ(p2.first, 2); EXPECT_EQ(p2.second, 3);
}

HWTEST_F(BlockSetUnitTest, blockset_test_028, TestSize.Level1)
{
    std::unique_ptr<TransferParams> transferParams = std::make_unique<TransferParams>();
    transferParams->storeBase = "/data/updater/ut_blockset_test";
    transferParams->canWrite = true;
    std::string cmdLine = "write 2,0,1,2,3";
    int fd = open("/tmp/blockset_test_028.bin", O_CREAT | O_RDWR | O_TRUNC, S_IRWXU);
    if (fd < 0) {
        return;
    }
    Command cmd(transferParams.get());
    cmd.Init(cmdLine);
    cmd.SetSrcFileDescriptor(fd);
    cmd.SetTargetFileDescriptor(fd);
    BlockSet targetBlock;
    targetBlock.ParserAndInsert("2,0,1,2,3");
    std::vector<uint8_t> buffer(H_BLOCK_SIZE * 2, 0x5A);
    size_t blockSize = 0;
    size_t pos = 1;
    std::string srcHash = "hashtest028";
    int32_t ret = targetBlock.LoadTargetBuffer(cmd, buffer, blockSize, pos, srcHash);
    EXPECT_EQ(ret, 1);
    close(fd);
}

HWTEST_F(BlockSetUnitTest, blockset_test_029, TestSize.Level1)
{
    std::unique_ptr<TransferParams> transferParams = std::make_unique<TransferParams>();
    transferParams->storeBase = "/data/updater/ut_blockset_test";
    transferParams->canWrite = false;
    std::string cmdLine = "write -";
    int fd = open("/tmp/blockset_test_029.bin", O_CREAT | O_RDWR | O_TRUNC, S_IRWXU);
    if (fd < 0) {
        return;
    }
    Command cmd(transferParams.get());
    cmd.Init(cmdLine);
    cmd.SetSrcFileDescriptor(fd);
    cmd.SetTargetFileDescriptor(fd);
    BlockSet targetBlock;
    std::vector<uint8_t> buffer(H_BLOCK_SIZE, 0);
    size_t blockSize = 0;
    size_t pos = 1;
    std::string srcHash = "";
    int32_t ret = targetBlock.LoadTargetBuffer(cmd, buffer, blockSize, pos, srcHash);
    EXPECT_EQ(ret, 1);
    close(fd);
}

HWTEST_F(BlockSetUnitTest, blockset_test_030, TestSize.Level1)
{
    std::unique_ptr<TransferParams> transferParams = std::make_unique<TransferParams>();
    transferParams->storeBase = "/data/updater/ut_blockset_test";
    transferParams->canWrite = true;
    std::string cmdLine = "write -";
    int fd = open("/tmp/blockset_test_030.bin", O_CREAT | O_RDWR | O_TRUNC, S_IRWXU);
    if (fd < 0) {
        return;
    }
    Command cmd(transferParams.get());
    cmd.Init(cmdLine);
    cmd.SetSrcFileDescriptor(fd);
    cmd.SetTargetFileDescriptor(fd);
    BlockSet targetBlock;
    targetBlock.ParserAndInsert("2,0,1,2,3");
    std::vector<uint8_t> buffer(H_BLOCK_SIZE * 2, 0);
    uint8_t digest[SHA256_DIGEST_LENGTH];
    SHA256(buffer.data(), buffer.size(), digest);
    char hashStr[SHA256_DIGEST_LENGTH * 2 + 1] = {0};
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        snprintf_s(hashStr + i * 2, 3, 3, "%02x", digest[i]);
    }
    std::string srcHash(hashStr);
    size_t blockSize = 2;
    size_t pos = 1;
    int32_t ret = targetBlock.LoadTargetBuffer(cmd, buffer, blockSize, pos, srcHash);
    EXPECT_EQ(ret, 0);
    close(fd);
}

HWTEST_F(BlockSetUnitTest, blockset_test_031, TestSize.Level1)
{
    std::unique_ptr<TransferParams> transferParams = std::make_unique<TransferParams>();
    transferParams->storeBase = "/data/updater/ut_blockset_test";
    transferParams->canWrite = true;
    std::string cmdLine = "write -";
    int fd = open("/tmp/blockset_test_031.bin", O_CREAT | O_RDWR | O_TRUNC, S_IRWXU);
    if (fd < 0) {
        return;
    }
    Command cmd(transferParams.get());
    cmd.Init(cmdLine);
    cmd.SetSrcFileDescriptor(fd);
    cmd.SetTargetFileDescriptor(fd);
    BlockSet targetBlock;
    targetBlock.ParserAndInsert("2,0,1,2,3");
    std::vector<uint8_t> buffer(H_BLOCK_SIZE * 2, 0);
    uint8_t digest[SHA256_DIGEST_LENGTH];
    SHA256(buffer.data(), buffer.size(), digest);
    char hashStr[SHA256_DIGEST_LENGTH * 2 + 1] = {0};
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        snprintf_s(hashStr + i * 2, 3, 3, "%02x", digest[i]);
    }
    std::string srcHash(hashStr);
    size_t blockSize = 2;
    size_t pos = 1;
    int32_t ret = targetBlock.LoadTargetBuffer(cmd, buffer, blockSize, pos, srcHash);
    EXPECT_EQ(ret, 0);
    close(fd);
}

HWTEST_F(BlockSetUnitTest, blockset_test_032, TestSize.Level1)
{
    std::unique_ptr<TransferParams> transferParams = std::make_unique<TransferParams>();
    transferParams->storeBase = "/data/updater/ut_blockset_test";
    transferParams->canWrite = true;
    std::string cmdLine = "write 2,0,1,2,3";
    int fd = open("/tmp/blockset_test_032.bin", O_CREAT | O_RDWR | O_TRUNC, S_IRWXU);
    if (fd < 0) {
        return;
    }
    Command cmd(transferParams.get());
    cmd.Init(cmdLine);
    cmd.SetSrcFileDescriptor(fd);
    cmd.SetTargetFileDescriptor(fd);
    BlockSet targetBlock;
    targetBlock.ParserAndInsert("2,0,1,2,3");
    std::vector<uint8_t> buffer(H_BLOCK_SIZE * 2, 0x5A);
    uint8_t digest[SHA256_DIGEST_LENGTH];
    SHA256(buffer.data(), buffer.size(), digest);
    char hashStr[SHA256_DIGEST_LENGTH * 2 + 1] = {0};
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        snprintf_s(hashStr + i * 2, 3, 3, "%02x", digest[i]);
    }
    std::string srcHash(hashStr);
    Store::WriteDataToStore(transferParams->storeBase, srcHash, buffer, buffer.size());
    size_t blockSize = 2;
    size_t pos = 1;
    int32_t ret = targetBlock.LoadTargetBuffer(cmd, buffer, blockSize, pos, srcHash);
    EXPECT_EQ(ret, 0);
    close(fd);
}

HWTEST_F(BlockSetUnitTest, blockset_test_033, TestSize.Level1)
{
    std::unique_ptr<TransferParams> transferParams = std::make_unique<TransferParams>();
    transferParams->storeBase = "/data/updater/ut_blockset_test";
    transferParams->canWrite = true;
    std::string cmdLine = "write 2,0,1,2,3";
    int fd = open("/tmp/blockset_test_033.bin", O_CREAT | O_RDWR | O_TRUNC, S_IRWXU);
    if (fd < 0) {
        return;
    }
    Command cmd(transferParams.get());
    cmd.Init(cmdLine);
    cmd.SetSrcFileDescriptor(fd);
    cmd.SetTargetFileDescriptor(fd);
    BlockSet targetBlock;
    targetBlock.ParserAndInsert("2,0,1,2,3");
    std::vector<uint8_t> buffer(H_BLOCK_SIZE * 2, 0x5A);
    size_t blockSize = 2;
    size_t pos = 1;
    std::string srcHash = "nonexistent_hash_033";
    int32_t ret = targetBlock.LoadTargetBuffer(cmd, buffer, blockSize, pos, srcHash);
    EXPECT_EQ(ret, -1);
    close(fd);
}

HWTEST_F(BlockSetUnitTest, blockset_test_034, TestSize.Level1)
{
    std::unique_ptr<TransferParams> transferParams = std::make_unique<TransferParams>();
    transferParams->storeBase = "/data/updater/ut_blockset_test";
    transferParams->canWrite = false;
    transferParams->writerThreadInfo = std::make_unique<WriterThreadInfo>();
    std::string cmdLine = "write -";
    int fd = open("/tmp/blockset_test_034.bin", O_CREAT | O_RDWR | O_TRUNC, S_IRWXU);
    if (fd < 0) {
        return;
    }
    Command cmd(transferParams.get());
    cmd.Init(cmdLine);
    cmd.SetSrcFileDescriptor(fd);
    cmd.SetTargetFileDescriptor(fd);
    BlockSet targetBlock;
    std::vector<uint8_t> buffer(H_BLOCK_SIZE, 0);
    size_t blockSize = 0;
    size_t pos = 1;
    std::string srcHash = "wrong_hash_034";
    int32_t ret = targetBlock.LoadTargetBuffer(cmd, buffer, blockSize, pos, srcHash);
    EXPECT_EQ(ret, -1);
    close(fd);
}

HWTEST_F(BlockSetUnitTest, blockset_test_035, TestSize.Level1)
{
    std::string filename = "/tmp/blockset_test_035.bin";
    int fd = open(filename.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRWXU);
    if (fd < 0) {
        return;
    }
    std::vector<uint8_t> writeBuffer(H_BLOCK_SIZE, 0xAB);
    BlockSet block(std::vector<BlockPair> {BlockPair{0, 1}});
    size_t ret = block.WriteDataToBlock(fd, writeBuffer);
    EXPECT_EQ(ret, H_BLOCK_SIZE);
    close(fd);

    fd = open(filename.c_str(), O_RDONLY);
    if (fd < 0) {
        return;
    }
    std::vector<uint8_t> readBuffer(H_BLOCK_SIZE, 0);
    ret = block.ReadDataFromBlock(fd, readBuffer);
    EXPECT_EQ(ret, H_BLOCK_SIZE);
    bool isSame = block.CompareDataFromBlock(fd, readBuffer);
    EXPECT_EQ(isSame, true);
    close(fd);

    fd = open(filename.c_str(), O_RDONLY);
    std::vector<uint8_t> wrongBuffer(H_BLOCK_SIZE, 0xCD);
    isSame = block.CompareDataFromBlock(fd, wrongBuffer);
    EXPECT_EQ(isSame, false);
    close(fd);
}

HWTEST_F(BlockSetUnitTest, blockset_test_036, TestSize.Level1)
{
    std::string filename = "/tmp/blockset_test_036.bin";
    int fd = open(filename.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRWXU);
    if (fd < 0) {
        return;
    }
    std::vector<uint8_t> buffer(H_BLOCK_SIZE, 0xAB);
    BlockSet block;
    size_t ret = block.ReadDataFromBlock(fd, buffer);
    EXPECT_EQ(ret, 0);
    close(fd);
}

HWTEST_F(BlockSetUnitTest, blockSet_test_037, TestSize.Level1)
{
    std::string filename = "/tmp/blockset_test_037.bin";
    int fd = open(filename.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRWXU);
    if (fd < 0) {
        return;
    }
    BlockSet emptyBlock;
    std::vector<uint8_t> buffer(H_BLOCK_SIZE, 0);
    size_t ret = emptyBlock.ReadDataFromBlock(fd, buffer);
    EXPECT_EQ(ret, 0);
    close(fd);
}

HWTEST_F(BlockSetUnitTest, blockset_test_038, TestSize.Level1)
{
    BlockSet source(std::vector<BlockPair> {BlockPair{0, 2}, BlockPair{4, 6}});
    BlockSet target(std::vector<BlockPair> {BlockPair{3, 5}, BlockPair{7, 9}});
    bool ret = BlockSet::IsTwoBlocksOverlap(source, target);
    EXPECT_EQ(ret, true);

    ret = BlockSet::IsTwoBlocksOverlap(source, source);
    EXPECT_EQ(ret, true);
}

HWTEST_F(BlockSetUnitTest, blockset_test_039, TestSize.Level1)
{
    std::vector<uint8_t> target(H_BLOCK_SIZE * 3, 0);
    std::vector<uint8_t> source(H_BLOCK_SIZE * 3, 0xBB);
    BlockSet location(std::vector<BlockPair> {BlockPair{0, 1}, BlockPair{1, 2}});
    BlockSet::MoveBlock(target, location, source);
    EXPECT_EQ(target.size(), H_BLOCK_SIZE * 3);
}

HWTEST_F(BlockSetUnitTest, blockset_test_040, TestSize.Level1)
{
    std::vector<uint8_t> target(H_BLOCK_SIZE * 20, 0);
    std::vector<uint8_t> source(H_BLOCK_SIZE * 20, 0xCC);
    BlockSet location(std::vector<BlockPair> {BlockPair{0, 10}, BlockPair{10, 20}});
    BlockSet::MoveBlock(target, location, source);
    EXPECT_EQ(target.size(), H_BLOCK_SIZE * 20);
}

HWTEST_F(BlockSetUnitTest, blockset_test_041, TestSize.Level1)
{
    std::string filename = "/tmp/blockset_test_041.bin";
    int fd = open(filename.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRWXU);
    if (fd < 0) {
        return;
    }
    BlockSet block(std::vector<BlockPair> {BlockPair{0, 2}});
    std::vector<uint8_t> buffer(H_BLOCK_SIZE * 2, 0xDD);
    int32_t ret = block.WriteZeroToBlock(fd, false);
    EXPECT_EQ(ret, 0);
    close(fd);

    fd = open(filename.c_str(), O_RDONLY);
    std::vector<uint8_t> readBuffer(H_BLOCK_SIZE * 2, 0);
    size_t readSize = block.ReadDataFromBlock(fd, readBuffer);
    EXPECT_EQ(readSize, H_BLOCK_SIZE * 2);
    bool isZero = true;
    for (size_t i = 0; i < readBuffer.size(); i++) {
        if (readBuffer[i] != 0) {
            isZero = false;
            break;
        }
    }
    EXPECT_EQ(isZero, true);
    close(fd);
}

HWTEST_F(BlockSetUnitTest, blockset_test_042, TestSize.Level1)
{
    std::unique_ptr<TransferParams> transferParams = std::make_unique<TransferParams>();
    transferParams->storeBase = "/data/updater/ut_blockset_test";
    transferParams->canWrite = true;
    std::string cmdLine = "write 2,0,1,1,2";
    int fd = open("/tmp/blockset_test_042.bin", O_CREAT | O_RDWR | O_TRUNC, S_IRWXU);
    if (fd < 0) {
        return;
    }
    Command cmd(transferParams.get());
    cmd.Init(cmdLine);
    cmd.SetSrcFileDescriptor(fd);
    cmd.SetTargetFileDescriptor(fd);
    BlockSet targetBlock;
    targetBlock.ParserAndInsert("2,0,1,1,2");
    std::vector<uint8_t> srcBuffer(H_BLOCK_SIZE * 2, 0x11);
    std::vector<uint8_t> patchBuffer(H_BLOCK_SIZE, 0);
    int32_t ret = targetBlock.WriteDiffToBlock(cmd, srcBuffer, patchBuffer.data(), patchBuffer.size(), false);
    EXPECT_EQ(ret, -1);
    close(fd);
}

HWTEST_F(BlockSetUnitTest, blockset_test_043, TestSize.Level1)
{
    std::unique_ptr<TransferParams> transferParams = std::make_unique<TransferParams>();
    transferParams->storeBase = "/data/updater/ut_blockset_test";
    transferParams->canWrite = true;
    std::string cmdLine = "write 2,0,1,1,2";
    int fd = open("/tmp/blockset_test_043.bin", O_CREAT | O_RDWR | O_TRUNC, S_IRWXU);
    if (fd < 0) {
        return;
    }
    Command cmd(transferParams.get());
    cmd.Init(cmdLine);
    cmd.SetSrcFileDescriptor(fd);
    cmd.SetTargetFileDescriptor(fd);
    BlockSet targetBlock;
    targetBlock.ParserAndInsert("2,0,1,1,2");
    std::vector<uint8_t> srcBuffer(H_BLOCK_SIZE * 2, 0x22);
    std::vector<uint8_t> patchBuffer(H_BLOCK_SIZE, 0);
    int32_t ret = targetBlock.WriteDiffToBlock(cmd, srcBuffer, patchBuffer.data(), patchBuffer.size(), true);
    EXPECT_EQ(ret, -1);
    close(fd);
}

HWTEST_F(BlockSetUnitTest, blockset_test_044, TestSize.Level1)
{
    std::string filename = "/tmp/blockset_test_044.bin";
    int fd = open(filename.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRWXU);
    if (fd < 0) {
        return;
    }
    std::vector<uint8_t> buffer(H_BLOCK_SIZE * 2, 0xAB);
    BlockSet block(std::vector<BlockPair> {BlockPair{0, 1}, BlockPair{1, 2}});
    size_t ret = block.WriteDataToBlock(fd, buffer);
    EXPECT_EQ(ret, H_BLOCK_SIZE * 2);
    close(fd);

    fd = open(filename.c_str(), O_RDONLY);
    std::vector<uint8_t> mismatchBuffer(H_BLOCK_SIZE, 0);
    bool cmpResult = block.CompareDataFromBlock(fd, mismatchBuffer);
    EXPECT_EQ(cmpResult, false);
    close(fd);
}

HWTEST_F(BlockSetUnitTest, blockset_test_045, TestSize.Level1)
{
    std::string filename = "/tmp/blockset_test_045.bin";
    int fd = open(filename.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRWXU);
    if (fd < 0) {
        return;
    }
    std::vector<uint8_t> buffer(H_BLOCK_SIZE * 2, 0);
    BlockSet block(std::vector<BlockPair> {BlockPair{0, 1}, BlockPair{1, 2}});
    size_t ret = block.WriteDataToBlock(fd, buffer);
    close(fd);

    fd = open(filename.c_str(), O_RDONLY);
    block.ParserAndInsert("4,0,1,2,3");
    ret = block.ReadDataFromBlock(fd, buffer);
    close(fd);
    EXPECT_EQ(ret, H_BLOCK_SIZE * 2);
}

HWTEST_F(BlockSetUnitTest, blockset_test_046, TestSize.Level1)
{
    std::unique_ptr<TransferParams> transferParams = std::make_unique<TransferParams>();
    transferParams->storeBase = "/data/updater/ut_blockset_test";
    transferParams->canWrite = true;
    std::string cmdLine = "move - 2,0,1,1,2";
    int fd = open("/tmp/blockset_test_046.bin", O_CREAT | O_RDWR | O_TRUNC, S_IRWXU);
    if (fd < 0) {
        return;
    }
    Command cmd(transferParams.get());
    cmd.Init(cmdLine);
    cmd.SetSrcFileDescriptor(fd);
    cmd.SetTargetFileDescriptor(fd);
    BlockSet targetBlock;
    std::vector<uint8_t> buffer(H_BLOCK_SIZE * 2, 0);
    size_t blockSize = 0;
    size_t pos = 1;
    std::string srcHash = "testhash046";
    int32_t ret = targetBlock.LoadTargetBuffer(cmd, buffer, blockSize, pos, srcHash);
    EXPECT_EQ(ret, 1);
    close(fd);
}

HWTEST_F(BlockSetUnitTest, blockset_test_047, TestSize.Level1)
{
    std::unique_ptr<TransferParams> transferParams = std::make_unique<TransferParams>();
    transferParams->storeBase = "/data/updater/ut_blockset_test";
    transferParams->canWrite = true;
    transferParams->writerThreadInfo = std::make_unique<WriterThreadInfo>();
    std::string cmdLine = "move - 2,0,1-2,0,1,1,2";
    int fd = open("/tmp/blockset_test_047.bin", O_CREAT | O_RDWR | O_TRUNC, S_IRWXU);
    if (fd < 0) {
        return;
    }
    Command cmd(transferParams.get());
    cmd.Init(cmdLine);
    cmd.SetSrcFileDescriptor(fd);
    cmd.SetTargetFileDescriptor(fd);
    BlockSet targetBlock;
    std::vector<uint8_t> buffer(H_BLOCK_SIZE * 2, 0);
    size_t blockSize = 0;
    size_t pos = 1;
    std::string srcHash = "testhash047";
    int32_t ret = targetBlock.LoadTargetBuffer(cmd, buffer, blockSize, pos, srcHash);
    EXPECT_EQ(ret, 1);
    close(fd);
}

HWTEST_F(BlockSetUnitTest, blockset_test_048, TestSize.Level1)
{
    std::unique_ptr<TransferParams> transferParams = std::make_unique<TransferParams>();
    transferParams->storeBase = "/data/updater/ut_blockset_test";
    transferParams->canWrite = true;
    std::string cmdLine = "write 2,0,1,2,3";
    int fd = open("/tmp/blockset_test_048.bin", O_CREAT | O_RDWR | O_TRUNC, S_IRWXU);
    if (fd < 0) {
        return;
    }
    Command cmd(transferParams.get());
    cmd.Init(cmdLine);
    cmd.SetSrcFileDescriptor(fd);
    cmd.SetTargetFileDescriptor(fd);
    BlockSet targetBlock;
    targetBlock.ParserAndInsert("2,0,1,2,3");
    std::vector<uint8_t> buffer(H_BLOCK_SIZE * 2, 0x5A);
    uint8_t digest[SHA256_DIGEST_LENGTH];
    SHA256(buffer.data(), buffer.size(), digest);
    char hashStr[SHA256_DIGEST_LENGTH * 2 + 1] = {0};
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        snprintf_s(hashStr + i * 2, 3, 3, "%02x", digest[i]);
    }
    std::string srcHash(hashStr);
    Store::WriteDataToStore(transferParams->storeBase, srcHash, buffer, buffer.size());
    size_t blockSize = 2;
    size_t pos = 1;
    int32_t ret = targetBlock.LoadTargetBuffer(cmd, buffer, blockSize, pos, srcHash);
    EXPECT_EQ(ret, 0);
    close(fd);
}

HWTEST_F(BlockSetUnitTest, blockset_test_049, TestSize.Level1)
{
    std::unique_ptr<TransferParams> transferParams = std::make_unique<TransferParams>();
    transferParams->storeBase = "/data/updater/ut_blockset_test";
    transferParams->canWrite = true;
    std::string cmdLine = "write -";
    int fd = open("/tmp/blockset_test_049.bin", O_CREAT | O_RDWR | O_TRUNC, S_IRWXU);
    if (fd < 0) {
        return;
    }
    Command cmd(transferParams.get());
    cmd.Init(cmdLine);
    cmd.SetSrcFileDescriptor(fd);
    cmd.SetTargetFileDescriptor(fd);
    BlockSet targetBlock;
    std::vector<uint8_t> buffer(H_BLOCK_SIZE, 0);
    size_t blockSize = 0;
    size_t pos = 1;
    std::string srcHash = "testhash049";
    int32_t ret = targetBlock.LoadTargetBuffer(cmd, buffer, blockSize, pos, srcHash);
    EXPECT_EQ(ret, 1);
    close(fd);
}

HWTEST_F(BlockSetUnitTest, blockset_test_050, TestSize.Level1)
{
    std::string filename = "/tmp/blockset_test_050.bin";
    int fd = open(filename.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRWXU);
    if (fd < 0) {
        return;
    }
    std::vector<uint8_t> buffer(H_BLOCK_SIZE * 3, 0xEE);
    BlockSet block(std::vector<BlockPair> {BlockPair{0, 1}, BlockPair{1, 2}, BlockPair{2, 3}});
    size_t ret = block.WriteDataToBlock(fd, buffer);
    EXPECT_EQ(ret, H_BLOCK_SIZE * 3);
    close(fd);

    fd = open(filename.c_str(), O_RDONLY);
    std::vector<uint8_t> readBuffer(H_BLOCK_SIZE * 3, 0);
    ret = block.ReadDataFromBlock(fd, readBuffer);
    EXPECT_EQ(ret, H_BLOCK_SIZE * 3);
    bool isSame = block.CompareDataFromBlock(fd, readBuffer);
    EXPECT_EQ(isSame, true);
    close(fd);
}
}
