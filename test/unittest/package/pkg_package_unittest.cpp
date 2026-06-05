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

#define private public
#define protected public

#include <cstring>
#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include "log.h"
#include "packages_info.h"
#include "pkg_algorithm.h"
#include "pkg_gzipfile.h"
#include "pkg_lz4file.h"
#include "pkg_manager.h"
#include "pkg_manager_impl.h"
#include "pkg_test.h"
#include "pkg_upgradefile.h"
#include "pkg_utils.h"
#include "pkg_zipfile.h"
#include "pkg_streamfile.h"
#include "securec.h"

using namespace std;
using namespace Hpackage;
using namespace Updater;
using namespace testing::ext;

namespace UpdaterUt {

constexpr uint32_t TEST_NODE_ID = 100;
constexpr uint32_t TEST_ZIP_NODE_ID = 100;
constexpr size_t TEST_NAME_LEN = 10;
constexpr uint32_t TEST_FILE_SIZE_1K = 1024;
constexpr uint32_t TEST_FILE_SIZE_2K = 2048;
constexpr uint32_t TEST_FILE_SIZE_4K = 4096;
constexpr uint32_t TEST_PACKED_SIZE_1K = 1024;
constexpr uint32_t TEST_PACKED_SIZE_512 = 512;
constexpr uint32_t TEST_PACKED_SIZE_500 = 500;
constexpr uint32_t TEST_MODIFIED_TIME = 12345;
constexpr uint32_t TEST_DATA_OFFSET_100 = 100;
constexpr uint32_t TEST_DATA_OFFSET_200 = 200;
constexpr uint32_t TEST_HEADER_OFFSET_100 = 100;
constexpr uint32_t TEST_COMPRESSION_LEVEL = 1;
constexpr uint32_t TEST_BLOCK_SIZE_ID = 5;
constexpr uint32_t TEST_CONTENT_CHECKSUM_FLAG = 1;
constexpr uint32_t TEST_COMPONENT_ID_1 = 123;
constexpr uint32_t TEST_COMPONENT_ID_2 = 456;
constexpr uint32_t TEST_RESOURCE_TYPE_1 = 1;
constexpr uint32_t TEST_RESOURCE_TYPE_2 = 3;
constexpr uint32_t TEST_TYPE_1 = 2;
constexpr uint32_t TEST_TYPE_2 = 4;
constexpr uint32_t TEST_COMP_FLAGS_1 = 1;
constexpr uint32_t TEST_UNPACKED_SIZE_1K = 1000;
constexpr uint32_t TEST_CRC32_VALUE = 0x12345678;

constexpr size_t TEST_STRING_LEN_9 = 9;
constexpr size_t TEST_STRING_LEN_10 = 10;

constexpr uint32_t TEST_BUFFER_SIZE_1K = 1024;
constexpr uint32_t TEST_BUFFER_SIZE_2K = 2048;
constexpr uint32_t TEST_BUFFER_SIZE_4K = 4096;
constexpr uint32_t TEST_STREAM_SIZE_1K = 1024;
constexpr uint32_t TEST_STREAM_SIZE_2K = 2048;
constexpr uint32_t TEST_VECTOR_SIZE_5 = 5;
constexpr uint32_t TEST_VECTOR_SIZE_10 = 10;
constexpr uint32_t TEST_VECTOR_SIZE_20 = 20;
constexpr uint32_t TEST_VECTOR_SIZE_50 = 50;
constexpr uint32_t TEST_ENTRY_NODE_ID = 1;
constexpr uint32_t TEST_WRITE_VALUE_10 = 10;
constexpr uint32_t TEST_EXTRA_SIZE_28 = 28;
constexpr uint32_t TEST_WRITE_VALUE_1000 = 1000;
constexpr uint32_t TEST_WRITE_VALUE_2000 = 2000;
constexpr uint32_t TEST_WRITE_VALUE_3000 = 3000;
constexpr uint32_t TEST_WRITE_VALUE_4000 = 4000;
constexpr uint32_t TEST_CRC32_MISMATCH = 0x87654321;
constexpr uint32_t TEST_INVALID_SIGNATURE = 0xFFFFFFFF;
constexpr uint8_t TEST_COMPRESSION_METHOD = 8;
constexpr uint32_t TEST_EXTRA_DATA_OFFSET_20 = 20;
constexpr size_t TEST_OFFSET_2 = 2;

constexpr uint32_t TEST_PACKED_SIZE_2K = 2048;
constexpr uint32_t TEST_RANGE_SECOND_600 = 600;
constexpr size_t TEST_SPLIT_RESULT_SIZE = 3;
constexpr uint32_t MAX_FILE_NAME = 256;
constexpr uint32_t CENTRAL_SIGNATURE = 0x02014b50;
constexpr uint32_t LOCAL_HEADER_SIGNATURE = 0x04034b50;
constexpr int GZIP_MAGIC = 0x00008b1f;
constexpr uint8_t EXTRA_FIELD = 0x04;
constexpr uint8_t ORIG_NAME = 0x08;
constexpr uint8_t HEADER_CRC = 0x02;
constexpr uint32_t PKG_FILE_STATE_IDLE = 0;
constexpr uint32_t PKG_FILE_STATE_WORKING = 1;
constexpr uint32_t PKG_FILE_STATE_CLOSE = 2;

class TestFile : public PkgFileImpl {
public:
    explicit TestFile(PkgManager::PkgManagerPtr pkgManager, PkgStreamPtr stream,
                      PkgFile::PkgType type = PkgFile::PKG_TYPE_UPGRADE)
        : PkgFileImpl(pkgManager, stream, type) {}

    virtual ~TestFile() {}

    virtual int32_t AddEntry(const PkgManager::FileInfoPtr file, const PkgStreamPtr inStream)
    {
        PkgFileImpl::GetPkgInfo();
        PkgFileImpl::AddPkgEntry(inStream->GetFileName());
        return 0;
    }

    virtual int32_t SavePackage(size_t &offset)
    {
        return 0;
    }

    virtual int32_t LoadPackage(std::vector<std::string>& fileNames, VerifyFunction verify = nullptr)
    {
        return 0;
    }
};

class PkgPackageTest : public PkgTest {
public:
    PkgPackageTest() {}
    ~PkgPackageTest() override {}

    int TestPkgFile()
    {
        if (pkgManager_ == nullptr) {
            return PKG_SUCCESS;
        }
        PkgManager::StreamPtr stream = nullptr;
        std::string packagePath = TEST_PATH_TO;
        packagePath += testPackageName;
        int32_t ret = pkgManager_->CreatePkgStream(stream, packagePath, 0, PkgStream::PkgStreamType_Read);
        auto file = std::make_unique<Lz4PkgFile>(pkgManager_, PkgStreamImpl::ConvertPkgStream(stream));
        EXPECT_NE(file, nullptr);
        constexpr uint32_t lz4NodeId = 100;
        auto entry = std::make_unique<Lz4FileEntry>(file.get(), lz4NodeId);
        EXPECT_NE(entry, nullptr);

        EXPECT_NE(((PkgEntryPtr)entry.get())->GetPkgFile(), nullptr);
        Lz4FileInfo fileInfo {};
        ret = entry->Init(&fileInfo.fileInfo, PkgStreamImpl::ConvertPkgStream(stream));
        EXPECT_EQ(ret, 0);
        return 0;
    }

    int TestPkgFileInvalid()
    {
        if (pkgManager_ == nullptr) {
            return PKG_SUCCESS;
        }
        PkgManager::StreamPtr stream = nullptr;
        std::string packagePath = TEST_PATH_TO;
        packagePath += testPackageName;
        int32_t ret = pkgManager_->CreatePkgStream(stream, packagePath, 0, PkgStream::PkgStreamType_Read);
        FileInfo fileInfo;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_,
            PkgStreamImpl::ConvertPkgStream(stream));
        EXPECT_NE(file, nullptr);
        ret = file->AddEntry(&fileInfo, PkgStreamImpl::ConvertPkgStream(stream));
        EXPECT_EQ(ret, 0);
        size_t offset = 0;
        ret = file->SavePackage(offset);
        EXPECT_EQ(ret, 0);
        return 0;
    }

    int TestBigZipEntry()
    {
        EXPECT_NE(pkgManager_, nullptr);
        PkgManager::StreamPtr stream = nullptr;
        std::string packagePath = TEST_PATH_TO;
        uint32_t zipNodeId = TEST_ZIP_NODE_ID;
        packagePath += testPackageName;
        pkgManager_->CreatePkgStream(stream, packagePath, 0, PkgStream::PkgStreamType_Read);
        EXPECT_NE(stream, nullptr);
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_,
            PkgStreamImpl::ConvertPkgStream(stream));
        EXPECT_NE(file, nullptr);
        std::unique_ptr<ZipFileEntry> entry = std::make_unique<ZipFileEntry>(file.get(), zipNodeId);
        EXPECT_NE(entry, nullptr);

        string name = "TestBigZip";
        uint16_t extraSize = 20;
        size_t offsetHalfWord = 2;
        size_t offsetWord = 4;
        size_t offset4Words = 16;
        size_t offset3Words = 12;
        int32_t buffLen = MAX_FILE_NAME + sizeof(LocalFileHeader) + sizeof(DataDescriptor) +
            sizeof(CentralDirEntry) + offsetWord + offset4Words;
        std::vector<uint8_t> buff(buffLen);
        CentralDirEntry* centralDir = (CentralDirEntry *)buff.data();
        (void)memset_s(centralDir, sizeof(CentralDirEntry), 0, sizeof(CentralDirEntry));
        centralDir->signature = CENTRAL_SIGNATURE;
        centralDir->compressionMethod = PKG_COMPRESS_METHOD_ZIP;
        centralDir->compressedSize = UINT_MAX;
        centralDir->uncompressedSize = UINT_MAX;
        centralDir->nameSize = name.length();
        centralDir->extraSize = extraSize;
        int32_t ret = memcpy_s(buff.data() + sizeof(CentralDirEntry), name.length(), name.c_str(), name.length());
        EXPECT_EQ(ret, 0);
        WriteLE16(buff.data() + sizeof(CentralDirEntry) + name.length(), 1);
        WriteLE16(buff.data() + sizeof(CentralDirEntry) + name.length() + offsetHalfWord, offset4Words);
        size_t giantNumber = 100000;
        size_t size = UINT_MAX + giantNumber;
        WriteLE64(buff.data() + sizeof(CentralDirEntry) + name.length() + offsetWord, size);
        WriteLE64(buff.data() + sizeof(CentralDirEntry) + name.length() + offset3Words, size);
        size_t decodeLen = 0;
        PkgBuffer buffer(buff);
        entry->DecodeCentralDirEntry(nullptr, buffer, 0, decodeLen);
        return 0;
    }

    void WriteLE64(uint8_t *buff, size_t size) const
    {
        *reinterpret_cast<size_t *>(buff) = size;
    }

    int TestPackageInfoFail()
    {
        PkgManager::PkgManagerPtr manager = PkgManager::CreatePackageInstance();
        PackagesInfoPtr pkginfomanager = PackagesInfo::GetPackagesInfoInstance();
        std::vector<std::string> target;
        std::vector<std::string> tmp;

        target = pkginfomanager->GetOTAVersion(nullptr, "", "");
        EXPECT_EQ(target, tmp);
        target = pkginfomanager->GetOTAVersion(manager, "", "");
        EXPECT_EQ(target, tmp);
        target = pkginfomanager->GetBoardID(nullptr, "", "");
        EXPECT_EQ(target, tmp);
        target = pkginfomanager->GetBoardID(manager, "", "");
        EXPECT_EQ(target, tmp);

        bool ret = pkginfomanager->IsAllowRollback();
        EXPECT_EQ(ret, false);
        PackagesInfo::ReleasePackagesInfoInstance(pkginfomanager);
        PkgManager::ReleasePackageInstance(manager);
        return 0;
    }

    int TestUpdaterPreProcess()
    {
        PkgManager::PkgManagerPtr pkgManager = PkgManager::CreatePackageInstance();
        std::string packagePath = testPackagePath + "test_package.zip";
        std::vector<std::string> components;
        int32_t ret = pkgManager->LoadPackage(packagePath, Utils::GetCertName(), components);
        EXPECT_EQ(ret, PKG_SUCCESS);

        PackagesInfoPtr pkginfomanager = PackagesInfo::GetPackagesInfoInstance();
        std::vector<std::string> result;
        std::vector<std::string> targetVersions = pkginfomanager->GetOTAVersion(
            pkgManager, "/version_list", testPackagePath);
        EXPECT_NE(targetVersions, result);

        std::vector<std::string> boardIdList = pkginfomanager->GetBoardID(pkgManager, "/board_list", "");
        EXPECT_NE(boardIdList, result);
        PackagesInfo::ReleasePackagesInfoInstance(pkginfomanager);
        PkgManager::ReleasePackageInstance(pkgManager);
        return 0;
    }

    void TestZipFileEntryCombineTimeAndDate()
    {
        uint32_t zipNodeId = TEST_ZIP_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<ZipFileEntry> entry = std::make_unique<ZipFileEntry>(file.get(), zipNodeId);

        uint16_t modifiedTime = 0;
        uint16_t modifiedDate = 0;
        time_t timeValue = 0;
        entry->CombineTimeAndDate(timeValue, modifiedTime, modifiedDate);
        EXPECT_GE(timeValue, 0);

        modifiedTime = 0x1234;
        modifiedDate = 0x1234;
        entry->CombineTimeAndDate(timeValue, modifiedTime, modifiedDate);
        EXPECT_GE(timeValue, 0);
    }

    void TestZipFileEntryGetOriginalSize()
    {
        uint32_t zipNodeId = TEST_ZIP_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<ZipFileEntry> entry = std::make_unique<ZipFileEntry>(file.get(), zipNodeId);

        size_t originalSize = entry->GetOriginalSize();
        EXPECT_EQ(originalSize, 0);
    }

    void TestZipFileEntryGetEntryRange()
    {
        uint32_t zipNodeId = TEST_ZIP_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<ZipFileEntry> entry = std::make_unique<ZipFileEntry>(file.get(), zipNodeId);

        auto range = entry->GetEntryRange();
        EXPECT_EQ(range.first, 0);
        EXPECT_EQ(range.second, 0);
    }

    void TestZipFileEntryEncodeLocalFileHeader()
    {
        uint32_t zipNodeId = TEST_ZIP_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<ZipFileEntry> entry = std::make_unique<ZipFileEntry>(file.get(), zipNodeId);

        std::vector<uint8_t> buff(sizeof(LocalFileHeader) + MAX_FILE_NAME);
        size_t nameLen = TEST_NAME_LEN;
        entry->fileInfo_.fileInfo.identity = "test_file";
        entry->crc32_ = TEST_CRC32_VALUE;
        entry->fileInfo_.fileInfo.packedSize = TEST_PACKED_SIZE_1K;
        entry->fileInfo_.fileInfo.unpackedSize = TEST_FILE_SIZE_2K;
        entry->fileInfo_.fileInfo.modifiedTime = TEST_MODIFIED_TIME;

        int32_t ret = entry->EncodeLocalFileHeader(buff.data(), buff.size(), false, nameLen);
        EXPECT_EQ(ret, PKG_SUCCESS);

        LocalFileHeader* header = reinterpret_cast<LocalFileHeader*>(buff.data());
        EXPECT_EQ(header->signature, LOCAL_HEADER_SIGNATURE);

        ret = entry->EncodeLocalFileHeader(buff.data(), buff.size(), true, nameLen);
        EXPECT_EQ(ret, PKG_SUCCESS);

        ret = entry->EncodeLocalFileHeader(buff.data(), 1, true, nameLen);
        EXPECT_EQ(ret, PKG_INVALID_PARAM);
    }

    void TestZipFileEntryEncodeDataDescriptor()
    {
        uint32_t zipNodeId = TEST_ZIP_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<ZipFileEntry> entry = std::make_unique<ZipFileEntry>(file.get(), zipNodeId);

        entry->crc32_ = TEST_CRC32_VALUE;
        entry->fileInfo_.fileInfo.packedSize = TEST_PACKED_SIZE_1K;
        entry->fileInfo_.fileInfo.unpackedSize = TEST_FILE_SIZE_2K;

        std::unique_ptr<MemoryMapStream> stream = std::make_unique<MemoryMapStream>(
            pkgManager_, "test_desc.bin", PkgBuffer(TEST_BUFFER_SIZE_1K), PkgStream::PkgStreamType_Write);

        uint32_t encodeLen = 0;
        int32_t ret = entry->EncodeDataDescriptor(stream.get(), 0, encodeLen);
        EXPECT_EQ(ret, PKG_SUCCESS);
        EXPECT_EQ(encodeLen, sizeof(DataDescriptor));
    }

    void TestZipFileEntryPackStream()
    {
        uint32_t zipNodeId = TEST_ZIP_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<ZipFileEntry> entry = std::make_unique<ZipFileEntry>(file.get(), zipNodeId);

        entry->fileInfo_.fileInfo.identity = "test_pack";
        entry->fileInfo_.method = Z_DEFLATED;
        entry->fileInfo_.fileInfo.headerOffset = 0;
        entry->fileInfo_.fileInfo.unpackedSize = TEST_FILE_SIZE_1K;

        std::unique_ptr<MemoryMapStream> inStream = std::make_unique<MemoryMapStream>(
            pkgManager_, "test_in.bin", PkgBuffer(TEST_BUFFER_SIZE_1K), PkgStream::PkgStreamType_Read);
        std::unique_ptr<MemoryMapStream> outStream = std::make_unique<MemoryMapStream>(
            pkgManager_, "test_out.bin", PkgBuffer(TEST_BUFFER_SIZE_4K), PkgStream::PkgStreamType_Write);

        PkgAlgorithm::PkgAlgorithmPtr algorithm = std::make_shared<PkgAlgorithm>();

        size_t encodeLen = 0;
        int32_t ret = entry->PackStream(inStream.get(), 0, encodeLen, algorithm, outStream.get());
        EXPECT_EQ(ret, PKG_SUCCESS);
        EXPECT_GT(encodeLen, 0);
    }

    void TestLz4FileEntryGetOriginalSize()
    {
        uint32_t nodeId = TEST_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<Lz4FileEntry> entry = std::make_unique<Lz4FileEntry>(file.get(), nodeId);

        entry->fileInfo_.fileInfo.unpackedSize = TEST_FILE_SIZE_2K;
        size_t originalSize = entry->GetOriginalSize();
        EXPECT_EQ(originalSize, TEST_FILE_SIZE_2K);
    }

    void TestLz4FileEntryGetEntryRange()
    {
        uint32_t nodeId = TEST_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<Lz4FileEntry> entry = std::make_unique<Lz4FileEntry>(file.get(), nodeId);

        auto range = entry->GetEntryRange();
        EXPECT_EQ(range.first, 0);
        EXPECT_EQ(range.second, 0);
    }

    void TestLz4FileEntryInit()
    {
        uint32_t nodeId = TEST_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<Lz4FileEntry> entry = std::make_unique<Lz4FileEntry>(file.get(), nodeId);

        Lz4FileInfo lz4Info {};
        lz4Info.fileInfo.identity = "test_lz4";
        lz4Info.fileInfo.unpackedSize = TEST_FILE_SIZE_1K;
        lz4Info.compressionLevel = TEST_COMPRESSION_LEVEL;
        lz4Info.blockIndependence = 0;
        lz4Info.blockSizeID = TEST_BLOCK_SIZE_ID;
        lz4Info.contentChecksumFlag = TEST_CONTENT_CHECKSUM_FLAG;

        std::unique_ptr<MemoryMapStream> stream = std::make_unique<MemoryMapStream>(
            pkgManager_, "test.bin", PkgBuffer(TEST_BUFFER_SIZE_1K), PkgStream::PkgStreamType_Read);

        int32_t ret = entry->Init(&lz4Info.fileInfo, stream.get());
        EXPECT_EQ(ret, PKG_SUCCESS);
    }

    void TestLz4FileEntryEnvelopHeader()
    {
        uint32_t nodeId = TEST_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<Lz4FileEntry> entry = std::make_unique<Lz4FileEntry>(file.get(), nodeId);

        entry->fileInfo_.fileInfo.headerOffset = TEST_HEADER_OFFSET_100;
        entry->fileInfo_.fileInfo.dataOffset = TEST_DATA_OFFSET_200;

        PkgManager::StreamPtr stream = nullptr;
        pkgManager_->CreatePkgStream(stream, "test.bin", TEST_STREAM_SIZE_1K, PkgStream::PkgStreamType_Read);
        ASSERT_NE(stream, nullptr);

        size_t encodeLen = 0;
        int32_t ret = entry->EncodeHeader(stream, TEST_HEADER_OFFSET_100, encodeLen);
        EXPECT_EQ(ret, PKG_SUCCESS);
        EXPECT_EQ(encodeLen, 0);
        EXPECT_EQ(entry->fileInfo_.fileInfo.headerOffset, TEST_HEADER_OFFSET_100);
    }

    void TestGZipFileEntryGetUpGradeCompInfo()
    {
        uint32_t nodeId = TEST_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<GZipFileEntry> entry = std::make_unique<GZipFileEntry>(file.get(), nodeId);

        entry->fileInfo_.fileInfo.identity = "test_gzip";
        entry->fileInfo_.fileInfo.modifiedTime = time(nullptr);

        std::vector<uint8_t> buffer(TEST_BUFFER_SIZE_1K);
        size_t offset = 0;
        PkgBuffer pkgBuffer(buffer);
        entry->GetUpGradeCompInfo(offset, pkgBuffer);

        EXPECT_GT(offset, sizeof(GZipHeader));
        GZipHeader* header = reinterpret_cast<GZipHeader*>(buffer.data());
        EXPECT_EQ(header->magic, GZIP_MAGIC);
    }

    void TestGZipFileEntryPack()
    {
        uint32_t nodeId = TEST_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<GZipFileEntry> entry = std::make_unique<GZipFileEntry>(file.get(), nodeId);

        entry->fileInfo_.fileInfo.identity = "test_gzip_pack";
        entry->fileInfo_.fileInfo.unpackedSize = TEST_FILE_SIZE_1K;
        entry->fileInfo_.fileInfo.dataOffset = TEST_DATA_OFFSET_100;

        PkgManager::StreamPtr inStream = nullptr;
        pkgManager_->CreatePkgStream(inStream, "test_in.bin", TEST_STREAM_SIZE_1K, PkgStream::PkgStreamType_Read);
        ASSERT_NE(inStream, nullptr);

        size_t encodeLen = 0;
        int32_t ret = entry->Pack(inStream, TEST_DATA_OFFSET_100, encodeLen);
        EXPECT_EQ(ret, PKG_SUCCESS);
    }

    void TestGZipFileEntryUnpack()
    {
        uint32_t nodeId = TEST_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<GZipFileEntry> entry = std::make_unique<GZipFileEntry>(file.get(), nodeId);

        entry->fileInfo_.fileInfo.identity = "test_gzip_unpack";
        entry->fileInfo_.fileInfo.packedSize = TEST_PACKED_SIZE_1K;
        entry->fileInfo_.fileInfo.headerOffset = 0;
        entry->fileInfo_.fileInfo.dataOffset = TEST_DATA_OFFSET_100;

        PkgManager::StreamPtr outStream = nullptr;
        pkgManager_->CreatePkgStream(outStream, "test_out.bin", TEST_STREAM_SIZE_2K, PkgStream::PkgStreamType_Write);
        ASSERT_NE(outStream, nullptr);

        int32_t ret = entry->Unpack(outStream);
        EXPECT_EQ(ret, PKG_SUCCESS);
    }

    void TestGZipFileEntryEncodeHeader()
    {
        uint32_t nodeId = TEST_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<GZipFileEntry> entry = std::make_unique<GZipFileEntry>(file.get(), nodeId);

        entry->fileInfo_.fileInfo.identity = "test_gzip_encode";

        PkgManager::StreamPtr stream = nullptr;
        pkgManager_->CreatePkgStream(stream, "test_encode.bin", TEST_STREAM_SIZE_2K, PkgStream::PkgStreamType_Write);
        ASSERT_NE(stream, nullptr);

        size_t encodeLen = 0;
        int32_t ret = entry->EncodeHeader(stream, 0, encodeLen);
        EXPECT_EQ(ret, PKG_SUCCESS);
    }

    void TestGZipFileEntryInit()
    {
        uint32_t nodeId = TEST_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<GZipFileEntry> entry = std::make_unique<GZipFileEntry>(file.get(), nodeId);

        entry->fileInfo_.fileInfo.identity = "test_gzip_init";
        entry->fileInfo_.fileInfo.unpackedSize = TEST_FILE_SIZE_1K;

        PkgManager::StreamPtr stream = nullptr;
        pkgManager_->CreatePkgStream(stream, "test_init.bin", TEST_STREAM_SIZE_1K, PkgStream::PkgStreamType_Read);
        ASSERT_NE(stream, nullptr);

        int32_t ret = entry->Init(&entry->fileInfo_.fileInfo, stream);
        EXPECT_EQ(ret, PKG_SUCCESS);
    }

    void TestGZipFileEntryInitWithFileInfo()
    {
        uint32_t nodeId = TEST_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<GZipFileEntry> entry = std::make_unique<GZipFileEntry>(file.get(), nodeId);

        FileInfo fileInfo {};
        fileInfo.identity = "test_gzip_info_init";
        fileInfo.unpackedSize = TEST_FILE_SIZE_2K;
        fileInfo.modifiedTime = time(nullptr);

        PkgManager::StreamPtr stream = nullptr;
        pkgManager_->CreatePkgStream(stream, "test_info_init.bin", TEST_STREAM_SIZE_2K, PkgStream::PkgStreamType_Read);
        ASSERT_NE(stream, nullptr);

        int32_t ret = entry->Init(&fileInfo, stream);
        EXPECT_EQ(ret, PKG_SUCCESS);
    }

    void TestGZipFileEntryEnvelopHeader()
    {
        uint32_t nodeId = TEST_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<GZipFileEntry> entry = std::make_unique<GZipFileEntry>(file.get(), nodeId);

        entry->fileInfo_.fileInfo.headerOffset = TEST_HEADER_OFFSET_100;
        entry->fileInfo_.fileInfo.dataOffset = TEST_DATA_OFFSET_200;

        PkgManager::StreamPtr stream = nullptr;
        pkgManager_->CreatePkgStream(stream, "test_envelop.bin", TEST_STREAM_SIZE_1K, PkgStream::PkgStreamType_Read);
        ASSERT_NE(stream, nullptr);

        size_t encodeLen = 0;
        int32_t ret = entry->EncodeHeader(stream, TEST_HEADER_OFFSET_100, encodeLen);
        EXPECT_EQ(ret, PKG_SUCCESS);
    }

    void TestGZipFileEntryGetOriginalSize()
    {
        uint32_t nodeId = TEST_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<GZipFileEntry> entry = std::make_unique<GZipFileEntry>(file.get(), nodeId);

        entry->fileInfo_.fileInfo.unpackedSize = TEST_FILE_SIZE_4K;
        entry->fileInfo_.fileInfo.packedSize = TEST_PACKED_SIZE_2K;

        size_t originalSize = entry->GetOriginalSize();
        EXPECT_EQ(originalSize, TEST_FILE_SIZE_4K);
    }

    void TestGZipFileEntryGetEntryRange()
    {
        uint32_t nodeId = TEST_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<GZipFileEntry> entry = std::make_unique<GZipFileEntry>(file.get(), nodeId);

        entry->fileInfo_.fileInfo.headerOffset = TEST_HEADER_OFFSET_100;
        entry->fileInfo_.fileInfo.dataOffset = TEST_DATA_OFFSET_200;
        entry->fileInfo_.fileInfo.packedSize = TEST_PACKED_SIZE_1K;

        auto range = entry->GetEntryRange();
        EXPECT_EQ(range.first, TEST_HEADER_OFFSET_100);
        EXPECT_GT(range.second, TEST_DATA_OFFSET_200);
    }

    void TestGZipFileEntryDecodeHeaderCalOffsetWithExtraField()
    {
        uint32_t nodeId = TEST_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<GZipFileEntry> entry = std::make_unique<GZipFileEntry>(file.get(), nodeId);

        std::vector<uint8_t> buffer(TEST_BUFFER_SIZE_1K);
        buffer[0] = 0;
        buffer[1] = 0;
        WriteLE16(buffer.data(), TEST_WRITE_VALUE_10);

        size_t offset = TEST_OFFSET_2;
        std::string fileName;
        entry->DecodeHeaderCalOffset(EXTRA_FIELD, PkgBuffer(buffer), offset, fileName);

        EXPECT_GT(offset, TEST_OFFSET_2);
    }

    void TestGZipFileEntryDecodeHeaderCalOffsetWithOrigName()
    {
        uint32_t nodeId = TEST_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<GZipFileEntry> entry = std::make_unique<GZipFileEntry>(file.get(), nodeId);

        std::vector<uint8_t> buffer(TEST_BUFFER_SIZE_1K);
        int32_t ret = memcpy_s(buffer.data() + TEST_OFFSET_2, TEST_BUFFER_SIZE_1K - TEST_OFFSET_2,
                               "test.txt", TEST_STRING_LEN_9);
        EXPECT_EQ(ret, EOK);

        size_t offset = TEST_OFFSET_2;
        std::string fileName;
        entry->DecodeHeaderCalOffset(ORIG_NAME, PkgBuffer(buffer), offset, fileName);

        EXPECT_EQ(fileName, "test.txt");
    }

    void TestGZipFileEntryDecodeHeaderCalOffsetWithHeaderCrc()
    {
        uint32_t nodeId = TEST_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<GZipFileEntry> entry = std::make_unique<GZipFileEntry>(file.get(), nodeId);

        std::vector<uint8_t> buffer(TEST_BUFFER_SIZE_1K);
        size_t offset = 0;
        std::string fileName;
        entry->DecodeHeaderCalOffset(HEADER_CRC, PkgBuffer(buffer), offset, fileName);

        EXPECT_EQ(offset, sizeof(uint16_t));
    }

    void TestGZipFileEntryCheckFileInfo()
    {
        uint32_t nodeId = TEST_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<GZipFileEntry> entry = std::make_unique<GZipFileEntry>(file.get(), nodeId);

        std::vector<uint8_t> data(TEST_BUFFER_SIZE_1K);
        WriteLE32(data.data(), 0x12345678);
        WriteLE32(data.data() + sizeof(uint32_t), TEST_WRITE_VALUE_1000);

        PkgAlgorithmContext context {{0, 0}, {0, 0}, 0, 0};
        context.crc = TEST_CRC32_VALUE;
        context.unpackedSize = TEST_UNPACKED_SIZE_1K;

        std::unique_ptr<MemoryMapStream> stream = std::make_unique<MemoryMapStream>(
            pkgManager_, "test_gzip.bin", PkgBuffer(data), PkgStream::PkgStreamType_Read);

        int32_t ret = entry->CheckFileInfo(context, stream.get());
        EXPECT_EQ(ret, PKG_SUCCESS);
    }

    void TestGZipFileEntryCheckFileInfoCrcMismatch()
    {
        uint32_t nodeId = TEST_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<GZipFileEntry> entry = std::make_unique<GZipFileEntry>(file.get(), nodeId);

        std::vector<uint8_t> data(TEST_BUFFER_SIZE_1K);
        WriteLE32(data.data(), TEST_CRC32_MISMATCH);
        WriteLE32(data.data() + sizeof(uint32_t), TEST_WRITE_VALUE_1000);

        PkgAlgorithmContext context {{0, 0}, {0, 0}, 0, 0};
        context.crc = TEST_CRC32_VALUE;
        context.unpackedSize = TEST_UNPACKED_SIZE_1K;

        // Use data vector to initialize stream buffer
        std::unique_ptr<MemoryMapStream> stream = std::make_unique<MemoryMapStream>(
            pkgManager_, "test_gzip.bin", PkgBuffer(data), PkgStream::PkgStreamType_Read);

        int32_t ret = entry->CheckFileInfo(context, stream.get());
        EXPECT_EQ(ret, PKG_VERIFY_FAIL);
    }

    void TestGZipFileEntryCheckFileInfoSizeMismatch()
    {
        uint32_t nodeId = TEST_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<GZipFileEntry> entry = std::make_unique<GZipFileEntry>(file.get(), nodeId);

        std::vector<uint8_t> data(TEST_BUFFER_SIZE_1K);
        WriteLE32(data.data(), 0x12345678);
        WriteLE32(data.data() + sizeof(uint32_t), TEST_WRITE_VALUE_2000);

        PkgAlgorithmContext context {{0, 0}, {0, 0}, 0, 0};
        context.crc = TEST_CRC32_VALUE;
        context.unpackedSize = TEST_UNPACKED_SIZE_1K;

        // Use data vector to initialize stream buffer
        std::unique_ptr<MemoryMapStream> stream = std::make_unique<MemoryMapStream>(
            pkgManager_, "test_gzip.bin", PkgBuffer(data), PkgStream::PkgStreamType_Read);

        int32_t ret = entry->CheckFileInfo(context, stream.get());
        EXPECT_EQ(ret, PKG_VERIFY_FAIL);
    }

    void TestPkgFileImplCheckStateBasic()
    {
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);

        std::vector<uint32_t> states = {PKG_FILE_STATE_IDLE};
        bool ret = file->CheckState(states, PKG_FILE_STATE_WORKING);
        EXPECT_TRUE(ret);

        states = {PKG_FILE_STATE_WORKING};
        ret = file->CheckState(states, PKG_FILE_STATE_CLOSE);
        EXPECT_TRUE(ret);
    }

    void TestPkgFileImplCheckStateInvalidState()
    {
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);

        std::vector<uint32_t> states = {PKG_FILE_STATE_CLOSE, PKG_FILE_STATE_WORKING};
        bool ret = file->CheckState(states, PKG_FILE_STATE_IDLE);
        EXPECT_FALSE(ret);
    }

    void TestPkgFileImplAddSignDataDigestTypeNone()
    {
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);

        size_t signOffset = 0;
        file->AddSignData(PKG_DIGEST_TYPE_NONE, TEST_DATA_OFFSET_100, signOffset);
        EXPECT_EQ(signOffset, TEST_HEADER_OFFSET_100);
    }

    void TestPkgFileImplConvertBufferToString()
    {
        std::string fileName;
        std::vector<uint8_t> buffer(TEST_VECTOR_SIZE_10);
        int32_t ret = memcpy_s(buffer.data(), TEST_VECTOR_SIZE_10, "test12345", TEST_STRING_LEN_9);
        EXPECT_EQ(ret, EOK);

        int32_t result = PkgFileImpl::ConvertBufferToString(fileName, PkgBuffer(buffer));
        EXPECT_EQ(result, PKG_SUCCESS);
        EXPECT_EQ(fileName, "test12345");
    }

    void TestPkgFileImplConvertBufferToStringWithNonPrintable()
    {
        std::string fileName;
        std::vector<uint8_t> buffer(TEST_VECTOR_SIZE_10);
        int32_t ret = memcpy_s(buffer.data(), TEST_VECTOR_SIZE_10, "test\x00test", TEST_STRING_LEN_9);
        EXPECT_EQ(ret, EOK);

        int32_t result = PkgFileImpl::ConvertBufferToString(fileName, PkgBuffer(buffer));
        EXPECT_EQ(result, PKG_SUCCESS);
        EXPECT_EQ(fileName, "test");
    }

    void TestPkgFileImplConvertStringToBuffer()
    {
        std::string fileName = "test_file";
        std::vector<uint8_t> buffer(TEST_VECTOR_SIZE_20);
        size_t realLen = 0;

        int32_t ret = PkgFileImpl::ConvertStringToBuffer(fileName, PkgBuffer(buffer), realLen);
        EXPECT_EQ(ret, PKG_SUCCESS);
        EXPECT_EQ(realLen, fileName.size());

        std::string result(reinterpret_cast<char*>(buffer.data()), realLen);
        EXPECT_EQ(result, fileName);
    }

    void TestPkgFileImplConvertStringToBufferBufferTooSmall()
    {
        std::string fileName = "test_file_long";
        std::vector<uint8_t> buffer(TEST_VECTOR_SIZE_5);
        size_t realLen = 0;

        int32_t ret = PkgFileImpl::ConvertStringToBuffer(fileName, PkgBuffer(buffer), realLen);
        EXPECT_EQ(ret, PKG_INVALID_PARAM);
    }

    void TestPkgFileImplExtractFileInvalidState()
    {
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);

        std::unique_ptr<MemoryMapStream> outStream = std::make_unique<MemoryMapStream>(
            pkgManager_, "out.bin", PkgBuffer(TEST_BUFFER_SIZE_1K), PkgStream::PkgStreamType_Write);

        int32_t ret = file->ExtractFile(nullptr, outStream.get());
        EXPECT_EQ(ret, PKG_INVALID_STATE);
    }

    void TestPkgFileImplFindPkgEntryInvalidState()
    {
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);

        PkgEntryPtr entry = file->FindPkgEntry("nonexistent");
        EXPECT_EQ(entry, nullptr);
    }

    void TestPkgFileImplAddPkgEntryInvalidType()
    {
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr, PkgFile::PKG_TYPE_MAX);

        PkgEntryPtr entry = file->AddPkgEntry("test_invalid");
        EXPECT_EQ(entry, nullptr);
    }

    void TestPkgEntryInitNullParams()
    {
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<Lz4FileEntry> entry = std::make_unique<Lz4FileEntry>(file.get(), TEST_ENTRY_NODE_ID);

        FileInfo fileInfo {};
        fileInfo.identity = "test";
        fileInfo.unpackedSize = TEST_FILE_SIZE_1K;

        std::unique_ptr<MemoryMapStream> stream = std::make_unique<MemoryMapStream>(
            pkgManager_, "test.bin", PkgBuffer(TEST_BUFFER_SIZE_1K), PkgStream::PkgStreamType_Read);

        int32_t ret = entry->Init(nullptr, stream.get());
        EXPECT_EQ(ret, PKG_INVALID_PARAM);

        ret = entry->Init(&fileInfo, nullptr);
        EXPECT_EQ(ret, PKG_INVALID_PARAM);
    }

    void TestPkgEntryInitValidParams()
    {
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<Lz4FileEntry> entry = std::make_unique<Lz4FileEntry>(file.get(), TEST_ENTRY_NODE_ID);

        FileInfo fileInfo {};
        fileInfo.identity = "test";
        fileInfo.unpackedSize = TEST_FILE_SIZE_1K;

        std::unique_ptr<MemoryMapStream> stream = std::make_unique<MemoryMapStream>(
            pkgManager_, "test.bin", PkgBuffer(TEST_BUFFER_SIZE_1K), PkgStream::PkgStreamType_Read);

        int32_t ret = entry->Init(&fileInfo, stream.get());
        EXPECT_EQ(ret, PKG_SUCCESS);
    }

    void TestPkgEntryInitUnpackedSizeZero()
    {
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<Lz4FileEntry> entry = std::make_unique<Lz4FileEntry>(file.get(), TEST_ENTRY_NODE_ID);

        FileInfo fileInfo {};
        fileInfo.identity = "test";
        fileInfo.unpackedSize = 0;

        std::unique_ptr<MemoryMapStream> stream = std::make_unique<MemoryMapStream>(
            pkgManager_, "test.bin", PkgBuffer(0), PkgStream::PkgStreamType_Read);

        int32_t ret = entry->Init(&fileInfo, stream.get());
        EXPECT_EQ(ret, PKG_INVALID_PARAM);
    }

    void TestUpgradeFileEntryInit()
    {
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<UpgradeFileEntry> entry = std::make_unique<UpgradeFileEntry>(file.get(), TEST_ENTRY_NODE_ID);

        ComponentInfo compInfo {};
        compInfo.fileInfo.identity = "comp_test";
        compInfo.fileInfo.unpackedSize = TEST_FILE_SIZE_2K;
        compInfo.version = "1.0.0";
        compInfo.id = TEST_COMPONENT_ID_1;
        compInfo.resType = TEST_RESOURCE_TYPE_1;
        compInfo.type = TEST_TYPE_1;
        compInfo.compFlags = 0;

        std::unique_ptr<MemoryMapStream> stream = std::make_unique<MemoryMapStream>(
            pkgManager_, "comp.bin", PkgBuffer(TEST_BUFFER_SIZE_2K), PkgStream::PkgStreamType_Read);

        int32_t ret = entry->Init(&compInfo.fileInfo, stream.get());
        EXPECT_EQ(ret, PKG_SUCCESS);
    }

    void TestUpgradeFileEntryGetUpGradeCompInfo()
    {
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<UpgradeFileEntry> entry = std::make_unique<UpgradeFileEntry>(file.get(), TEST_ENTRY_NODE_ID);

        entry->fileInfo_.fileInfo.identity = "upgrade_test";
        entry->fileInfo_.version = "2.0.0";
        entry->fileInfo_.id = TEST_COMPONENT_ID_2;
        entry->fileInfo_.resType = TEST_RESOURCE_TYPE_2;
        entry->fileInfo_.type = TEST_TYPE_2;
        entry->fileInfo_.compFlags = TEST_COMP_FLAGS_1;
        entry->fileInfo_.originalSize = TEST_FILE_SIZE_4K;
        (void)memset_s(entry->fileInfo_.digest, sizeof(entry->fileInfo_.digest), 0, DIGEST_MAX_LEN);

        UpgradeCompInfo comp {};
        int32_t ret = entry->GetUpGradeCompInfo(comp);
        EXPECT_EQ(ret, PKG_SUCCESS);
    }

    void TestUpgradeFileEntryGetOriginalSizeFoundInChunkInfo()
    {
        std::unique_ptr<UpgradePkgFile> pkgFile = std::make_unique<UpgradePkgFile>(
            pkgManager_, nullptr, nullptr);
        std::unique_ptr<UpgradeFileEntry> entry = std::make_unique<UpgradeFileEntry>(pkgFile.get(), TEST_ENTRY_NODE_ID);

        entry->fileInfo_.fileInfo.identity = "test_image";
        entry->fileInfo_.fileInfo.unpackedSize = TEST_FILE_SIZE_2K;
        entry->fileName_ = "test_image";

        pkgFile->chunkInfo_.imageSizeMap["test_image"] = TEST_FILE_SIZE_2K;

        size_t originalSize = entry->GetOriginalSize();
        EXPECT_EQ(originalSize, TEST_FILE_SIZE_2K);
    }

    void TestUpgradeFileEntryGetOriginalSizeNotFoundInChunkInfo()
    {
        std::unique_ptr<UpgradePkgFile> pkgFile = std::make_unique<UpgradePkgFile>(
            pkgManager_, nullptr, nullptr);
        std::unique_ptr<UpgradeFileEntry> entry = std::make_unique<UpgradeFileEntry>(pkgFile.get(), TEST_ENTRY_NODE_ID);

        entry->fileInfo_.fileInfo.identity = "test_image";
        entry->fileInfo_.fileInfo.unpackedSize = TEST_FILE_SIZE_1K;
        entry->fileName_ = "not_found_image";

        size_t originalSize = entry->GetOriginalSize();
        EXPECT_EQ(originalSize, TEST_FILE_SIZE_1K);
    }

    void TestUpgradeFileEntryGetEntryRange()
    {
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<UpgradeFileEntry> entry = std::make_unique<UpgradeFileEntry>(file.get(), TEST_ENTRY_NODE_ID);

        entry->dataOffset_ = TEST_DATA_OFFSET_100;
        entry->readOffset_ = 0;
        entry->fileInfo_.fileInfo.packedSize = TEST_PACKED_SIZE_500;

        auto range = entry->GetEntryRange();
        EXPECT_EQ(range.first, TEST_HEADER_OFFSET_100);
        EXPECT_EQ(range.second, TEST_RANGE_SECOND_600);
    }

    void TestStreamPkgFileGetPkgType()
    {
        std::unique_ptr<StreamPkgFile> streamFile = std::make_unique<StreamPkgFile>(
            pkgManager_, nullptr, nullptr, nullptr);

        auto pkgType = streamFile->GetPkgType();
        EXPECT_EQ(pkgType, TestFile::PKG_TYPE_STREAM);
    }

    void TestStreamPkgFileGetPkgEntryStream()
    {
        std::unique_ptr<StreamPkgFile> streamFile = std::make_unique<StreamPkgFile>(
            pkgManager_, nullptr, nullptr, nullptr);

        auto entryStream = streamFile->GetPkgEntryStream();
        EXPECT_EQ(entryStream, nullptr);
    }

    void TestStreamPkgFileSetUpradeEntryStreamNullEntry()
    {
        std::unique_ptr<StreamPkgFile> streamFile = std::make_unique<StreamPkgFile>(
            pkgManager_, nullptr, nullptr, nullptr);

        int32_t ret = streamFile->SetUpradeEntryStream(nullptr);
        EXPECT_EQ(ret, PKG_INVALID_PARAM);
    }

    void TestPackagesInfoSplitStringBasic()
    {
        std::string str = "line1\nline2\nline3";
        std::string pattern = "\n";
        std::vector<std::string> ret = Updater::Utils::SplitString(str, pattern);
        EXPECT_EQ(ret.size(), TEST_SPLIT_RESULT_SIZE);
    }

    void TestPackagesInfoSplitStringSingleChar()
    {
        std::string str = "a b c";
        std::string pattern = " ";
        std::vector<std::string> ret = Updater::Utils::SplitString(str, pattern);
        EXPECT_EQ(ret.size(), TEST_SPLIT_RESULT_SIZE);
    }

    void TestPackagesInfoSplitStringWithNewlines()
    {
        std::string str = "line1\nline2\nline3";
        std::string pattern = "\n";
        std::vector<std::string> ret = Updater::Utils::SplitString(str, pattern);
        EXPECT_EQ(ret.size(), TEST_SPLIT_RESULT_SIZE);
    }

    void TestPackagesInfoSplitStringLeadingTrailingSpaces()
    {
        std::string str = "  test  string  ";
        std::string pattern = " ";
        std::vector<std::string> ret = Updater::Utils::SplitString(str, pattern);
        EXPECT_GE(ret.size(), 0);
    }

    void TestGZipFileEntryEncodeHeaderNullOutStream()
    {
        uint32_t nodeId = TEST_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<GZipFileEntry> entry = std::make_unique<GZipFileEntry>(file.get(), nodeId);

        entry->fileInfo_.fileInfo.identity = "test_gzip_encode_null";

        size_t encodeLen = 0;
        int32_t ret = entry->EncodeHeader(nullptr, 0, encodeLen);
        EXPECT_NE(ret, PKG_SUCCESS);
    }

    void TestGZipFileEntryPackNullAlgorithm()
    {
        uint32_t nodeId = TEST_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<GZipFileEntry> entry = std::make_unique<GZipFileEntry>(file.get(), nodeId);

        entry->fileInfo_.fileInfo.identity = "test_gzip_pack_null";
        entry->fileInfo_.fileInfo.unpackedSize = TEST_FILE_SIZE_1K;
        entry->fileInfo_.fileInfo.dataOffset = TEST_DATA_OFFSET_100;

        size_t encodeLen = 0;
        int32_t ret = entry->Pack(nullptr, TEST_DATA_OFFSET_100, encodeLen);
        EXPECT_NE(ret, PKG_SUCCESS);
    }

    void TestGZipFileEntryUnpackNullAlgorithm()
    {
        uint32_t nodeId = TEST_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<GZipFileEntry> entry = std::make_unique<GZipFileEntry>(file.get(), nodeId);

        entry->fileInfo_.fileInfo.identity = "test_gzip_unpack_null";
        entry->fileInfo_.fileInfo.packedSize = TEST_PACKED_SIZE_1K;
        entry->fileInfo_.fileInfo.headerOffset = 0;
        entry->fileInfo_.fileInfo.dataOffset = TEST_DATA_OFFSET_100;

        int32_t ret = entry->Unpack(nullptr);
        EXPECT_NE(ret, PKG_SUCCESS);
    }

    void TestGZipPkgFileAddEntryNullParams()
    {
        std::unique_ptr<GZipPkgFile> gzipFile = std::make_unique<GZipPkgFile>(pkgManager_, nullptr);

        int32_t ret = gzipFile->AddEntry(nullptr, nullptr);
        EXPECT_EQ(ret, PKG_INVALID_PARAM);
    }

    void TestLz4FileEntryPackNullParams()
    {
        uint32_t nodeId = TEST_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<Lz4FileEntry> entry = std::make_unique<Lz4FileEntry>(file.get(), nodeId);

        entry->fileInfo_.fileInfo.identity = "test_lz4_pack_null";
        entry->fileInfo_.fileInfo.headerOffset = 0;

        size_t encodeLen = 0;
        int32_t ret = entry->Pack(nullptr, 0, encodeLen);
        EXPECT_NE(ret, PKG_SUCCESS);
    }

    void TestLz4FileEntryUnpackNullParams()
    {
        uint32_t nodeId = TEST_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<Lz4FileEntry> entry = std::make_unique<Lz4FileEntry>(file.get(), nodeId);

        entry->fileInfo_.fileInfo.identity = "test_lz4_unpack_null";
        entry->fileInfo_.fileInfo.packedSize = TEST_PACKED_SIZE_1K;
        entry->fileInfo_.fileInfo.headerOffset = 0;
        entry->fileInfo_.fileInfo.dataOffset = TEST_DATA_OFFSET_100;

        int32_t ret = entry->Unpack(nullptr);
        EXPECT_NE(ret, PKG_SUCCESS);
    }

    void TestLz4PkgFileAddEntryNullParams()
    {
        std::unique_ptr<Lz4PkgFile> lz4File = std::make_unique<Lz4PkgFile>(pkgManager_, nullptr);

        int32_t ret = lz4File->AddEntry(nullptr, nullptr);
        EXPECT_EQ(ret, PKG_INVALID_PARAM);
    }

    void TestZipPkgFileAddEntryNullParams()
    {
        std::unique_ptr<ZipPkgFile> zipFile = std::make_unique<ZipPkgFile>(pkgManager_, nullptr);

        int32_t ret = zipFile->AddEntry(nullptr, nullptr);
        EXPECT_EQ(ret, PKG_INVALID_PARAM);
    }

    void TestZipPkgFileSavePackageInvalidState()
    {
        std::unique_ptr<ZipPkgFile> zipFile = std::make_unique<ZipPkgFile>(pkgManager_, nullptr);

        size_t signOffset = 0;
        int32_t ret = zipFile->SavePackage(signOffset);
        EXPECT_NE(ret, PKG_SUCCESS);
    }

    void TestZipFileEntryDecodeCentralDirEntryInvalidSignature()
    {
        uint32_t zipNodeId = TEST_ZIP_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<ZipFileEntry> entry = std::make_unique<ZipFileEntry>(file.get(), zipNodeId);

        std::vector<uint8_t> bufferData(sizeof(CentralDirEntry) + TEST_VECTOR_SIZE_10);
        WriteLE32(bufferData.data() + offsetof(CentralDirEntry, signature), TEST_INVALID_SIGNATURE);

        size_t decodeLen = 0;
        PkgBuffer buffer(bufferData);
        int32_t ret = entry->DecodeCentralDirEntry(nullptr, buffer, 0, decodeLen);
        EXPECT_NE(ret, PKG_SUCCESS);
    }

    void TestZipFileEntryDoDecodeCentralDirEntryWithZip64()
    {
        uint32_t zipNodeId = TEST_ZIP_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<ZipFileEntry> entry = std::make_unique<ZipFileEntry>(file.get(), zipNodeId);

        std::vector<uint8_t> bufferData(sizeof(CentralDirEntry) + TEST_VECTOR_SIZE_50);
        WriteLE16(bufferData.data() + offsetof(CentralDirEntry, compressionMethod), TEST_COMPRESSION_METHOD);
        WriteLE16(bufferData.data() + offsetof(CentralDirEntry, nameSize), TEST_VECTOR_SIZE_10);
        WriteLE16(bufferData.data() + offsetof(CentralDirEntry, extraSize), TEST_EXTRA_SIZE_28);
        int ret = memcpy_s(bufferData.data() + sizeof(CentralDirEntry), TEST_VECTOR_SIZE_50,
                           "testfile1", TEST_STRING_LEN_10);
        EXPECT_EQ(ret, EOK);

        uint8_t* extraData = bufferData.data() + sizeof(CentralDirEntry) + TEST_VECTOR_SIZE_10;
        WriteLE16(extraData, TEST_ENTRY_NODE_ID);
        WriteLE32(extraData + sizeof(uint32_t), TEST_WRITE_VALUE_1000);
        WriteLE32(extraData + sizeof(uint32_t), TEST_WRITE_VALUE_2000);
        WriteLE32(extraData + sizeof(uint32_t) + sizeof(uint32_t), TEST_WRITE_VALUE_3000);
        WriteLE32(extraData + TEST_EXTRA_DATA_OFFSET_20, TEST_WRITE_VALUE_4000);

        entry->fileInfo_.fileInfo.packedSize = UINT_MAX;
        entry->fileInfo_.fileInfo.unpackedSize = UINT_MAX;
        entry->fileInfo_.fileInfo.headerOffset = UINT_MAX;

        size_t decodeLen = 0;
        PkgBuffer buffer(bufferData);
        int32_t ret2 = entry->DoDecodeCentralDirEntry(buffer, decodeLen, 46, 10, 28);
        EXPECT_EQ(ret2, PKG_SUCCESS);
    }

    void TestStreamPkgFileSetUpradeEntryStreamValidEntry()
    {
        std::unique_ptr<StreamPkgFile> streamFile = std::make_unique<StreamPkgFile>(
            pkgManager_, nullptr, nullptr, nullptr);

        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<UpgradeFileEntry> entry = std::make_unique<UpgradeFileEntry>(file.get(), TEST_ENTRY_NODE_ID);

        int32_t ret = streamFile->SetUpradeEntryStream(entry.get());
        EXPECT_EQ(ret, PKG_INVALID_PARAM);
    }

    void TestStreamPkgFileGetPkgEntryStreamNullStream()
    {
        std::unique_ptr<StreamPkgFile> streamFile = std::make_unique<StreamPkgFile>(
            pkgManager_, nullptr, nullptr, nullptr);

        auto stream = streamFile->GetPkgEntryStream();
        EXPECT_EQ(stream, nullptr);
    }

    void TestUpgradePkgFileGetUpgradeSignatureLen()
    {
        std::unique_ptr<UpgradePkgFile> upgradeFile = std::make_unique<UpgradePkgFile>(
            pkgManager_, nullptr, nullptr);

        size_t len = upgradeFile->GetUpgradeSignatureLen();
        EXPECT_GT(len, 0);
    }

    void TestUpgradePkgFileGetDigestLen()
    {
        std::unique_ptr<UpgradePkgFile> upgradeFile = std::make_unique<UpgradePkgFile>(
            pkgManager_, nullptr, nullptr);

        size_t len = upgradeFile->GetDigestLen();
        EXPECT_EQ(len, 0);
    }

    void TestUpgradePkgFileGetPkgInfo()
    {
        std::unique_ptr<UpgradePkgFile> upgradeFile = std::make_unique<UpgradePkgFile>(
            pkgManager_, nullptr, nullptr);

        auto info = upgradeFile->GetPkgInfo();
        EXPECT_NE(info, nullptr);
    }

    void TestUpgradePkgFileGetPkgType()
    {
        std::unique_ptr<UpgradePkgFile> upgradeFile = std::make_unique<UpgradePkgFile>(
            pkgManager_, nullptr, nullptr);

        auto type = upgradeFile->GetPkgType();
        EXPECT_EQ(type, PkgFile::PKG_TYPE_UPGRADE);
    }

    void TestUpgradePkgFileGetUpgradeFileVer()
    {
        std::unique_ptr<UpgradePkgFile> upgradeFile = std::make_unique<UpgradePkgFile>(
            pkgManager_, nullptr, nullptr);

        auto ver = upgradeFile->GetUpgradeFileVer();
        EXPECT_GE(ver, 0);
    }

    void TestUpgradePkgFileGetUpgradeChunkInfo()
    {
        std::unique_ptr<UpgradePkgFile> upgradeFile = std::make_unique<UpgradePkgFile>(
            pkgManager_, nullptr, nullptr);

        const auto& chunkInfo = upgradeFile->GetUpgradeChunkInfo();
        EXPECT_EQ(chunkInfo.partitionNum, 0);
    }

    void TestUpgradePkgFileGetImgHashData()
    {
        std::unique_ptr<UpgradePkgFile> upgradeFile = std::make_unique<UpgradePkgFile>(
            pkgManager_, nullptr, nullptr);

        auto hashData = upgradeFile->GetImgHashData();
        EXPECT_EQ(hashData, nullptr);
    }

    void TestUpgradePkgFileGetPkgMgr()
    {
        std::unique_ptr<UpgradePkgFile> upgradeFile = std::make_unique<UpgradePkgFile>(
            pkgManager_, nullptr, nullptr);

        auto mgr = upgradeFile->GetPkgMgr();
        EXPECT_NE(mgr, nullptr);
    }

    void TestUpgradeFileEntryVerifyNullHashData()
    {
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<UpgradeFileEntry> entry = std::make_unique<UpgradeFileEntry>(file.get(), TEST_ENTRY_NODE_ID);

        entry->fileName_ = "test_verify";
        entry->dataOffset_ = TEST_DATA_OFFSET_100;
        entry->fileInfo_.fileInfo.packedSize = TEST_PACKED_SIZE_1K;

        std::vector<uint8_t> bufferData(TEST_BUFFER_SIZE_1K);
        PkgBuffer buffer(bufferData);
        int32_t ret = entry->Verify(buffer, TEST_BUFFER_SIZE_1K, TEST_DATA_OFFSET_100);
        EXPECT_EQ(ret, PKG_SUCCESS);
    }

    void TestUpgradeFileEntryEncodeHeaderNullParams()
    {
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<UpgradeFileEntry> entry = std::make_unique<UpgradeFileEntry>(file.get(), TEST_ENTRY_NODE_ID);

        entry->fileInfo_.fileInfo.identity = "test_encode_null";
        entry->fileInfo_.version = "1.0.0";
        entry->fileInfo_.id = 1;
        entry->fileInfo_.type = 1;
        entry->fileInfo_.compFlags = 0;
        entry->fileInfo_.originalSize = TEST_BUFFER_SIZE_1K;

        size_t encodeLen = 0;
        int32_t ret = entry->EncodeHeader(nullptr, 0, encodeLen);
        EXPECT_NE(ret, PKG_SUCCESS);
    }

    void TestUpgradeFileEntryPackNullParams()
    {
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<UpgradeFileEntry> entry = std::make_unique<UpgradeFileEntry>(file.get(), TEST_ENTRY_NODE_ID);

        entry->fileInfo_.fileInfo.identity = "test_pack_null";
        entry->fileInfo_.fileInfo.unpackedSize = TEST_FILE_SIZE_1K;
        entry->fileInfo_.fileInfo.packedSize = TEST_PACKED_SIZE_512;
        (void)memset_s(entry->fileInfo_.digest, sizeof(entry->fileInfo_.digest), 0, DIGEST_MAX_LEN);

        size_t encodeLen = 0;
        int32_t result = entry->Pack(nullptr, 0, encodeLen);
        EXPECT_NE(result, PKG_SUCCESS);
    }

    void TestUpgradeFileEntryDecodeHeaderBufferTooSmall()
    {
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<UpgradeFileEntry> entry = std::make_unique<UpgradeFileEntry>(file.get(), TEST_ENTRY_NODE_ID);

        std::vector<uint8_t> bufferData(TEST_VECTOR_SIZE_10);
        PkgBuffer buffer(bufferData);
        size_t decodeLen = 0;
        int32_t ret = entry->DecodeHeader(buffer, 0, 0, decodeLen);
        EXPECT_NE(ret, PKG_SUCCESS);
    }

    void TestUpgradeFileEntryUnpackNullParams()
    {
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<UpgradeFileEntry> entry = std::make_unique<UpgradeFileEntry>(file.get(), TEST_ENTRY_NODE_ID);

        entry->fileInfo_.fileInfo.identity = "test_unpack_null";
        entry->fileInfo_.fileInfo.packedSize = TEST_PACKED_SIZE_1K;
        (void)memset_s(entry->fileInfo_.digest, sizeof(entry->fileInfo_.digest), 0, DIGEST_MAX_LEN);

        int32_t result = entry->Unpack(nullptr);
        EXPECT_NE(result, PKG_SUCCESS);
    }

    void TestUpgradePkgFileAddEntryNullParams()
    {
        std::unique_ptr<UpgradePkgFile> upgradeFile = std::make_unique<UpgradePkgFile>(
            pkgManager_, nullptr, nullptr);

        int32_t ret = upgradeFile->AddEntry(nullptr, nullptr);
        EXPECT_EQ(ret, PKG_INVALID_PARAM);
    }

    void TestUpgradePkgFileLoadPackageNullVerifier()
    {
        std::unique_ptr<UpgradePkgFile> upgradeFile = std::make_unique<UpgradePkgFile>(
            pkgManager_, nullptr, nullptr);

        std::vector<std::string> fileNames;
        int32_t ret = upgradeFile->LoadPackage(fileNames, nullptr);
        EXPECT_EQ(ret, PKG_INVALID_SIGNATURE);
    }

    void TestUpgradePkgFileLoadPackageInvalidState()
    {
        std::unique_ptr<UpgradePkgFile> upgradeFile = std::make_unique<UpgradePkgFile>(
            pkgManager_, nullptr, nullptr);

        std::vector<std::string> fileNames;
        int32_t ret = upgradeFile->LoadPackage(fileNames, nullptr);
        EXPECT_NE(ret, PKG_SUCCESS);
    }

    void TestPkgFileImplExtractFileNullEntry()
    {
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr, PkgFile::PKG_TYPE_UPGRADE);

        std::unique_ptr<MemoryMapStream> outStream = std::make_unique<MemoryMapStream>(
            pkgManager_, "out.bin", PkgBuffer(TEST_BUFFER_SIZE_1K), PkgStream::PkgStreamType_Write);

        int32_t ret = file->ExtractFile(nullptr, outStream.get());
        EXPECT_NE(ret, PKG_SUCCESS);
    }

    void TestPkgFileImplFindPkgEntryValid()
    {
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);

        PkgEntryPtr entry = file->AddPkgEntry("test_valid");
        EXPECT_NE(entry, nullptr);

        PkgEntryPtr foundEntry = file->FindPkgEntry("test_valid");
        EXPECT_EQ(foundEntry, nullptr);
    }

    void TestPkgFileImplAddPkgEntryUpgrade()
    {
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);

        PkgEntryPtr entry = file->AddPkgEntry("test_upgrade");
        EXPECT_NE(entry, nullptr);
    }

    void TestPkgFileImplAddPkgEntryZip()
    {
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr, PkgFile::PKG_TYPE_ZIP);

        PkgEntryPtr entry = file->AddPkgEntry("test_zip");
        EXPECT_NE(entry, nullptr);
    }

    void TestPkgFileImplAddPkgEntryLz4()
    {
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr, PkgFile::PKG_TYPE_LZ4);

        PkgEntryPtr entry = file->AddPkgEntry("test_lz4");
        EXPECT_NE(entry, nullptr);
    }

    void TestPkgFileImplAddPkgEntryGzip()
    {
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr, PkgFile::PKG_TYPE_GZIP);

        PkgEntryPtr entry = file->AddPkgEntry("test_gzip");
        EXPECT_NE(entry, nullptr);
    }

    void TestPackagesInfoGetPackagesInfoInstance()
    {
        PackagesInfoPtr info1 = PackagesInfo::GetPackagesInfoInstance();
        EXPECT_NE(info1, nullptr);

        PackagesInfo::ReleasePackagesInfoInstance(info1);
    }

    void TestPackagesInfoReleasePackagesInfoInstanceMultiple()
    {
        PackagesInfoPtr info1 = PackagesInfo::GetPackagesInfoInstance();
        EXPECT_NE(info1, nullptr);

        PackagesInfo::ReleasePackagesInfoInstance(info1);

        PackagesInfoPtr info2 = PackagesInfo::GetPackagesInfoInstance();
        EXPECT_NE(info2, nullptr);
    }

    void TestUpgradeFileEntryUnpackValidStream()
    {
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<UpgradeFileEntry> entry = std::make_unique<UpgradeFileEntry>(file.get(), TEST_ENTRY_NODE_ID);

        entry->fileInfo_.fileInfo.identity = "test_unpack_valid";
        entry->fileInfo_.fileInfo.packedSize = TEST_PACKED_SIZE_1K;
        entry->fileInfo_.fileInfo.headerOffset = 0;
        entry->fileInfo_.fileInfo.dataOffset = TEST_DATA_OFFSET_100;
        (void)memset_s(entry->fileInfo_.digest, sizeof(entry->fileInfo_.digest), 0, DIGEST_MAX_LEN);

        int32_t result = entry->Unpack(nullptr);
        EXPECT_NE(result, PKG_SUCCESS);
    }

    void TestUpgradeFileEntryDecodeHeaderNullBuffer()
    {
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<UpgradeFileEntry> entry = std::make_unique<UpgradeFileEntry>(file.get(), TEST_ENTRY_NODE_ID);

        std::vector<uint8_t> bufferData(TEST_VECTOR_SIZE_10);
        PkgBuffer buffer(bufferData);
        size_t decodeLen = 0;
        int32_t ret = entry->DecodeHeader(buffer, 0, 0, decodeLen);
        EXPECT_NE(ret, PKG_SUCCESS);
    }

    void TestZipFileEntryDecodeHeaderNullBuffer()
    {
        uint32_t zipNodeId = TEST_ZIP_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<ZipFileEntry> entry = std::make_unique<ZipFileEntry>(file.get(), zipNodeId);

        PkgBuffer buffer(nullptr, 0);
        size_t decodeLen = 0;
        int32_t ret = entry->DecodeHeader(buffer, 0, 0, decodeLen);
        EXPECT_NE(ret, PKG_SUCCESS);
    }

    void TestGZipFileEntryPackValidStream()
    {
        uint32_t nodeId = TEST_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<GZipFileEntry> entry = std::make_unique<GZipFileEntry>(file.get(), nodeId);

        entry->fileInfo_.fileInfo.identity = "test_gzip_pack_valid";
        entry->fileInfo_.fileInfo.unpackedSize = TEST_FILE_SIZE_1K;
        entry->fileInfo_.fileInfo.dataOffset = TEST_DATA_OFFSET_100;

        size_t encodeLen = 0;
        int32_t ret = entry->Pack(nullptr, TEST_DATA_OFFSET_100, encodeLen);
        EXPECT_NE(ret, PKG_SUCCESS);
    }

    void TestGZipFileEntryUnpackValidStream()
    {
        uint32_t nodeId = TEST_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<GZipFileEntry> entry = std::make_unique<GZipFileEntry>(file.get(), nodeId);

        entry->fileInfo_.fileInfo.identity = "test_gzip_unpack_valid";
        entry->fileInfo_.fileInfo.packedSize = TEST_PACKED_SIZE_1K;
        entry->fileInfo_.fileInfo.headerOffset = 0;
        entry->fileInfo_.fileInfo.dataOffset = TEST_DATA_OFFSET_100;

        int32_t ret = entry->Unpack(nullptr);
        EXPECT_NE(ret, PKG_SUCCESS);
    }

    void TestLz4FileEntryPackNullStream()
    {
        uint32_t nodeId = TEST_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<Lz4FileEntry> entry = std::make_unique<Lz4FileEntry>(file.get(), nodeId);

        entry->fileInfo_.fileInfo.identity = "test_lz4_pack_null";
        entry->fileInfo_.fileInfo.unpackedSize = TEST_FILE_SIZE_1K;

        size_t encodeLen = 0;
        int32_t ret = entry->Pack(nullptr, 0, encodeLen);
        EXPECT_NE(ret, PKG_SUCCESS);
    }

    void TestLz4FileEntryUnpackNullStream()
    {
        uint32_t nodeId = TEST_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<Lz4FileEntry> entry = std::make_unique<Lz4FileEntry>(file.get(), nodeId);

        entry->fileInfo_.fileInfo.identity = "test_lz4_unpack_null";
        entry->fileInfo_.fileInfo.packedSize = TEST_PACKED_SIZE_1K;
        entry->fileInfo_.fileInfo.headerOffset = 0;
        entry->fileInfo_.fileInfo.dataOffset = TEST_DATA_OFFSET_100;

        int32_t ret = entry->Unpack(nullptr);
        EXPECT_NE(ret, PKG_SUCCESS);
    }

    void TestLz4FileEntryUnpackValidStream()
    {
        uint32_t nodeId = TEST_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<Lz4FileEntry> entry = std::make_unique<Lz4FileEntry>(file.get(), nodeId);

        entry->fileInfo_.fileInfo.identity = "test_lz4_unpack_valid";
        entry->fileInfo_.fileInfo.packedSize = TEST_PACKED_SIZE_1K;
        entry->fileInfo_.fileInfo.headerOffset = 0;
        entry->fileInfo_.fileInfo.dataOffset = TEST_DATA_OFFSET_100;

        PkgManager::StreamPtr outStream = nullptr;
        pkgManager_->CreatePkgStream(outStream, "test_lz4_out.bin", TEST_STREAM_SIZE_2K,
                                     PkgStream::PkgStreamType_Write);
        ASSERT_NE(outStream, nullptr);

        int32_t ret = entry->Unpack(outStream);
        EXPECT_NE(ret, PKG_SUCCESS);
    }

    void TestGZipFileEntryDecodeHeaderBufferTooSmall()
    {
        uint32_t nodeId = TEST_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<GZipFileEntry> entry = std::make_unique<GZipFileEntry>(file.get(), nodeId);

        std::vector<uint8_t> bufferData(TEST_ENTRY_NODE_ID);
        PkgBuffer buffer(bufferData);
        size_t decodeLen = 0;
        int32_t ret = entry->DecodeHeader(buffer, 0, 0, decodeLen);
        EXPECT_NE(ret, PKG_SUCCESS);
    }

    void TestGZipFileEntryEnvelopHeaderNullStream()
    {
        uint32_t nodeId = TEST_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<GZipFileEntry> entry = std::make_unique<GZipFileEntry>(file.get(), nodeId);

        entry->fileInfo_.fileInfo.headerOffset = TEST_HEADER_OFFSET_100;
        entry->fileInfo_.fileInfo.dataOffset = TEST_DATA_OFFSET_200;

        size_t encodeLen = 0;
        int32_t ret = entry->EncodeHeader(nullptr, TEST_DATA_OFFSET_200, encodeLen);
        EXPECT_NE(ret, PKG_SUCCESS);
    }

    void TestPkgFileImplSavePackageNullStream()
    {
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);

        size_t signOffset = 0;
        int32_t ret = file->SavePackage(signOffset);
        EXPECT_EQ(ret, PKG_SUCCESS);
    }

    void TestUpgradePkgFileReadImgHashDataFileInvalidType()
    {
        std::unique_ptr<UpgradePkgFile> upgradeFile = std::make_unique<UpgradePkgFile>(
            pkgManager_, nullptr, nullptr);

        int32_t ret = upgradeFile->ReadImgHashDataFile("invalid_type");
        EXPECT_EQ(ret, PKG_SUCCESS);
    }

    void TestUpgradeFileEntryGetUpGradeCompInfoEmptyFields()
    {
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<UpgradeFileEntry> entry = std::make_unique<UpgradeFileEntry>(file.get(), TEST_ENTRY_NODE_ID);

        entry->fileInfo_.fileInfo.identity = "upgrade_empty";
        entry->fileInfo_.version = "";
        entry->fileInfo_.id = 0;
        entry->fileInfo_.resType = 0;
        entry->fileInfo_.type = 0;
        entry->fileInfo_.compFlags = 0;
        entry->fileInfo_.originalSize = 0;
        (void)memset_s(entry->fileInfo_.digest, sizeof(entry->fileInfo_.digest), 0, DIGEST_MAX_LEN);

        UpgradeCompInfo comp {};
        int32_t ret = entry->GetUpGradeCompInfo(comp);
        EXPECT_EQ(ret, PKG_SUCCESS);
    }

    void TestGZipFileEntryGetUpGradeCompInfoWithVersion()
    {
        uint32_t nodeId = TEST_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<GZipFileEntry> entry = std::make_unique<GZipFileEntry>(file.get(), nodeId);

        entry->fileInfo_.fileInfo.identity = "test_gzip_version";
        entry->fileInfo_.fileInfo.modifiedTime = time(nullptr);

        std::vector<uint8_t> buffer(TEST_BUFFER_SIZE_1K);
        size_t offset = 0;
        PkgBuffer pkgBuffer(buffer);
        entry->GetUpGradeCompInfo(offset, pkgBuffer);

        EXPECT_GT(offset, 0);
    }

    void TestUpgradeFileEntryGetEntryRangeWithZeroOffset()
    {
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<UpgradeFileEntry> entry = std::make_unique<UpgradeFileEntry>(file.get(), TEST_ENTRY_NODE_ID);

        entry->dataOffset_ = 0;
        entry->readOffset_ = 0;
        entry->fileInfo_.fileInfo.packedSize = 0;

        auto range = entry->GetEntryRange();
        EXPECT_EQ(range.first, 0);
        EXPECT_EQ(range.second, 0);
    }

    void TestStreamPkgFileSetUpradeEntryStreamNullFile()
    {
        std::unique_ptr<StreamPkgFile> streamFile = std::make_unique<StreamPkgFile>(
            pkgManager_, nullptr, nullptr, nullptr);

        int32_t ret = streamFile->SetUpradeEntryStream(nullptr);
        EXPECT_EQ(ret, PKG_INVALID_PARAM);
    }

    void TestUpgradePkgFileGetUpgradeChunkInfoWithData()
    {
        std::unique_ptr<UpgradePkgFile> upgradeFile = std::make_unique<UpgradePkgFile>(
            pkgManager_, nullptr, nullptr);

        const auto& chunkInfo = upgradeFile->GetUpgradeChunkInfo();
        EXPECT_EQ(chunkInfo.partitionNum, 0);
    }

    void TestUpgradePkgFileGetUpgradeFileVerDefaultValue()
    {
        std::unique_ptr<UpgradePkgFile> upgradeFile = std::make_unique<UpgradePkgFile>(
            pkgManager_, nullptr, nullptr);

        auto ver = upgradeFile->GetUpgradeFileVer();
        EXPECT_GE(ver, 0);
    }

    void TestPkgFileImplAddSignDataSha256()
    {
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);

        size_t signOffset = 0;
        file->AddSignData(PKG_DIGEST_TYPE_SHA256, TEST_DATA_OFFSET_100, signOffset);
        EXPECT_EQ(signOffset, TEST_HEADER_OFFSET_100);
    }

    void TestPkgFileImplAddSignDataSha384()
    {
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);

        size_t signOffset = 0;
        file->AddSignData(PKG_DIGEST_TYPE_SHA384, TEST_DATA_OFFSET_200, signOffset);
        EXPECT_EQ(signOffset, TEST_DATA_OFFSET_200);
    }

    void TestZipFileEntryGetOriginalSizeWithUnpackedSize()
    {
        uint32_t zipNodeId = TEST_ZIP_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<ZipFileEntry> entry = std::make_unique<ZipFileEntry>(file.get(), zipNodeId);

        entry->fileInfo_.fileInfo.unpackedSize = TEST_FILE_SIZE_1K;
        entry->fileInfo_.fileInfo.packedSize = TEST_PACKED_SIZE_512;

        size_t originalSize = entry->GetOriginalSize();
        EXPECT_EQ(originalSize, TEST_FILE_SIZE_1K);
    }

    void TestGZipFileEntryGetOriginalSizeWithValidData()
    {
        uint32_t nodeId = TEST_NODE_ID;
        std::unique_ptr<TestFile> file = std::make_unique<TestFile>(pkgManager_, nullptr);
        std::unique_ptr<GZipFileEntry> entry = std::make_unique<GZipFileEntry>(file.get(), nodeId);

        entry->fileInfo_.fileInfo.unpackedSize = TEST_FILE_SIZE_2K;
        entry->fileInfo_.fileInfo.packedSize = TEST_PACKED_SIZE_1K;

        size_t originalSize = entry->GetOriginalSize();
        EXPECT_EQ(originalSize, TEST_FILE_SIZE_2K);
    }

    void TestPackagesInfoIsAllowRollbackDefault()
    {
        PkgManager::PkgManagerPtr manager = PkgManager::CreatePackageInstance();
        PackagesInfoPtr pkginfomanager = PackagesInfo::GetPackagesInfoInstance();

        bool ret = pkginfomanager->IsAllowRollback();
        EXPECT_EQ(ret, false);

        PackagesInfo::ReleasePackagesInfoInstance(pkginfomanager);
        PkgManager::ReleasePackageInstance(manager);
    }

    void TestUpgradePkgFileParseComponentsEmptyList()
    {
        std::unique_ptr<UpgradePkgFile> upgradeFile = std::make_unique<UpgradePkgFile>(
            pkgManager_, nullptr, nullptr);

        std::vector<std::string> fileNames;
        int32_t ret = upgradeFile->ParseComponents(fileNames);
        EXPECT_EQ(ret, PKG_SUCCESS);
    }

    void TestUpgradePkgFileGetComponentsEmptyList()
    {
        std::unique_ptr<UpgradePkgFile> upgradeFile = std::make_unique<UpgradePkgFile>(
            pkgManager_, nullptr, nullptr);

        const auto& components = upgradeFile->GetComponents();
        EXPECT_EQ(components.size(), 0);
    }

    void TestGZipPkgFileGetComponentsEmptyList()
    {
        std::unique_ptr<GZipPkgFile> gzipFile = std::make_unique<GZipPkgFile>(pkgManager_, nullptr);

        const auto& components = gzipFile->GetComponents();
        EXPECT_EQ(components.size(), 0);
    }

    void TestLz4PkgFileGetComponentsEmptyList()
    {
        std::unique_ptr<Lz4PkgFile> lz4File = std::make_unique<Lz4PkgFile>(pkgManager_, nullptr);

        const auto& components = lz4File->GetComponents();
        EXPECT_EQ(components.size(), 0);
    }

    void TestZipPkgFileGetComponentsEmptyList()
    {
        std::unique_ptr<ZipPkgFile> zipFile = std::make_unique<ZipPkgFile>(pkgManager_, nullptr);

        const auto& components = zipFile->GetComponents();
        EXPECT_EQ(components.size(), 0);
    }

    void TestUpgradePkgFileClearPkgStream()
    {
        std::unique_ptr<UpgradePkgFile> upgradeFile = std::make_unique<UpgradePkgFile>(
            pkgManager_, nullptr, nullptr);

        upgradeFile->ClearPkgStream();
        auto stream = upgradeFile->GetPkgStream();
        EXPECT_EQ(stream, nullptr);
    }

    void TestGZipPkgFileClearPkgStream()
    {
        std::unique_ptr<GZipPkgFile> gzipFile = std::make_unique<GZipPkgFile>(pkgManager_, nullptr);

        gzipFile->ClearPkgStream();
        auto stream = gzipFile->GetPkgStream();
        EXPECT_EQ(stream, nullptr);
    }
};

HWTEST_F(PkgPackageTest, TestUpdaterPreProcess, TestSize.Level1)
{
    PkgPackageTest test;
    EXPECT_EQ(0, test.TestUpdaterPreProcess());
}

HWTEST_F(PkgPackageTest, TestPackageInfoFail, TestSize.Level1)
{
    PkgPackageTest test;
    EXPECT_EQ(0, test.TestPackageInfoFail());
}

HWTEST_F(PkgPackageTest, TestPkgFile, TestSize.Level1)
{
    PkgPackageTest test;
    EXPECT_EQ(0, test.TestPkgFile());
}

HWTEST_F(PkgPackageTest, TestPkgFileInvalid, TestSize.Level1)
{
    PkgPackageTest test;
    EXPECT_EQ(0, test.TestPkgFileInvalid());
}

HWTEST_F(PkgPackageTest, TestBigZip, TestSize.Level1)
{
    PkgPackageTest test;
    EXPECT_EQ(0, test.TestBigZipEntry());
}

HWTEST_F(PkgPackageTest, ZipFileEntry_CombineTimeAndDate, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestZipFileEntryCombineTimeAndDate();
}

HWTEST_F(PkgPackageTest, ZipFileEntry_GetOriginalSize, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestZipFileEntryGetOriginalSize();
}

HWTEST_F(PkgPackageTest, ZipFileEntry_GetEntryRange, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestZipFileEntryGetEntryRange();
}

HWTEST_F(PkgPackageTest, ZipFileEntry_EncodeLocalFileHeader, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestZipFileEntryEncodeLocalFileHeader();
}

HWTEST_F(PkgPackageTest, ZipFileEntry_EncodeDataDescriptor, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestZipFileEntryEncodeDataDescriptor();
}

HWTEST_F(PkgPackageTest, ZipFileEntry_PackStream, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestZipFileEntryPackStream();
}

HWTEST_F(PkgPackageTest, Lz4FileEntry_GetOriginalSize, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestLz4FileEntryGetOriginalSize();
}

HWTEST_F(PkgPackageTest, Lz4FileEntry_GetEntryRange, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestLz4FileEntryGetEntryRange();
}

HWTEST_F(PkgPackageTest, Lz4FileEntry_Init, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestLz4FileEntryInit();
}

HWTEST_F(PkgPackageTest, Lz4FileEntry_Pack_NullParams, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestLz4FileEntryPackNullParams();
}

HWTEST_F(PkgPackageTest, GZipFileEntry_GetUpGradeCompInfo, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestGZipFileEntryGetUpGradeCompInfo();
}

HWTEST_F(PkgPackageTest, GZipFileEntry_DecodeHeaderCalOffset_WithExtraField, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestGZipFileEntryDecodeHeaderCalOffsetWithExtraField();
}

HWTEST_F(PkgPackageTest, GZipFileEntry_DecodeHeaderCalOffset_WithOrigName, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestGZipFileEntryDecodeHeaderCalOffsetWithOrigName();
}

HWTEST_F(PkgPackageTest, GZipFileEntry_DecodeHeaderCalOffset_WithHeaderCrc, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestGZipFileEntryDecodeHeaderCalOffsetWithHeaderCrc();
}

HWTEST_F(PkgPackageTest, GZipFileEntry_CheckFileInfo, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestGZipFileEntryCheckFileInfo();
}

HWTEST_F(PkgPackageTest, GZipFileEntry_CheckFileInfo_CrcMismatch, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestGZipFileEntryCheckFileInfoCrcMismatch();
}

HWTEST_F(PkgPackageTest, GZipFileEntry_CheckFileInfo_SizeMismatch, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestGZipFileEntryCheckFileInfoSizeMismatch();
}

HWTEST_F(PkgPackageTest, PkgFileImpl_CheckState_Basic, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestPkgFileImplCheckStateBasic();
}

HWTEST_F(PkgPackageTest, PkgFileImpl_CheckState_InvalidState, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestPkgFileImplCheckStateInvalidState();
}

HWTEST_F(PkgPackageTest, PkgFileImpl_AddSignData_DigestTypeNone, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestPkgFileImplAddSignDataDigestTypeNone();
}

HWTEST_F(PkgPackageTest, PkgFileImpl_ConvertBufferToString, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestPkgFileImplConvertBufferToString();
}

HWTEST_F(PkgPackageTest, PkgFileImpl_ConvertBufferToString_WithNonPrintable, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestPkgFileImplConvertBufferToStringWithNonPrintable();
}

HWTEST_F(PkgPackageTest, PkgFileImpl_ConvertStringToBuffer, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestPkgFileImplConvertStringToBuffer();
}

HWTEST_F(PkgPackageTest, PkgFileImpl_ConvertStringToBuffer_BufferTooSmall, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestPkgFileImplConvertStringToBufferBufferTooSmall();
}

HWTEST_F(PkgPackageTest, PkgFileImpl_ExtractFile_InvalidState, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestPkgFileImplExtractFileInvalidState();
}

HWTEST_F(PkgPackageTest, PkgFileImpl_FindPkgEntry_InvalidState, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestPkgFileImplFindPkgEntryInvalidState();
}

HWTEST_F(PkgPackageTest, PkgFileImpl_AddPkgEntry_InvalidType, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestPkgFileImplAddPkgEntryInvalidType();
}

HWTEST_F(PkgPackageTest, PkgEntry_Init_NullParams, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestPkgEntryInitNullParams();
}

HWTEST_F(PkgPackageTest, PkgEntry_Init_ValidParams, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestPkgEntryInitValidParams();
}

HWTEST_F(PkgPackageTest, PkgEntry_Init_UnpackedSizeZero, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestPkgEntryInitUnpackedSizeZero();
}

HWTEST_F(PkgPackageTest, UpgradeFileEntry_Init, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestUpgradeFileEntryInit();
}

HWTEST_F(PkgPackageTest, UpgradeFileEntry_GetUpGradeCompInfo, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestUpgradeFileEntryGetUpGradeCompInfo();
}

HWTEST_F(PkgPackageTest, UpgradeFileEntry_GetOriginalSize_FoundInChunkInfo, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestUpgradeFileEntryGetOriginalSizeFoundInChunkInfo();
}

HWTEST_F(PkgPackageTest, UpgradeFileEntry_GetOriginalSize_NotFoundInChunkInfo, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestUpgradeFileEntryGetOriginalSizeNotFoundInChunkInfo();
}

HWTEST_F(PkgPackageTest, UpgradeFileEntry_GetEntryRange, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestUpgradeFileEntryGetEntryRange();
}

HWTEST_F(PkgPackageTest, StreamPkgFile_GetPkgType, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestStreamPkgFileGetPkgType();
}

HWTEST_F(PkgPackageTest, StreamPkgFile_GetPkgEntryStream, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestStreamPkgFileGetPkgEntryStream();
}

HWTEST_F(PkgPackageTest, StreamPkgFile_SetUpradeEntryStream_NullEntry, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestStreamPkgFileSetUpradeEntryStreamNullEntry();
}

HWTEST_F(PkgPackageTest, PkgFileImpl_AddSignData_Sha256, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestPkgFileImplAddSignDataSha256();
}

HWTEST_F(PkgPackageTest, GZipFileEntry_EncodeHeader_NullOutStream, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestGZipFileEntryEncodeHeaderNullOutStream();
}

HWTEST_F(PkgPackageTest, GZipFileEntry_Pack_NullAlgorithm, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestGZipFileEntryPackNullAlgorithm();
}

HWTEST_F(PkgPackageTest, GZipFileEntry_Unpack_NullAlgorithm, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestGZipFileEntryUnpackNullAlgorithm();
}

HWTEST_F(PkgPackageTest, GZipPkgFile_AddEntry_NullParams, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestGZipPkgFileAddEntryNullParams();
}

HWTEST_F(PkgPackageTest, Lz4PkgFile_AddEntry_NullParams, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestLz4PkgFileAddEntryNullParams();
}

HWTEST_F(PkgPackageTest, ZipPkgFile_AddEntry_NullParams, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestZipPkgFileAddEntryNullParams();
}

HWTEST_F(PkgPackageTest, ZipPkgFile_SavePackage_InvalidState, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestZipPkgFileSavePackageInvalidState();
}

HWTEST_F(PkgPackageTest, ZipFileEntry_DecodeCentralDirEntry_InvalidSignature, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestZipFileEntryDecodeCentralDirEntryInvalidSignature();
}

HWTEST_F(PkgPackageTest, ZipFileEntry_DoDecodeCentralDirEntry_WithZip64, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestZipFileEntryDoDecodeCentralDirEntryWithZip64();
}

HWTEST_F(PkgPackageTest, StreamPkgFile_SetUpradeEntryStream_ValidEntry, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestStreamPkgFileSetUpradeEntryStreamValidEntry();
}

HWTEST_F(PkgPackageTest, StreamPkgFile_GetPkgEntryStream_NullStream, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestStreamPkgFileGetPkgEntryStreamNullStream();
}

HWTEST_F(PkgPackageTest, UpgradePkgFile_GetUpgradeSignatureLen, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestUpgradePkgFileGetUpgradeSignatureLen();
}

HWTEST_F(PkgPackageTest, UpgradePkgFile_GetDigestLen, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestUpgradePkgFileGetDigestLen();
}

HWTEST_F(PkgPackageTest, UpgradePkgFile_GetPkgInfo, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestUpgradePkgFileGetPkgInfo();
}

HWTEST_F(PkgPackageTest, UpgradePkgFile_GetPkgType, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestUpgradePkgFileGetPkgType();
}

HWTEST_F(PkgPackageTest, UpgradePkgFile_GetUpgradeFileVer, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestUpgradePkgFileGetUpgradeFileVer();
}

HWTEST_F(PkgPackageTest, UpgradePkgFile_GetUpgradeChunkInfo, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestUpgradePkgFileGetUpgradeChunkInfo();
}

HWTEST_F(PkgPackageTest, UpgradePkgFile_GetImgHashData, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestUpgradePkgFileGetImgHashData();
}

HWTEST_F(PkgPackageTest, UpgradePkgFile_GetPkgMgr, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestUpgradePkgFileGetPkgMgr();
}

HWTEST_F(PkgPackageTest, UpgradeFileEntry_Verify_NullHashData, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestUpgradeFileEntryVerifyNullHashData();
}

HWTEST_F(PkgPackageTest, UpgradeFileEntry_EncodeHeader_NullParams, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestUpgradeFileEntryEncodeHeaderNullParams();
}

HWTEST_F(PkgPackageTest, UpgradeFileEntry_Pack_NullParams, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestUpgradeFileEntryPackNullParams();
}

HWTEST_F(PkgPackageTest, UpgradeFileEntry_DecodeHeader_BufferTooSmall, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestUpgradeFileEntryDecodeHeaderBufferTooSmall();
}

HWTEST_F(PkgPackageTest, UpgradeFileEntry_Unpack_NullParams, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestUpgradeFileEntryUnpackNullParams();
}

HWTEST_F(PkgPackageTest, UpgradePkgFileAddEntryNullParams, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestUpgradePkgFileAddEntryNullParams();
}

HWTEST_F(PkgPackageTest, UpgradePkgFile_LoadPackage_NullVerifier, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestUpgradePkgFileLoadPackageNullVerifier();
}

HWTEST_F(PkgPackageTest, UpgradePkgFile_LoadPackage_InvalidState, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestUpgradePkgFileLoadPackageInvalidState();
}

HWTEST_F(PkgPackageTest, PkgFileImpl_ExtractFile_NullEntry, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestPkgFileImplExtractFileNullEntry();
}

HWTEST_F(PkgPackageTest, PkgFileImpl_FindPkgEntry_Valid, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestPkgFileImplFindPkgEntryValid();
}

HWTEST_F(PkgPackageTest, PkgFileImpl_AddPkgEntry_Upgrade, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestPkgFileImplAddPkgEntryUpgrade();
}

HWTEST_F(PkgPackageTest, PkgFileImpl_AddPkgEntry_Zip, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestPkgFileImplAddPkgEntryZip();
}

HWTEST_F(PkgPackageTest, PkgFileImpl_AddPkgEntry_Lz4, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestPkgFileImplAddPkgEntryLz4();
}

HWTEST_F(PkgPackageTest, PkgFileImpl_AddPkgEntry_Gzip, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestPkgFileImplAddPkgEntryGzip();
}

HWTEST_F(PkgPackageTest, PackagesInfo_GetPackagesInfoInstance, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestPackagesInfoGetPackagesInfoInstance();
}

HWTEST_F(PkgPackageTest, PackagesInfo_ReleasePackagesInfoInstance_Multiple, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestPackagesInfoReleasePackagesInfoInstanceMultiple();
}

HWTEST_F(PkgPackageTest, UpgradeFileEntry_Unpack_ValidStream, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestUpgradeFileEntryUnpackValidStream();
}

HWTEST_F(PkgPackageTest, UpgradeFileEntry_DecodeHeader_NullBuffer, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestUpgradeFileEntryDecodeHeaderNullBuffer();
}

HWTEST_F(PkgPackageTest, ZipFileEntry_DecodeHeader_NullBuffer, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestZipFileEntryDecodeHeaderNullBuffer();
}

HWTEST_F(PkgPackageTest, GZipFileEntry_Pack_ValidStream, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestGZipFileEntryPackValidStream();
}

HWTEST_F(PkgPackageTest, GZipFileEntry_Unpack_ValidStream, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestGZipFileEntryUnpackValidStream();
}

HWTEST_F(PkgPackageTest, Lz4FileEntry_Pack_ValidStream, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestLz4FileEntryPackNullStream();
}

HWTEST_F(PkgPackageTest, Lz4FileEntry_Unpack_NullStream, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestLz4FileEntryUnpackNullStream();
}

HWTEST_F(PkgPackageTest, Lz4FileEntry_Unpack_ValidStream, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestLz4FileEntryUnpackValidStream();
}

HWTEST_F(PkgPackageTest, GZipFileEntry_DecodeHeader_BufferTooSmall, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestGZipFileEntryDecodeHeaderBufferTooSmall();
}

HWTEST_F(PkgPackageTest, GZipFileEntry_EnvelopHeader_NullStream, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestGZipFileEntryEnvelopHeaderNullStream();
}

HWTEST_F(PkgPackageTest, PkgFileImpl_SavePackage_NullStream, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestPkgFileImplSavePackageNullStream();
}

HWTEST_F(PkgPackageTest, UpgradePkgFile_ReadImgHashDataFile_InvalidType, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestUpgradePkgFileReadImgHashDataFileInvalidType();
}

HWTEST_F(PkgPackageTest, UpgradeFileEntry_GetUpGradeCompInfo_EmptyFields, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestUpgradeFileEntryGetUpGradeCompInfoEmptyFields();
}

HWTEST_F(PkgPackageTest, GZipFileEntry_GetUpGradeCompInfo_WithVersion, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestGZipFileEntryGetUpGradeCompInfoWithVersion();
}

HWTEST_F(PkgPackageTest, UpgradeFileEntry_GetEntryRange_WithZeroOffset, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestUpgradeFileEntryGetEntryRangeWithZeroOffset();
}

HWTEST_F(PkgPackageTest, StreamPkgFile_SetUpradeEntryStream_NullFile, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestStreamPkgFileSetUpradeEntryStreamNullFile();
}

HWTEST_F(PkgPackageTest, UpgradePkgFile_GetUpgradeChunkInfo_WithData, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestUpgradePkgFileGetUpgradeChunkInfoWithData();
}

HWTEST_F(PkgPackageTest, UpgradePkgFile_GetUpgradeFileVer_DefaultValue, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestUpgradePkgFileGetUpgradeFileVerDefaultValue();
}

HWTEST_F(PkgPackageTest, ZipFileEntry_GetOriginalSize_WithUnpackedSize, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestZipFileEntryGetOriginalSizeWithUnpackedSize();
}

HWTEST_F(PkgPackageTest, GZipFileEntry_GetOriginalSize_WithValidData, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestGZipFileEntryGetOriginalSizeWithValidData();
}

HWTEST_F(PkgPackageTest, PackagesInfo_IsAllowRollback_Default, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestPackagesInfoIsAllowRollbackDefault();
}

HWTEST_F(PkgPackageTest, UpgradePkgFile_ParseComponents_EmptyList, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestUpgradePkgFileParseComponentsEmptyList();
}

HWTEST_F(PkgPackageTest, UpgradePkgFile_GetComponents_EmptyList, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestUpgradePkgFileGetComponentsEmptyList();
}

HWTEST_F(PkgPackageTest, GZipPkgFile_GetComponents_EmptyList, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestGZipPkgFileGetComponentsEmptyList();
}

HWTEST_F(PkgPackageTest, Lz4PkgFile_GetComponents_EmptyList, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestLz4PkgFileGetComponentsEmptyList();
}

HWTEST_F(PkgPackageTest, ZipPkgFile_GetComponents_EmptyList, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestZipPkgFileGetComponentsEmptyList();
}

HWTEST_F(PkgPackageTest, UpgradePkgFile_ClearPkgStream, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestUpgradePkgFileClearPkgStream();
}

HWTEST_F(PkgPackageTest, GZipPkgFile_ClearPkgStream, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestGZipPkgFileClearPkgStream();
}

HWTEST_F(PkgPackageTest, GZipFileEntry_EncodeHeader, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestGZipFileEntryEncodeHeader();
}

HWTEST_F(PkgPackageTest, GZipFileEntry_EnvelopHeader, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestGZipFileEntryEnvelopHeader();
}

HWTEST_F(PkgPackageTest, GZipFileEntry_GetEntryRange, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestGZipFileEntryGetEntryRange();
}

HWTEST_F(PkgPackageTest, GZipFileEntry_GetOriginalSize, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestGZipFileEntryGetOriginalSize();
}

HWTEST_F(PkgPackageTest, GZipFileEntry_Init, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestGZipFileEntryInit();
}

HWTEST_F(PkgPackageTest, GZipFileEntry_InitWithFileInfo, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestGZipFileEntryInitWithFileInfo();
}

HWTEST_F(PkgPackageTest, GZipFileEntry_Pack, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestGZipFileEntryPack();
}

HWTEST_F(PkgPackageTest, GZipFileEntry_Unpack, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestGZipFileEntryUnpack();
}

HWTEST_F(PkgPackageTest, Lz4FileEntry_EnvelopHeader, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestLz4FileEntryEnvelopHeader();
}

HWTEST_F(PkgPackageTest, Lz4FileEntry_Unpack_NullParams, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestLz4FileEntryUnpackNullParams();
}

HWTEST_F(PkgPackageTest, PackagesInfo_SplitString_Basic, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestPackagesInfoSplitStringBasic();
}

HWTEST_F(PkgPackageTest, PackagesInfo_SplitString_SingleChar, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestPackagesInfoSplitStringSingleChar();
}

HWTEST_F(PkgPackageTest, PackagesInfo_SplitString_WithNewlines, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestPackagesInfoSplitStringWithNewlines();
}

HWTEST_F(PkgPackageTest, PackagesInfo_SplitString_LeadingTrailingSpaces, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestPackagesInfoSplitStringLeadingTrailingSpaces();
}

HWTEST_F(PkgPackageTest, PkgFileImpl_AddSignData_Sha384, TestSize.Level1)
{
    PkgPackageTest test;
    test.TestPkgFileImplAddSignDataSha384();
}

}