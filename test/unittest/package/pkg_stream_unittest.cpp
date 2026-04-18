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
 
#include <cstring>
#include <fcntl.h>
#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "log.h"
#include "pkg_manager.h"
#include "pkg_stream.h"
#include "pkg_test.h"
#include "ring_buffer/ring_buffer.h"
 
using namespace std;
using namespace Hpackage;
using namespace Updater;
using namespace testing::ext;
 
namespace UpdaterUt {
 
class TestablePkgStreamImpl : public PkgStreamImpl {
public:
    explicit TestablePkgStreamImpl(PkgManager::PkgManagerPtr pkgManager, const std::string fileName)
        : PkgStreamImpl(pkgManager, fileName) {}
    virtual ~TestablePkgStreamImpl() {}
 
    int32_t Seek(long int offset, int whence) override
    {
        UNUSED(offset);
        UNUSED(whence);
        return PKG_SUCCESS;
    }
 
    size_t GetFileLength() override
    {
        return 0;
    }
};
 
class PkgStreamImplTest : public PkgTest {
public:
    PkgStreamImplTest() {}
    ~PkgStreamImplTest() override {}
 
protected:
    void SetUp() override
    {
        PkgTest::SetUp();
    }
    void TearDown() override
    {
        PkgTest::TearDown();
    }
};
 
class FileStreamTest : public PkgTest {
public:
    FileStreamTest() {}
    ~FileStreamTest() override {}
 
protected:
    void SetUp() override
    {
        PkgTest::SetUp();
        static const mode_t permissionDir = 0755;
        mkdir("/data/updater", permissionDir);
        testFilePath_ = "/data/updater/test_pkg_stream.bin";
        FILE *fp = fopen(testFilePath_.c_str(), "wb");
        if (fp != nullptr) {
            const char *testData = "Test data for FileStream";
            fwrite(testData, 1, strlen(testData), fp);
            fclose(fp);
        }
    }
    void TearDown() override
    {
        if (access(testFilePath_.c_str(), F_OK) == 0) {
            unlink(testFilePath_.c_str());
        }
        PkgTest::TearDown();
    }
 
    std::string testFilePath_;
};
 
class MemoryMapStreamTest : public PkgTest {
public:
    MemoryMapStreamTest() {}
    ~MemoryMapStreamTest() override {}
 
protected:
    void SetUp() override
    {
        PkgTest::SetUp();
        constexpr size_t testMemSize = 4096;
        memBuffer_.buffer = new uint8_t[testMemSize]{};
        memBuffer_.length = testMemSize;
    }
    void TearDown() override
    {
        PkgTest::TearDown();
    }
 
    PkgBuffer memBuffer_;
};
 
class FlowDataStreamTest : public PkgTest {
public:
    FlowDataStreamTest() {}
    ~FlowDataStreamTest() override {}
 
protected:
    void SetUp() override
    {
        PkgTest::SetUp();
        ringBuf_ = new Updater::RingBuffer();
        if (ringBuf_ != nullptr) {
            const int bufferSize = 1024;
            const int channelCount = 4;
            (void)ringBuf_->Init(bufferSize, channelCount);
        }
    }
    void TearDown() override
    {
        if (ringBuf_ != nullptr) {
            delete ringBuf_;
            ringBuf_ = nullptr;
        }
        PkgTest::TearDown();
    }
 
    Updater::RingBuffer *ringBuf_ = nullptr;
};
 
HWTEST_F(PkgStreamImplTest, PkgStreamImplGetFileName, TestSize.Level1)
{
    TestablePkgStreamImpl stream(nullptr, "test_file.bin");
    std::string fileName = stream.GetFileName();
    EXPECT_EQ(fileName, "test_file.bin");
}
 
HWTEST_F(PkgStreamImplTest, PkgStreamImplAddRef, TestSize.Level1)
{
    TestablePkgStreamImpl stream(nullptr, "test_file.bin");
    EXPECT_TRUE(stream.IsRef());
    stream.AddRef();
    EXPECT_FALSE(stream.IsRef());
}
 
HWTEST_F(PkgStreamImplTest, PkgStreamImplDelRef, TestSize.Level1)
{
    TestablePkgStreamImpl stream(nullptr, "test_file.bin");
    stream.AddRef();
    stream.AddRef();
    EXPECT_FALSE(stream.IsRef());
    stream.DelRef();
    EXPECT_FALSE(stream.IsRef());
    stream.DelRef();
    EXPECT_TRUE(stream.IsRef());
}
 
HWTEST_F(PkgStreamImplTest, PkgStreamImplConvertPkgStream, TestSize.Level1)
{
    PkgManager::StreamPtr streamPtr = nullptr;
    PkgStreamPtr result = PkgStreamImpl::ConvertPkgStream(streamPtr);
    EXPECT_EQ(result, nullptr);
}
 
HWTEST_F(PkgStreamImplTest, PkgStreamImplGetStreamType, TestSize.Level1)
{
    TestablePkgStreamImpl stream(nullptr, "test_file.bin");
    EXPECT_EQ(stream.GetStreamType(), PkgStream::PkgStreamType_Read);
}
 
HWTEST_F(FileStreamTest, FileStreamDestructorWithNullStream, TestSize.Level1)
{
    mkdir("/data/updater", 0755);
    FILE *fp = fopen("/data/updater/test_null_stream.bin", "wb");
    if (fp != nullptr) {
        fwrite("test", 1, 4, fp);
        fclose(fp);
    }
    {
        FILE *stream = fopen("/data/updater/test_null_stream.bin", "rb");
        FileStream fs(nullptr, "test.bin", stream, PkgStream::PkgStreamType_Read);
    }
    unlink("/data/updater/test_null_stream.bin");
}
 
HWTEST_F(FileStreamTest, FileStreamReadWithNullStream, TestSize.Level1)
{
    FileStream fs(nullptr, "test.bin", nullptr, PkgStream::PkgStreamType_Read);
    PkgBuffer data(100);
    size_t readLen = 0;
    int32_t ret = fs.Read(data, 0, 100, readLen);
    EXPECT_EQ(ret, PKG_INVALID_STREAM);
}
 
HWTEST_F(FileStreamTest, FileStreamReadWithInsufficientBuffer, TestSize.Level1)
{
    FILE *fp = fopen(testFilePath_.c_str(), "rb");
    ASSERT_TRUE(fp != nullptr);
    FileStream fs(nullptr, testFilePath_.c_str(), fp, PkgStream::PkgStreamType_Read);
    PkgBuffer data(10);
    size_t readLen = 0;
    int32_t ret = fs.Read(data, 0, 100, readLen);
    EXPECT_EQ(ret, PKG_INVALID_STREAM);
    fclose(fp);
}
 
HWTEST_F(FileStreamTest, FileStreamReadSuccess, TestSize.Level1)
{
    FILE *fp = fopen(testFilePath_.c_str(), "rb");
    ASSERT_TRUE(fp != nullptr);
    FileStream fs(nullptr, testFilePath_.c_str(), fp, PkgStream::PkgStreamType_Read);
    PkgBuffer data(100);
    size_t readLen = 0;
    int32_t ret = fs.Read(data, 0, 20, readLen);
    EXPECT_EQ(ret, PKG_SUCCESS);
    EXPECT_GT(readLen, 0);
    fclose(fp);
}
 
HWTEST_F(FileStreamTest, FileStreamReadWithInvalidStart, TestSize.Level1)
{
    FILE *fp = fopen(testFilePath_.c_str(), "rb");
    ASSERT_TRUE(fp != nullptr);
    FileStream fs(nullptr, testFilePath_.c_str(), fp, PkgStream::PkgStreamType_Read);
    PkgBuffer data(100);
    size_t readLen = 0;
    int32_t ret = fs.Read(data, 1000, 20, readLen);
    EXPECT_EQ(ret, PKG_INVALID_STREAM);
    fclose(fp);
}
 
HWTEST_F(FileStreamTest, FileStreamWriteWithInvalidStreamType, TestSize.Level1)
{
    FILE *fp = fopen(testFilePath_.c_str(), "rb");
    ASSERT_TRUE(fp != nullptr);
    FileStream fs(nullptr, testFilePath_.c_str(), fp, PkgStream::PkgStreamType_Read);
    const char *writeData = "write test";
    PkgBuffer data(reinterpret_cast<uint8_t*>(const_cast<char*>(writeData)), strlen(writeData));
    int32_t ret = fs.Write(data, strlen(writeData), 0);
    EXPECT_EQ(ret, PKG_INVALID_STREAM);
    fclose(fp);
}
 
HWTEST_F(FileStreamTest, FileStreamWriteSuccess, TestSize.Level1)
{
    std::string writeFilePath = "/data/updater/test_write_stream.bin";
    FILE *fp = fopen(writeFilePath.c_str(), "wb");
    ASSERT_TRUE(fp != nullptr);
    FileStream fs(nullptr, writeFilePath.c_str(), fp, PkgStream::PkgStreamType_Write);
    const char *writeData = "write test data";
    PkgBuffer data(reinterpret_cast<uint8_t*>(const_cast<char*>(writeData)), strlen(writeData));
    int32_t ret = fs.Write(data, strlen(writeData), 0);
    EXPECT_EQ(ret, PKG_SUCCESS);
    fclose(fp);
    unlink(writeFilePath.c_str());
}
 
HWTEST_F(FileStreamTest, FileStreamGetFileLengthWithNullStream, TestSize.Level1)
{
    FileStream fs(nullptr, "test.bin", nullptr, PkgStream::PkgStreamType_Read);
    size_t len = fs.GetFileLength();
    EXPECT_EQ(len, 0);
}
 
HWTEST_F(FileStreamTest, FileStreamGetFileLengthSuccess, TestSize.Level1)
{
    FILE *fp = fopen(testFilePath_.c_str(), "rb");
    ASSERT_TRUE(fp != nullptr);
    FileStream fs(nullptr, testFilePath_.c_str(), fp, PkgStream::PkgStreamType_Read);
    size_t len = fs.GetFileLength();
    EXPECT_GT(len, 0);
    fclose(fp);
}
 
HWTEST_F(FileStreamTest, FileStreamSeekWithNullStream, TestSize.Level1)
{
    FileStream fs(nullptr, "test.bin", nullptr, PkgStream::PkgStreamType_Read);
    int32_t ret = fs.Seek(0, SEEK_SET);
    EXPECT_EQ(ret, PKG_INVALID_STREAM);
}
 
HWTEST_F(FileStreamTest, FileStreamSeekSuccess, TestSize.Level1)
{
    FILE *fp = fopen(testFilePath_.c_str(), "rb");
    ASSERT_TRUE(fp != nullptr);
    FileStream fs(nullptr, testFilePath_.c_str(), fp, PkgStream::PkgStreamType_Read);
    int32_t ret = fs.Seek(5, SEEK_SET);
    EXPECT_EQ(ret, 0);
    fclose(fp);
}
 
HWTEST_F(FileStreamTest, FileStreamFlushWithNullStream, TestSize.Level1)
{
    FileStream fs(nullptr, "test.bin", nullptr, PkgStream::PkgStreamType_Read);
    int32_t ret = fs.Flush(100);
    EXPECT_EQ(ret, PKG_INVALID_STREAM);
}
 
HWTEST_F(FileStreamTest, FileStreamFlushSuccess, TestSize.Level1)
{
    FILE *fp = fopen(testFilePath_.c_str(), "rb+");
    ASSERT_TRUE(fp != nullptr);
    FileStream fs(nullptr, testFilePath_.c_str(), fp, PkgStream::PkgStreamType_Write);
    int32_t ret = fs.Flush(20);
    EXPECT_EQ(ret, PKG_SUCCESS);
    fclose(fp);
}
 
HWTEST_F(MemoryMapStreamTest, MemoryMapStreamDestructorWithMemoryMap, TestSize.Level1)
{
    {
        MemoryMapStream mms(nullptr, "test.bin", memBuffer_, PkgStream::PkgStreamType_MemoryMap);
    }
}
 
HWTEST_F(MemoryMapStreamTest, MemoryMapStreamReadWithNullMemMap, TestSize.Level1)
{
    PkgBuffer nullBuffer;
    nullBuffer.buffer = nullptr;
    nullBuffer.length = 0;
    MemoryMapStream mms(nullptr, "test.bin", nullBuffer, PkgStream::PkgStreamType_MemoryMap);
    PkgBuffer data(100);
    size_t readLen = 0;
    int32_t ret = mms.Read(data, 0, 100, readLen);
    EXPECT_EQ(ret, PKG_INVALID_STREAM);
}
 
HWTEST_F(MemoryMapStreamTest, MemoryMapStreamReadWithInvalidStart, TestSize.Level1)
{
    MemoryMapStream mms(nullptr, "test.bin", memBuffer_, PkgStream::PkgStreamType_MemoryMap);
    PkgBuffer data(100);
    size_t readLen = 0;
    int32_t ret = mms.Read(data, 5000, 100, readLen);
    EXPECT_EQ(ret, PKG_INVALID_STREAM);
}
 
HWTEST_F(MemoryMapStreamTest, MemoryMapStreamReadWithInsufficientBuffer, TestSize.Level1)
{
    MemoryMapStream mms(nullptr, "test.bin", memBuffer_, PkgStream::PkgStreamType_MemoryMap);
    PkgBuffer data(10);
    size_t readLen = 0;
    int32_t ret = mms.Read(data, 0, 100, readLen);
    EXPECT_EQ(ret, PKG_INVALID_STREAM);
}
 
HWTEST_F(MemoryMapStreamTest, MemoryMapStreamReadSuccess, TestSize.Level1)
{
    memcpy_s(memBuffer_.buffer, sizeof(memBuffer_.buffer), "Test data for memory map", 25);
    MemoryMapStream mms(nullptr, "test.bin", memBuffer_, PkgStream::PkgStreamType_MemoryMap);
    PkgBuffer data(100);
    data.data.resize(100);
    size_t readLen = 0;
    int32_t ret = mms.Read(data, 0, 20, readLen);
    EXPECT_EQ(ret, PKG_SUCCESS);
    EXPECT_EQ(readLen, 20);
}
 
HWTEST_F(MemoryMapStreamTest, MemoryMapStreamWriteWithNullMemMap, TestSize.Level1)
{
    PkgBuffer nullBuffer;
    nullBuffer.buffer = nullptr;
    nullBuffer.length = 0;
    MemoryMapStream mms(nullptr, "test.bin", nullBuffer, PkgStream::PkgStreamType_MemoryMap);
    const char *writeData = "write test";
    PkgBuffer data(reinterpret_cast<uint8_t*>(const_cast<char*>(writeData)), strlen(writeData));
    int32_t ret = mms.Write(data, strlen(writeData), 0);
    EXPECT_EQ(ret, PKG_INVALID_STREAM);
}
 
HWTEST_F(MemoryMapStreamTest, MemoryMapStreamWriteWithInvalidStart, TestSize.Level1)
{
    MemoryMapStream mms(nullptr, "test.bin", memBuffer_, PkgStream::PkgStreamType_MemoryMap);
    const char *writeData = "write test";
    PkgBuffer data(reinterpret_cast<uint8_t*>(const_cast<char*>(writeData)), strlen(writeData));
    int32_t ret = mms.Write(data, strlen(writeData), 5000);
    EXPECT_EQ(ret, PKG_INVALID_STREAM);
}
 
HWTEST_F(MemoryMapStreamTest, MemoryMapStreamWriteSuccess, TestSize.Level1)
{
    MemoryMapStream mms(nullptr, "test.bin", memBuffer_, PkgStream::PkgStreamType_MemoryMap);
    const char *writeData = "write test data";
    PkgBuffer data(reinterpret_cast<uint8_t*>(const_cast<char*>(writeData)), strlen(writeData));
    int32_t ret = mms.Write(data, strlen(writeData), 0);
    EXPECT_EQ(ret, PKG_SUCCESS);
}
 
HWTEST_F(MemoryMapStreamTest, MemoryMapStreamSeekSEEKSetNegative, TestSize.Level1)
{
    MemoryMapStream mms(nullptr, "test.bin", memBuffer_, PkgStream::PkgStreamType_MemoryMap);
    int32_t ret = mms.Seek(-1, SEEK_SET);
    EXPECT_EQ(ret, PKG_INVALID_STREAM);
}
 
HWTEST_F(MemoryMapStreamTest, MemoryMapStreamSeekSEEKSetValid, TestSize.Level1)
{
    MemoryMapStream mms(nullptr, "test.bin", memBuffer_, PkgStream::PkgStreamType_MemoryMap);
    int32_t ret = mms.Seek(100, SEEK_SET);
    EXPECT_EQ(ret, PKG_SUCCESS);
}
 
HWTEST_F(MemoryMapStreamTest, MemoryMapStreamSeekSEEKSetExceed, TestSize.Level1)
{
    MemoryMapStream mms(nullptr, "test.bin", memBuffer_, PkgStream::PkgStreamType_MemoryMap);
    int32_t ret = mms.Seek(5000, SEEK_SET);
    EXPECT_EQ(ret, PKG_INVALID_STREAM);
}
 
HWTEST_F(MemoryMapStreamTest, MemoryMapStreamSeekSEEKCurExceed, TestSize.Level1)
{
    MemoryMapStream mms(nullptr, "test.bin", memBuffer_, PkgStream::PkgStreamType_MemoryMap);
    mms.Seek(100, SEEK_SET);
    int32_t ret = mms.Seek(5000, SEEK_CUR);
    EXPECT_EQ(ret, PKG_INVALID_STREAM);
}
 
HWTEST_F(MemoryMapStreamTest, MemoryMapStreamSeekSEEKCurValid, TestSize.Level1)
{
    MemoryMapStream mms(nullptr, "test.bin", memBuffer_, PkgStream::PkgStreamType_MemoryMap);
    mms.Seek(100, SEEK_SET);
    int32_t ret = mms.Seek(50, SEEK_CUR);
    EXPECT_EQ(ret, PKG_SUCCESS);
}
 
HWTEST_F(MemoryMapStreamTest, MemoryMapStreamSeekSEEKEndPositive, TestSize.Level1)
{
    MemoryMapStream mms(nullptr, "test.bin", memBuffer_, PkgStream::PkgStreamType_MemoryMap);
    int32_t ret = mms.Seek(1, SEEK_END);
    EXPECT_EQ(ret, PKG_INVALID_STREAM);
}
 
HWTEST_F(MemoryMapStreamTest, MemoryMapStreamSeekSEEKEndValid, TestSize.Level1)
{
    MemoryMapStream mms(nullptr, "test.bin", memBuffer_, PkgStream::PkgStreamType_MemoryMap);
    int32_t ret = mms.Seek(-100, SEEK_END);
    EXPECT_EQ(ret, PKG_SUCCESS);
}
 
HWTEST_F(MemoryMapStreamTest, MemoryMapStreamSeekSEEKEndOverflow, TestSize.Level1)
{
    MemoryMapStream mms(nullptr, "test.bin", memBuffer_, PkgStream::PkgStreamType_MemoryMap);
    int32_t ret = mms.Seek(-5000, SEEK_END);
    EXPECT_EQ(ret, PKG_INVALID_STREAM);
}
 
HWTEST_F(MemoryMapStreamTest, MemoryMapStreamGetBuffer, TestSize.Level1)
{
    MemoryMapStream mms(nullptr, "test.bin", memBuffer_, PkgStream::PkgStreamType_MemoryMap);
    PkgBuffer outBuffer;
    int32_t ret = mms.GetBuffer(outBuffer);
    EXPECT_EQ(ret, PKG_SUCCESS);
    EXPECT_EQ(outBuffer.buffer, memBuffer_.buffer);
    EXPECT_EQ(outBuffer.length, memBuffer_.length);
}
 
HWTEST_F(FlowDataStreamTest, FlowDataStreamReadWithInvalidOffset, TestSize.Level1)
{
    FlowDataStream fds(nullptr, "test.bin", 1000, ringBuf_, PkgStream::PkgStreamType_FlowData);
    PkgBuffer data(100);
    size_t readLen = 0;
    int32_t ret = fds.Read(data, 100, 50, readLen);
    EXPECT_EQ(ret, PKG_INVALID_STREAM);
}
 
HWTEST_F(FlowDataStreamTest, FlowDataStreamReadWithInsufficientBuffer, TestSize.Level1)
{
    FlowDataStream fds(nullptr, "test.bin", 1000, ringBuf_, PkgStream::PkgStreamType_FlowData);
    PkgBuffer data(10);
    size_t readLen = 0;
    int32_t ret = fds.Read(data, 0, 100, readLen);
    EXPECT_EQ(ret, PKG_INVALID_STREAM);
}
 
HWTEST_F(FlowDataStreamTest, FlowDataStreamWriteWithNullRingBuf, TestSize.Level1)
{
    FlowDataStream fds(nullptr, "test.bin", 1000, nullptr, PkgStream::PkgStreamType_FlowData);
    const char *writeData = "write test";
    PkgBuffer data(reinterpret_cast<uint8_t*>(const_cast<char*>(writeData)), strlen(writeData));
    int32_t ret = fds.Write(data, strlen(writeData), 0);
    EXPECT_EQ(ret, PKG_INVALID_STREAM);
}
 
HWTEST_F(FlowDataStreamTest, FlowDataStreamWriteWithInvalidOffset, TestSize.Level1)
{
    FlowDataStream fds(nullptr, "test.bin", 1000, ringBuf_, PkgStream::PkgStreamType_FlowData);
    const char *writeData = "write test";
    PkgBuffer data(reinterpret_cast<uint8_t*>(const_cast<char*>(writeData)), strlen(writeData));
    int32_t ret = fds.Write(data, strlen(writeData), 100);
    EXPECT_EQ(ret, PKG_INVALID_STREAM);
}
 
HWTEST_F(FlowDataStreamTest, FlowDataStreamWriteSuccess, TestSize.Level1)
{
    FlowDataStream fds(nullptr, "test.bin", 1000, ringBuf_, PkgStream::PkgStreamType_FlowData);
    const char *writeData = "write test data";
    PkgBuffer data(reinterpret_cast<uint8_t*>(const_cast<char*>(writeData)), strlen(writeData));
    int32_t ret = fds.Write(data, strlen(writeData), 0);
    EXPECT_EQ(ret, PKG_SUCCESS);
}
 
HWTEST_F(FlowDataStreamTest, FlowDataStreamStop, TestSize.Level1)
{
    FlowDataStream fds(nullptr, "test.bin", 1000, ringBuf_, PkgStream::PkgStreamType_FlowData);
    fds.Stop();
}
 
HWTEST_F(FlowDataStreamTest, FlowDataStreamGetReadOffset, TestSize.Level1)
{
    FlowDataStream fds(nullptr, "test.bin", 1000, ringBuf_, PkgStream::PkgStreamType_FlowData);
    size_t offset = fds.GetReadOffset();
    EXPECT_EQ(offset, 0);
}
 
HWTEST_F(FlowDataStreamTest, FlowDataStreamGetFileLength, TestSize.Level1)
{
    FlowDataStream fds(nullptr, "test.bin", 1000, ringBuf_, PkgStream::PkgStreamType_FlowData);
    size_t len = fds.GetFileLength();
    EXPECT_EQ(len, 1000);
}
 
HWTEST_F(FlowDataStreamTest, FlowDataStreamGetStreamType, TestSize.Level1)
{
    FlowDataStream fds(nullptr, "test.bin", 1000, ringBuf_, PkgStream::PkgStreamType_FlowData);
    EXPECT_EQ(fds.GetStreamType(), PkgStream::PkgStreamType_FlowData);
}
 
class ProcessorStreamTest : public PkgTest {
public:
    ProcessorStreamTest() {}
    ~ProcessorStreamTest() override {}
 
protected:
    void SetUp() override
    {
        PkgTest::SetUp();
    }
    void TearDown() override
    {
        PkgTest::TearDown();
    }
 
    static int32_t MockProcessor(const PkgBuffer &data, size_t size, size_t start, bool isEnd, const void *context)
    {
        UNUSED(data);
        UNUSED(start);
        UNUSED(context);
        if (isEnd) {
            return PKG_SUCCESS;
        }
        return static_cast<int32_t>(size);
    }
};
 
HWTEST_F(ProcessorStreamTest, ProcessorStreamWriteWithNullProcessor, TestSize.Level1)
{
    ProcessorStream ps(nullptr, "test.bin", nullptr, nullptr);
    const char *writeData = "write test";
    PkgBuffer data(reinterpret_cast<uint8_t*>(const_cast<char*>(writeData)), strlen(writeData));
    int32_t ret = ps.Write(data, strlen(writeData), 0);
    EXPECT_EQ(ret, PKG_INVALID_STREAM);
}
 
HWTEST_F(ProcessorStreamTest, ProcessorStreamWriteSuccess, TestSize.Level1)
{
    ProcessorStream ps(nullptr, "test.bin", MockProcessor, this);
    const char *writeData = "write test data";
    PkgBuffer data(reinterpret_cast<uint8_t*>(const_cast<char*>(writeData)), strlen(writeData));
    int32_t ret = ps.Write(data, strlen(writeData), 0);
    EXPECT_EQ(ret, static_cast<int32_t>(strlen(writeData)));
}
 
HWTEST_F(ProcessorStreamTest, ProcessorStreamFlushWithNullProcessor, TestSize.Level1)
{
    ProcessorStream ps(nullptr, "test.bin", nullptr, nullptr);
    int32_t ret = ps.Flush(100);
    EXPECT_EQ(ret, PKG_INVALID_STREAM);
}
 
HWTEST_F(ProcessorStreamTest, ProcessorStreamFlushSuccess, TestSize.Level1)
{
    ProcessorStream ps(nullptr, "test.bin", MockProcessor, this);
    int32_t ret = ps.Flush(100);
    EXPECT_EQ(ret, PKG_SUCCESS);
}
 
HWTEST_F(ProcessorStreamTest, ProcessorStreamRead, TestSize.Level1)
{
    ProcessorStream ps(nullptr, "test.bin", MockProcessor, this);
    PkgBuffer data(100);
    size_t readLen = 0;
    int32_t ret = ps.Read(data, 0, 100, readLen);
    EXPECT_EQ(ret, PKG_INVALID_STREAM);
}
 
HWTEST_F(ProcessorStreamTest, ProcessorStreamGetStreamType, TestSize.Level1)
{
    ProcessorStream ps(nullptr, "test.bin", MockProcessor, this);
    EXPECT_EQ(ps.GetStreamType(), PkgStream::PkgStreamType_Process);
}
 
HWTEST_F(FileStreamTest, FileStreamReadWithReadLenZero, TestSize.Level1)
{
    FileStream fs(nullptr, "test.bin", nullptr, PkgStream::PkgStreamType_Write);
    const char *writeData = "write test";
    PkgBuffer data(reinterpret_cast<uint8_t*>(const_cast<char*>(writeData)), strlen(writeData));
    int32_t ret = fs.Write(data, strlen(writeData), 0);
    EXPECT_EQ(ret, PKG_INVALID_STREAM);
}
 
HWTEST_F(FileStreamTest, FileStreamWriteWithFseekFail, TestSize.Level1)
{
    mkdir("/data/updater", 0755);
    std::string writeFilePath = "/data/updater/test_write_fail.bin";
    FILE *fp = fopen(writeFilePath.c_str(), "wb");
    ASSERT_TRUE(fp != nullptr);
    FileStream fs(nullptr, writeFilePath.c_str(), fp, PkgStream::PkgStreamType_Write);
    const char *writeData = "write test data";
    PkgBuffer data(reinterpret_cast<uint8_t*>(const_cast<char*>(writeData)), strlen(writeData));
    fclose(fp);
    fp = fopen(writeFilePath.c_str(), "rb");
    ASSERT_TRUE(fp != nullptr);
    int32_t ret = fs.Write(data, strlen(writeData), 0);
    EXPECT_EQ(ret, PKG_INVALID_STREAM);
    fclose(fp);
    unlink(writeFilePath.c_str());
}
 
HWTEST_F(FileStreamTest, FileStreamWriteWithWriteFail, TestSize.Level1)
{
    std::string writeFilePath = "/data/updater/test_write_fail2.bin";
    FILE *fp = fopen(writeFilePath.c_str(), "wb");
    ASSERT_TRUE(fp != nullptr);
    FileStream fs(nullptr, writeFilePath.c_str(), fp, PkgStream::PkgStreamType_Write);
    const char *writeData = "write test data";
    PkgBuffer data(reinterpret_cast<uint8_t*>(const_cast<char*>(writeData)), strlen(writeData));
    int32_t ret = fs.Write(data, strlen(writeData), 0);
    EXPECT_EQ(ret, PKG_SUCCESS);
    fclose(fp);
    unlink(writeFilePath.c_str());
}
 
HWTEST_F(FileStreamTest, FileStreamGetFileLengthWithSeekFail, TestSize.Level1)
{
    FILE *fp = fopen(testFilePath_.c_str(), "rb");
    ASSERT_TRUE(fp != nullptr);
    FileStream fs(nullptr, testFilePath_.c_str(), fp, PkgStream::PkgStreamType_Read);
    fseek(fp, -1, SEEK_END);
    size_t len = fs.GetFileLength();
    EXPECT_GT(len, 0);
    fclose(fp);
}
 
HWTEST_F(FileStreamTest, FileStreamGetReadOffset, TestSize.Level1)
{
    FILE *fp = fopen(testFilePath_.c_str(), "rb");
    ASSERT_TRUE(fp != nullptr);
    FileStream fs(nullptr, testFilePath_.c_str(), fp, PkgStream::PkgStreamType_Read);
    fs.Seek(5, SEEK_SET);
    int64_t offset = fs.GetReadOffset();
    EXPECT_EQ(offset, 5);
    fclose(fp);
}
 
HWTEST_F(MemoryMapStreamTest, MemoryMapStreamFlushSuccess, TestSize.Level1)
{
    MemoryMapStream mms(nullptr, "test.bin", memBuffer_, PkgStream::PkgStreamType_MemoryMap);
    int32_t ret = mms.Flush(4096);
    EXPECT_EQ(ret, PKG_SUCCESS);
}
 
HWTEST_F(MemoryMapStreamTest, MemoryMapStreamFlushWithSizeMismatch, TestSize.Level1)
{
    MemoryMapStream mms(nullptr, "test.bin", memBuffer_, PkgStream::PkgStreamType_MemoryMap);
    int32_t ret = mms.Flush(8192);
    EXPECT_EQ(ret, PKG_SUCCESS);
}
 
HWTEST_F(FlowDataStreamTest, FlowDataStreamReadSuccess, TestSize.Level1)
{
    FlowDataStream fds(nullptr, "test.bin", 1000, ringBuf_, PkgStream::PkgStreamType_FlowData);
    const char *writeData = "test data for read";
    PkgBuffer data(reinterpret_cast<uint8_t*>(const_cast<char*>(writeData)), strlen(writeData));
    fds.Write(data, strlen(writeData), 0);
    PkgBuffer readData(100);
    readData.data.resize(100);
    size_t readLen = 0;
    int32_t ret = fds.Read(readData, 0, strlen(writeData), readLen);
    EXPECT_EQ(ret, PKG_SUCCESS);
    EXPECT_GT(readLen, 0);
}
 
HWTEST_F(FlowDataStreamTest, FlowDataStreamReadWithMemcpyFail, TestSize.Level1)
{
    FlowDataStream fds(nullptr, "test.bin", 1000, nullptr, PkgStream::PkgStreamType_FlowData);
    PkgBuffer data(100);
    data.buffer = nullptr;
    size_t readLen = 0;
    int32_t ret = fds.Read(data, 0, 100, readLen);
    EXPECT_EQ(ret, PKG_INVALID_STREAM);
}
 
HWTEST_F(MemoryMapStreamTest, MemoryMapStreamReadWithEmptyDataVector, TestSize.Level1)
{
    memcpy_s(memBuffer_.buffer, sizeof(memBuffer_.buffer), "Test data for memory map", 25);
    MemoryMapStream mms(nullptr, "test.bin", memBuffer_, PkgStream::PkgStreamType_MemoryMap);
    PkgBuffer data;
    data.buffer = nullptr;
    data.length = 30;
    data.data.resize(0);
    size_t readLen = 0;
    int32_t ret = mms.Read(data, 0, 20, readLen);
    EXPECT_EQ(ret, PKG_SUCCESS);
    EXPECT_EQ(data.buffer, memBuffer_.buffer);
    EXPECT_EQ(readLen, 20);
}
 
HWTEST_F(MemoryMapStreamTest, MemoryMapStreamWriteWithSizeExceedCopyLen, TestSize.Level1)
{
    constexpr size_t bigSize = 1024 * 1024 * 10;
    memBuffer_.buffer = new uint8_t[bigSize];
    memBuffer_.length = bigSize;
    std::fill(memBuffer_.buffer, memBuffer_.buffer + bigSize, 0);
    constexpr size_t largeDataSize = bigSize << 1;
    std::vector<uint8_t> largeData(largeDataSize, 'x');
    PkgBuffer data(largeData.data(), largeDataSize);
    MemoryMapStream mms(nullptr, "test.bin", memBuffer_, PkgStream::PkgStreamType_MemoryMap);
    int32_t ret = mms.Write(data, largeDataSize, 0);
    EXPECT_EQ(ret, PKG_INVALID_STREAM);
}
 
HWTEST_F(MemoryMapStreamTest, MemoryMapStreamDestructorWithNonMemoryMapType, TestSize.Level1)
{
    PkgBuffer buf;
    buf.buffer = new uint8_t[1024];
    buf.length = 1024;
    {
        MemoryMapStream mms(nullptr, "test.bin", buf, PkgStream::PkgStreamType_Write);
    }
    buf.buffer = nullptr;
}
 
HWTEST_F(FlowDataStreamTest, FlowDataStreamSeekAlwaysFail, TestSize.Level1)
{
    FlowDataStream fds(nullptr, "test.bin", 1000, ringBuf_, PkgStream::PkgStreamType_FlowData);
    int32_t ret = fds.Seek(0, SEEK_SET);
    EXPECT_EQ(ret, PKG_INVALID_STREAM);
}
 
HWTEST_F(FlowDataStreamTest, FlowDataStreamFlushAlwaysFail, TestSize.Level1)
{
    FlowDataStream fds(nullptr, "test.bin", 1000, ringBuf_, PkgStream::PkgStreamType_FlowData);
    int32_t ret = fds.Flush(100);
    EXPECT_EQ(ret, PKG_INVALID_STREAM);
}
 
HWTEST_F(FlowDataStreamTest, FlowDataStreamReadWithWriteOffsetMismatch, TestSize.Level1)
{
    FlowDataStream fds(nullptr, "test.bin", 1000, ringBuf_, PkgStream::PkgStreamType_FlowData);
    const char *writeData = "test data";
    PkgBuffer data(reinterpret_cast<uint8_t*>(const_cast<char*>(writeData)), strlen(writeData));
    fds.Write(data, strlen(writeData), 0);
    PkgBuffer readData(100);
    readData.data.resize(100);
    size_t readLen = 0;
    int32_t ret = fds.Read(readData, 50, strlen(writeData), readLen);
    EXPECT_EQ(ret, PKG_INVALID_STREAM);
}
 
HWTEST_F(FlowDataStreamTest, FlowDataStreamWriteWithWriteOffsetMismatch, TestSize.Level1)
{
    FlowDataStream fds(nullptr, "test.bin", 1000, ringBuf_, PkgStream::PkgStreamType_FlowData);
    const char *writeData = "test data";
    PkgBuffer data(reinterpret_cast<uint8_t*>(const_cast<char*>(writeData)), strlen(writeData));
    fds.Write(data, strlen(writeData), 0);
    const char *writeData2 = "more data";
    PkgBuffer data2(reinterpret_cast<uint8_t*>(const_cast<char*>(writeData2)), strlen(writeData2));
    int32_t ret = fds.Write(data2, strlen(writeData2), 50);
    EXPECT_EQ(ret, PKG_INVALID_STREAM);
}
 
HWTEST_F(ProcessorStreamTest, ProcessorStreamGetFileLengthAlwaysZero, TestSize.Level1)
{
    ProcessorStream ps(nullptr, "test.bin", MockProcessor, this);
    size_t len = ps.GetFileLength();
    EXPECT_EQ(len, 0);
}
 
HWTEST_F(FileStreamTest, FileStreamGetBufferBaseImpl, TestSize.Level1)
{
    FileStream fs(nullptr, "test.bin", nullptr, PkgStream::PkgStreamType_Read);
    PkgBuffer buffer;
    int32_t ret = fs.GetBuffer(buffer);
    EXPECT_EQ(ret, PKG_SUCCESS);
}
 
HWTEST_F(MemoryMapStreamTest, MemoryMapStreamGetStreamTypeWrite, TestSize.Level1)
{
    MemoryMapStream mms(nullptr, "test.bin", memBuffer_, PkgStream::PkgStreamType_Write);
    EXPECT_EQ(mms.GetStreamType(), PkgStream::PkgStreamType_Write);
}
 
HWTEST_F(MemoryMapStreamTest, MemoryMapStreamGetStreamTypeMemoryMap, TestSize.Level1)
{
    MemoryMapStream mms(nullptr, "test.bin", memBuffer_, PkgStream::PkgStreamType_MemoryMap);
    EXPECT_EQ(mms.GetStreamType(), PkgStream::PkgStreamType_MemoryMap);
}
 
HWTEST_F(FileStreamTest, FileStreamGetStreamTypeWrite, TestSize.Level1)
{
    FileStream fs(nullptr, "test.bin", nullptr, PkgStream::PkgStreamType_Write);
    EXPECT_EQ(fs.GetStreamType(), PkgStream::PkgStreamType_Write);
}
 
class ShmDataStreamTest : public PkgTest {
public:
    ShmDataStreamTest() : shmId_("ut_test_shm") {}
    ~ShmDataStreamTest() override {}
 
protected:
    void SetUp() override
    {
        PkgTest::SetUp();
        shmInfo_.shmId = shmId_;
        const int defaultFileLength = 1024;
        shmInfo_.fileLen = defaultFileLength;
        shmInfo_.offset = 0;
    }
    void TearDown() override
    {
        shm_unlink(shmId_.c_str());
        PkgTest::TearDown();
    }
 
    ShmInfo shmInfo_;
    std::string shmId_;
};
 
HWTEST_F(ShmDataStreamTest, ShmDataStreamGetStreamType, TestSize.Level1)
{
    ShmDataStream sds(nullptr, "test.bin", shmInfo_, PkgStream::PkgStreamType_ShmData);
    EXPECT_EQ(sds.GetStreamType(), PkgStream::PkgStreamType_ShmData);
}
 
HWTEST_F(ShmDataStreamTest, ShmDataStreamGetFileLength, TestSize.Level1)
{
    ShmDataStream sds(nullptr, "test.bin", shmInfo_, PkgStream::PkgStreamType_ShmData);
    size_t len = sds.GetFileLength();
    EXPECT_EQ(len, 1024);
}
 
HWTEST_F(ShmDataStreamTest, ShmDataStreamSetpkgLen, TestSize.Level1)
{
    ShmDataStream sds(nullptr, "test.bin", shmInfo_, PkgStream::PkgStreamType_ShmData);
    sds.SetpkgLen(2048);
    size_t len = sds.GetFileLength();
    EXPECT_EQ(len, 2048);
}
 
HWTEST_F(ShmDataStreamTest, ShmDataStreamGetReadOffset, TestSize.Level1)
{
    ShmDataStream sds(nullptr, "test.bin", shmInfo_, PkgStream::PkgStreamType_ShmData);
    sds.SetOffset(100);
    int64_t offset = sds.GetReadOffset();
    EXPECT_EQ(offset, 100);
}
 
HWTEST_F(ShmDataStreamTest, ShmDataStreamSeekAlwaysSuccess, TestSize.Level1)
{
    ShmDataStream sds(nullptr, "test.bin", shmInfo_, PkgStream::PkgStreamType_ShmData);
    int32_t ret = sds.Seek(100, SEEK_SET);
    EXPECT_EQ(ret, PKG_SUCCESS);
    ret = sds.Seek(-50, SEEK_END);
    EXPECT_EQ(ret, PKG_SUCCESS);
    ret = sds.Seek(10, SEEK_CUR);
    EXPECT_EQ(ret, PKG_SUCCESS);
}
 
HWTEST_F(ShmDataStreamTest, ShmDataStreamCreateShmRingBufferWithNewShm, TestSize.Level1)
{
    ShmDataStream sds(nullptr, "test.bin", shmInfo_, PkgStream::PkgStreamType_ShmData);
    int32_t ret = sds.CreateShmRingBuffer();
    EXPECT_EQ(ret, 0);
    sds.Exit();
}
 
HWTEST_F(ShmDataStreamTest, ShmDataStreamCreateShmRingBufferFails, TestSize.Level1)
{
    ShmInfo invalidInfo;
    invalidInfo.shmId = "";
    invalidInfo.fileLen = 1024;
    invalidInfo.offset = 0;
    ShmDataStream sds(nullptr, "test.bin", invalidInfo, PkgStream::PkgStreamType_ShmData);
    int32_t ret = sds.CreateShmRingBuffer();
    EXPECT_EQ(ret, -1);
}
 
HWTEST_F(ShmDataStreamTest, ShmDataStreamInitShmRingBufferWithExistingShm, TestSize.Level1)
{
    ShmDataStream sdsCreate(nullptr, "test.bin", shmInfo_, PkgStream::PkgStreamType_ShmData);
    ASSERT_EQ(sdsCreate.CreateShmRingBuffer(), 0);
    ShmDataStream sdsInit(nullptr, "test.bin", shmInfo_, PkgStream::PkgStreamType_ShmData);
    int32_t ret = sdsInit.InitShmRingBuffer();
    EXPECT_EQ(ret, 0);
    sdsInit.Exit();
}
 
HWTEST_F(ShmDataStreamTest, ShmDataStreamInitShmRingBufferFailsWithNonexistentShm, TestSize.Level1)
{
    ShmInfo nonexistentInfo;
    nonexistentInfo.shmId = "nonexistent_shm_id_12345";
    nonexistentInfo.fileLen = 1024;
    nonexistentInfo.offset = 0;
    ShmDataStream sdsInit(nullptr, "test.bin", nonexistentInfo, PkgStream::PkgStreamType_ShmData);
    int32_t ret = sdsInit.InitShmRingBuffer();
    EXPECT_EQ(ret, -1);
}
 
HWTEST_F(ShmDataStreamTest, ShmDataStreamStopWithNullRb, TestSize.Level1)
{
    ShmInfo info;
    info.shmId = "test_null_rb_shm";
    info.fileLen = 1024;
    info.offset = 0;
    ShmDataStream sds(nullptr, "test.bin", info, PkgStream::PkgStreamType_ShmData);
    sds.Stop();
}
 
HWTEST_F(ShmDataStreamTest, ShmDataStreamStopWithValidRb, TestSize.Level1)
{
    ShmDataStream sds(nullptr, "test.bin", shmInfo_, PkgStream::PkgStreamType_ShmData);
    ASSERT_EQ(sds.CreateShmRingBuffer(), 0);
    sds.Stop();
}
 
HWTEST_F(ShmDataStreamTest, ShmDataStreamExitWithValidRb, TestSize.Level1)
{
    ShmDataStream sds(nullptr, "test.bin", shmInfo_, PkgStream::PkgStreamType_ShmData);
    ASSERT_EQ(sds.CreateShmRingBuffer(), 0);
    sds.Exit();
}
 
HWTEST_F(ShmDataStreamTest, ShmDataStreamExitWithNullRb, TestSize.Level1)
{
    ShmDataStream sds(nullptr, "test.bin", shmInfo_, PkgStream::PkgStreamType_ShmData);
    sds.Exit();
}
 
HWTEST_F(ShmDataStreamTest, ShmDataStreamReadWithOffsetMismatch, TestSize.Level1)
{
    ShmDataStream sds(nullptr, "test.bin", shmInfo_, PkgStream::PkgStreamType_ShmData);
    sds.SetOffset(100);
    PkgBuffer data(100);
    data.data.resize(100);
    size_t readLen = 0;
    int32_t ret = sds.Read(data, 0, 50, readLen);
    EXPECT_EQ(ret, PKG_INVALID_STREAM);
}
 
HWTEST_F(ShmDataStreamTest, ShmDataStreamReadWithInsufficientBuffer, TestSize.Level1)
{
    ShmDataStream sds(nullptr, "test.bin", shmInfo_, PkgStream::PkgStreamType_ShmData);
    sds.SetOffset(0);
    PkgBuffer data(10);
    data.data.resize(10);
    size_t readLen = 0;
    int32_t ret = sds.Read(data, 0, 100, readLen);
    EXPECT_EQ(ret, PKG_INVALID_STREAM);
}
 
HWTEST_F(ShmDataStreamTest, ShmDataStreamReadWithNullBuffer, TestSize.Level1)
{
    ShmDataStream sds(nullptr, "test.bin", shmInfo_, PkgStream::PkgStreamType_ShmData);
    sds.SetOffset(0);
    PkgBuffer data;
    data.buffer = nullptr;
    data.length = 100;
    size_t readLen = 0;
    int32_t ret = sds.Read(data, 0, 50, readLen);
    EXPECT_EQ(ret, PKG_INVALID_STREAM);
}
 
HWTEST_F(ShmDataStreamTest, ShmDataStreamWriteWithOffsetMismatch, TestSize.Level1)
{
    ShmDataStream sds(nullptr, "test.bin", shmInfo_, PkgStream::PkgStreamType_ShmData);
    sds.SetOffset(100);
    const char *writeData = "test data";
    PkgBuffer data(reinterpret_cast<uint8_t*>(const_cast<char*>(writeData)), strlen(writeData));
    int32_t ret = sds.Write(data, strlen(writeData), 0);
    EXPECT_EQ(ret, PKG_INVALID_STREAM);
}
 
HWTEST_F(ShmDataStreamTest, ShmDataStreamWriteWithNullBuffer, TestSize.Level1)
{
    ShmDataStream sds(nullptr, "test.bin", shmInfo_, PkgStream::PkgStreamType_ShmData);
    sds.SetOffset(0);
    PkgBuffer data;
    data.buffer = nullptr;
    data.length = 0;
    int32_t ret = sds.Write(data, 0, 0);
    EXPECT_EQ(ret, PKG_INVALID_STREAM);
}
 
HWTEST_F(ShmDataStreamTest, ShmDataStreamReadFullyWriteReadSequence, TestSize.Level1)
{
    ShmInfo shmInfo;
    shmInfo.shmId = "ut_readfully_coverage_test";
    shmInfo.fileLen = 1024;
    shmInfo.offset = 0;
 
    ShmDataStream sds(nullptr, "test.bin", shmInfo, PkgStream::PkgStreamType_ShmData);
    EXPECT_EQ(sds.CreateShmRingBuffer(), 0);
 
    const char *testData = "data for read fully coverage test";
    PkgBuffer writeData(reinterpret_cast<uint8_t*>(const_cast<char*>(testData)), strlen(testData));
    EXPECT_EQ(sds.Write(writeData, strlen(testData), 0), 0);
 
    PkgBuffer readBuf;
    readBuf.data.resize(100);
    readBuf.buffer = readBuf.data.data();
    readBuf.length = 100;
    size_t readLen = 0;
    sds.SetOffset(0);
    ASSERT_EQ(sds.Read(readBuf, 0, 10, readLen), 0);
    EXPECT_GT(readLen, 0U);
    sds.Exit();
    shm_unlink(shmInfo.shmId.c_str());
}
 
HWTEST_F(ShmDataStreamTest, ShmDataStreamReadWithCurrLenGreaterThanNeedRead, TestSize.Level1)
{
    ShmInfo shmInfo;
    shmInfo.shmId = "ut_currlen_gt_needread";
    shmInfo.fileLen = 1024;
    shmInfo.offset = 0;
 
    ShmDataStream sds(nullptr, "test.bin", shmInfo, PkgStream::PkgStreamType_ShmData);
    EXPECT_EQ(sds.CreateShmRingBuffer(), 0);
 
    const char *testData = "ABCDEFGHIJ";
    PkgBuffer writeData(reinterpret_cast<uint8_t*>(const_cast<char*>(testData)), strlen(testData));
    ASSERT_EQ(sds.Write(writeData, strlen(testData), 0), 0);
 
    sds.SetOffset(0);
    PkgBuffer readBuf;
    readBuf.data.resize(100);
    readBuf.buffer = readBuf.data.data();
    readBuf.length = 100;
    size_t readLen = 0;
    ASSERT_EQ(sds.Read(readBuf, 0, 5, readLen), 0);
    EXPECT_EQ(readLen, 5U);
 
    sds.SetOffset(5);
    PkgBuffer readBuf2;
    readBuf2.data.resize(100);
    readBuf2.buffer = readBuf2.data.data();
    readBuf2.length = 100;
    readLen = 0;
    ASSERT_EQ(sds.Read(readBuf2, 5, 3, readLen), 0);
    EXPECT_EQ(readLen, 3U);
 
    sds.Exit();
    shm_unlink(shmInfo.shmId.c_str());
}
 
HWTEST_F(ShmDataStreamTest, ShmDataStreamReadWithCurrLenLessThanNeedRead, TestSize.Level1)
{
    ShmInfo shmInfo;
    shmInfo.shmId = "ut_currlen_lt_needread";
    shmInfo.fileLen = 1024;
    shmInfo.offset = 0;
 
    ShmDataStream sds(nullptr, "test.bin", shmInfo, PkgStream::PkgStreamType_ShmData, 100);
    EXPECT_EQ(sds.CreateShmRingBuffer(), 0);
 
    const char *testData = "ABCDEFGHIJKLMNOPQRST";
    PkgBuffer writeData(reinterpret_cast<uint8_t*>(const_cast<char*>(testData)), strlen(testData));
    ASSERT_EQ(sds.Write(writeData, strlen(testData), 0), 0);
 
    sds.SetOffset(0);
    PkgBuffer readBuf;
    readBuf.data.resize(100);
    readBuf.buffer = readBuf.data.data();
    readBuf.length = 100;
    size_t readLen = 0;
    ASSERT_EQ(sds.Read(readBuf, 0, 5, readLen), 0);
    EXPECT_EQ(readLen, 5U);
    EXPECT_EQ(memcmp(readBuf.buffer, "ABCDE", 5), 0);
 
    sds.SetOffset(5);
    PkgBuffer readBuf2;
    readBuf2.data.resize(100);
    readBuf2.buffer = readBuf2.data.data();
    readBuf2.length = 100;
    readLen = 0;
    int32_t ret = sds.Read(readBuf2, 5, 20, readLen);
    EXPECT_EQ(ret, PKG_INVALID_STREAM);
    EXPECT_EQ(readLen, 15U);
 
    sds.Exit();
    shm_unlink(shmInfo.shmId.c_str());
}
 
HWTEST_F(ShmDataStreamTest, ShmDataStreamReadFullyNeedReadLenGreaterThanBlockLen, TestSize.Level1)
{
    ShmInfo shmInfo;
    shmInfo.shmId = "ut_needread_ge_blocklen";
    shmInfo.fileLen = 1024;
    shmInfo.offset = 0;
 
    ShmDataStream sds(nullptr, "test.bin", shmInfo, PkgStream::PkgStreamType_ShmData);
    EXPECT_EQ(sds.CreateShmRingBuffer(), 0);
 
    const char *testData = "ABCDEFGHIJKLMNOP";
    PkgBuffer writeData(reinterpret_cast<uint8_t*>(const_cast<char*>(testData)), strlen(testData));
    ASSERT_EQ(sds.Write(writeData, strlen(testData), 0), 0);
 
    sds.SetOffset(0);
    PkgBuffer readBuf;
    readBuf.data.resize(100);
    readBuf.buffer = readBuf.data.data();
    readBuf.length = 100;
    size_t readLen = 0;
    ASSERT_EQ(sds.Read(readBuf, 0, 16, readLen), 0);
    EXPECT_EQ(readLen, 16U);
    EXPECT_EQ(memcmp(readBuf.buffer, "ABCDEFGHIJKLMNOP", 16), 0);
 
    sds.Exit();
    shm_unlink(shmInfo.shmId.c_str());
}
 
} // namespace UpdaterUt