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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <unistd.h>
#include <unittest_comm.h>
#include "utils.h"
#include "utils_common.h"

using namespace Updater;
using namespace Utils;
using namespace testing::ext;
using namespace std;

namespace UpdaterUt {
class UtilsUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void) {};
    static void TearDownTestCase(void) {};
    void SetUp() {};
    void TearDown() {};
};

void CreateFile(const std::string &filepath, size_t fileSize)
{
    const char fillChar = '0';
    std::ofstream outFile(filepath, std::ios::binary);
    if (!outFile) {
        std::cout << "create file fail: " << filepath << endl;
        return;
    }
    std::string data(fileSize, fillChar);
    outFile.write(data.c_str(), fileSize);
    outFile.close();
}

HWTEST_F(UtilsUnitTest, updater_utils_test_001, TestSize.Level0)
{
    string emptyStr = Utils::Trim("");
    EXPECT_STREQ(emptyStr.c_str(), "");
    emptyStr = Utils::Trim("   ");
    EXPECT_STREQ(emptyStr.c_str(), "");
    emptyStr = Utils::Trim("aa   ");
    EXPECT_STREQ(emptyStr.c_str(), "aa");
}

HWTEST_F(UtilsUnitTest, updater_utils_test_002, TestSize.Level0)
{
    uint8_t a[1] = {0};
    a[0] = 1;
    string newStr = Utils::ConvertSha256Hex(a, 1);
    EXPECT_STREQ(newStr.c_str(), "01");
}

HWTEST_F(UtilsUnitTest, updater_utils_test_003, TestSize.Level0)
{
    string str = "aaa\nbbb";
    vector<string> newStr = Utils::SplitString(str, "\n");
    EXPECT_EQ(newStr[0], "aaa");
    EXPECT_EQ(newStr[1], "bbb");
}

HWTEST_F(UtilsUnitTest, updater_utils_test_004, TestSize.Level0)
{
    EXPECT_EQ(Utils::MkdirRecursive("/data/xx?xx", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH), 0);
}

HWTEST_F(UtilsUnitTest, updater_utils_test_005, TestSize.Level0)
{
    string input = "";
    int output = Utils::String2Int<int>(input, 10);
    EXPECT_EQ(output, 0);
    input = "0x01";
    output = Utils::String2Int<int>(input, 10);
    EXPECT_EQ(output, 1);
}

HWTEST_F(UtilsUnitTest, updater_utils_test_006, TestSize.Level0)
{
    std::vector<std::string> files;
    string path = "/data/updater/log";
    Utils::SaveLogs();
    Utils::CompressLogs("/data/updater/log/updater_log_test");
    EXPECT_NE(Utils::GetFilesFromDirectory(path, files, true), -1);
}

HWTEST_F(UtilsUnitTest, RemoveDirTest, TestSize.Level0)
{
    string path = "";
    EXPECT_EQ(Utils::RemoveDir(path), false);
    path = TEST_PATH_FROM + "../utils/nonExistDir";
    EXPECT_EQ(Utils::RemoveDir(path), false);
    path = "/data/updater/rmDir";
    int ret = mkdir(path.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    if (ret == 0) {
        ofstream tmpFile;
        string filePath = path + "/tmpFile";
        tmpFile.open(filePath.c_str());
        if (tmpFile.is_open()) {
            tmpFile.close();
            EXPECT_EQ(Utils::RemoveDir(path), true);
        }
    }
}

HWTEST_F(UtilsUnitTest, IsUpdaterMode, TestSize.Level0)
{
    EXPECT_EQ(Utils::IsUpdaterMode(), false);
}

HWTEST_F(UtilsUnitTest, IsFileExist, TestSize.Level0)
{
    EXPECT_EQ(Utils::IsFileExist("/bin/test_updater"), false);
    EXPECT_EQ(Utils::IsFileExist("/data/updater/updater/etc/fstab.ut.updater"), true);
}

HWTEST_F(UtilsUnitTest, IsDirExist, TestSize.Level0)
{
    EXPECT_EQ(Utils::IsDirExist("/bin/test_updater"), false);
    EXPECT_EQ(Utils::IsDirExist("/bin"), true);
    EXPECT_EQ(Utils::IsDirExist("/bin/"), true);
}

HWTEST_F(UtilsUnitTest, CopyUpdaterLogsTest, TestSize.Level0)
{
    const std::string sLog = "/data/updater/main_data/updater.tab";
    const std::string dLog = "/data/updater/main_data/ut_dLog.txt";
    bool ret = Utils::CopyUpdaterLogs(sLog, dLog);
    EXPECT_EQ(ret, true);
    unlink(dLog.c_str());
}

HWTEST_F(UtilsUnitTest, GetDirSizeForFileTest, TestSize.Level0)
{
    const std::string testNoPath = "xxx";
    long long int ret = Utils::GetDirSizeForFile(testNoPath);
    EXPECT_EQ(ret, -1);
    const std::string testVaildPath = "xxx/xxx";
    ret = Utils::GetDirSizeForFile(testVaildPath);
    EXPECT_EQ(ret, 0);
    const std::string testPath = "/data/updater/updater/etc/fstab.ut.updater";
    ret = Utils::GetDirSizeForFile(testPath);
    EXPECT_EQ(ret, 827); // 827 : file size
}

HWTEST_F(UtilsUnitTest, GetTagValInStrTest, TestSize.Level0)
{
    const std::string tag = "abc";
    std::string ret = "";
    Utils::GetTagValInStr("", tag, ret);
    EXPECT_EQ(ret, "");
    Utils::GetTagValInStr("abcde=12", tag, ret);
    EXPECT_EQ(ret, "");
    Utils::GetTagValInStr("abc=12", tag, ret);
    EXPECT_EQ(ret, "12");
}

HWTEST_F(UtilsUnitTest, IsValidHexTest, TestSize.Level0)
{
    bool ret = Utils::IsValidHexStr("a1");
    EXPECT_EQ(ret, true);
    ret = Utils::IsValidHexStr("1*");
    EXPECT_EQ(ret, false);
    ret = Utils::IsValidHexStr("ABCDEF");
    EXPECT_EQ(ret, true);
}

HWTEST_F(UtilsUnitTest, TrimStringTest, TestSize.Level0)
{
    std::string ret = "abc";
    Utils::TrimString(ret);
    EXPECT_EQ(ret, "abc");
    ret = "abc\r\n";
    Utils::TrimString(ret);
    EXPECT_EQ(ret, "abc");
}

HWTEST_F(UtilsUnitTest, TrimUpdateMode, TestSize.Level0)
{
    const std::string updateMode = "--update_package=";
    EXPECT_EQ(Utils::TrimUpdateMode(updateMode), "update_package");
    const std::string sdcardMode = "--sdcard_update";
    EXPECT_EQ(Utils::TrimUpdateMode(sdcardMode), "sdcard_update");
    const std::string intralMode = "--sdcard_intral_update";
    EXPECT_EQ(Utils::TrimUpdateMode(intralMode), "sdcard_intral_update");
    const std::string emptyStr = "";
    EXPECT_EQ(Utils::TrimUpdateMode(emptyStr), "");
}

HWTEST_F(UtilsUnitTest, ConvertStrToNumber_test_001, TestSize.Level0)
{
    const std::string str = "12345";
    int64_t value;
    EXPECT_EQ(Utils::ConvertToLongLong(str, value), true);
}

HWTEST_F(UtilsUnitTest, ConvertStrToNumber_test_002, TestSize.Level0)
{
    const std::string str = "12345";
    int32_t value;
    EXPECT_EQ(Utils::ConvertToLong(str, value), true);
}

HWTEST_F(UtilsUnitTest, ConvertStrToNumber_test_003, TestSize.Level0)
{
    const std::string str = "12345";
    uint32_t value;
    EXPECT_EQ(Utils::ConvertToUnsignedLong(str, value), true);
}

HWTEST_F(UtilsUnitTest, ConvertStrToNumber_test_004, TestSize.Level0)
{
    const std::string str = "1.2345";
    double value;
    EXPECT_EQ(Utils::ConvertToDouble(str, value), true);
}

HWTEST_F(UtilsUnitTest, ConvertStrToNumber_test_005, TestSize.Level0)
{
    const std::string str = "1.2345";
    float value;
    EXPECT_EQ(Utils::ConvertToFloat(str, value), true);
}

HWTEST_F(UtilsUnitTest, pathToRealPath_ShouldReturnFalse_WhenPathIsEmpty, TestSize.Level0)
{
    std::string realPath;
    EXPECT_FALSE(PathToRealPath("", realPath));
}

HWTEST_F(UtilsUnitTest, pathToRealPath_ShouldReturnFalse_WhenPathLenIsError, TestSize.Level0)
{
    std::string realPath;
    std::string longPath(PATH_MAX, 'a');
    EXPECT_FALSE(PathToRealPath(longPath, realPath));
}

HWTEST_F(UtilsUnitTest, pathToRealPath_ShouldReturnFalse_WhenPathToRealpathError, TestSize.Level0)
{
    std::string realPath;
    EXPECT_FALSE(PathToRealPath("/path/to/nonexistent/file", realPath));
}

HWTEST_F(UtilsUnitTest, loadLibrary_ShouldReturnNull_WhenPathIsEmpty, TestSize.Level0)
{
    std::string libPath = "";
    void* handle = LoadLibrary(libPath);
    EXPECT_EQ(handle, nullptr);
}

HWTEST_F(UtilsUnitTest, loadLibrary_ShouldReturnNull_WhenRealPathError, TestSize.Level0)
{
    std::string libPath = "/invalid/path";
    void* handle = LoadLibrary(libPath);
    EXPECT_EQ(handle, nullptr);
}

HWTEST_F(UtilsUnitTest, loadLibrary_ShouldReturnNull_WhenLibPathInvalid, TestSize.Level0)
{
    std::string libPath = "/invalid/lib/path";
    void* handle = LoadLibrary(libPath);
    EXPECT_EQ(handle, nullptr);
}

HWTEST_F(UtilsUnitTest, loadLibrary_ShouldReturnNull_WhenDlopenFail, TestSize.Level0)
{
    std::string libPath = "/system/lib64/invalid.so";
    void* handle = LoadLibrary(libPath);
    EXPECT_EQ(handle, nullptr);
}

HWTEST_F(UtilsUnitTest, getFunction_NullHandle_Test, TestSize.Level0)
{
    void* handle = nullptr;
    std::string funcName = "testFunc";
    void* result = GetFunction(handle, funcName);
    EXPECT_EQ(result, nullptr);
}

HWTEST_F(UtilsUnitTest, getFunction_EmptyFuncName_Test, TestSize.Level0)
{
    void* handle = reinterpret_cast<void *>(0x12345678);
    std::string funcName = "";
    void* result = GetFunction(handle, funcName);
    EXPECT_EQ(result, nullptr);
}

HWTEST_F(UtilsUnitTest, readStringFromProcFile_OpenFileFailed_Test, TestSize.Level0)
{
    std::string filePath = "/path/to/nonexistent/file";
    std::string content;
    EXPECT_FALSE(ReadStringFromProcFile(filePath, content));
}

HWTEST_F(UtilsUnitTest, readStringFromProcFile_FileOversize_Test, TestSize.Level0)
{
    std::string filePath = "/data/updater/updater/invalid_file";
    CreateFile(filePath, 1024 * 5);
    std::string content;
    EXPECT_FALSE(ReadStringFromProcFile(filePath, content));
    EXPECT_EQ(DeleteFile(filePath), 0);
}

HWTEST_F(UtilsUnitTest, readStringFromProcFile_Success_Test, TestSize.Level0)
{
    std::string filePath = "/data/updater/updater/valid_file";
    CreateFile(filePath, 1024 * 4);
    std::string content;
    EXPECT_TRUE(ReadStringFromProcFile(filePath, content));
    EXPECT_EQ(DeleteFile(filePath), 0);
}

HWTEST_F(UtilsUnitTest, copyFile_WhenSrcIsInvalid, TestSize.Level0)
{
    std::string src = "/invalid/path/to/file";
    std::string dest = "/data/updater/updater/valid_file";
    CreateFile(dest, 1);
    bool isAppend = false;
    EXPECT_FALSE(CopyFile(src, dest, isAppend));
    EXPECT_EQ(DeleteFile(dest), 0);
}

HWTEST_F(UtilsUnitTest, copyFile_WhenDestIsInvalid, TestSize.Level0)
{
    std::string src = "/data/updater/updater/valid_file";
    std::string dest = "/invalid/path/to/dest";
    CreateFile(src, 1);
    bool isAppend = false;
    EXPECT_FALSE(CopyFile(src, dest, isAppend));
    EXPECT_EQ(DeleteFile(src), 0);
}

HWTEST_F(UtilsUnitTest, copyFile__WhenFileCopySuccess, TestSize.Level0)
{
    std::string src = "/data/updater/updater/src_file";
    std::string dest = "/data/updater/updater/dest_file";
    CreateFile(src, 1);
    CreateFile(dest, 0);
    bool isAppend = false;
    EXPECT_TRUE(CopyFile(src, dest, isAppend));
    EXPECT_EQ(DeleteFile(src), 0);
    EXPECT_EQ(DeleteFile(dest), 0);
}

HWTEST_F(UtilsUnitTest, copyFileAppend_WhenFileCopySuccess, TestSize.Level0)
{
    std::string src = "/data/updater/updater/src_file";
    std::string dest = "/data/updater/updater/dest_file";
    CreateFile(src, 1);
    CreateFile(dest, 0);
    bool isAppend = true;
    EXPECT_TRUE(CopyFile(src, dest, isAppend));
    EXPECT_EQ(DeleteFile(src), 0);
    EXPECT_EQ(DeleteFile(dest), 0);
}

HWTEST_F(UtilsUnitTest, copyDir_Test_01, TestSize.Level0)
{
    std::string srcPath = "/path/to/nonexistent/dir";
    std::string dstPath = "/data/updater/updater/dest_dir";
    EXPECT_FALSE(CopyDir(srcPath, dstPath));
    std::error_code ec;
    std::filesystem::remove_all(dstPath.c_str(), ec);
}

HWTEST_F(UtilsUnitTest, copyDir_Test_02, TestSize.Level0)
{
    std::string srcPath = "/data/updater/updater/src_dir";
    EXPECT_EQ(MkdirRecursive(srcPath, 0755), 0);
    std::string dstPath = "/path/to/nonexistent/dir";
    EXPECT_FALSE(CopyDir(srcPath, dstPath));
    std::error_code ec;
    std::filesystem::remove_all(srcPath.c_str(), ec);
}

HWTEST_F(UtilsUnitTest, copyDir_Test_03, TestSize.Level0)
{
    std::string srcPath = "/data/updater/updater/src_dir";
    std::string dstPath = "/data/updater/updater/dst_dir";
    EXPECT_EQ(MkdirRecursive(srcPath, 0755), 0);
    EXPECT_EQ(MkdirRecursive(dstPath, 0755), 0);
    EXPECT_TRUE(CopyDir(srcPath, dstPath));
    std::error_code ec;
    std::filesystem::remove_all(srcPath.c_str(), ec);
    std::filesystem::remove_all(dstPath.c_str(), ec);
}

HWTEST_F(UtilsUnitTest, copyDir_Test_04, TestSize.Level0)
{
    std::string srcPath = "/data/updater/updater/sub_dir/dir";
    std::string dstPath = "/data/updater/updater/dst_dir";
    EXPECT_EQ(MkdirRecursive(srcPath, 0755), 0);
    EXPECT_TRUE(CopyDir(srcPath, dstPath));
    std::error_code ec;
    std::string path = "/data/updater/updater/sub_dir";
    std::filesystem::remove_all(path.c_str(), ec);
    std::filesystem::remove_all(dstPath.c_str(), ec);
}

HWTEST_F(UtilsUnitTest, deleteOldFile_WhenFolderDoesNotExist, TestSize.Level0)
{
    std::string folderPath = "/data/updater/updater/nonexistent_folder";
    EXPECT_FALSE(DeleteOldFile(folderPath));
}

HWTEST_F(UtilsUnitTest, deleteOldFile_WhenFolderIsEmpty, TestSize.Level0)
{
    std::string dirPath = "/data/updater/updater/dir";
    EXPECT_EQ(MkdirRecursive(dirPath, 0755), 0);
    EXPECT_FALSE(DeleteOldFile(dirPath));
    std::error_code ec;
    std::filesystem::remove_all(dirPath.c_str(), ec);
}

HWTEST_F(UtilsUnitTest, deleteOldFile_WhenFileIsDeletedSuccessfully, TestSize.Level0)
{
    std::string dirPath = "/data/updater/updater/dir";
    EXPECT_EQ(MkdirRecursive(dirPath, 0755), 0);
    std::string fileName = "file";
    CreateFile(dirPath + "/" + fileName, 1);
    EXPECT_TRUE(DeleteOldFile(dirPath));
    std::error_code ec;
    std::filesystem::remove_all(dirPath.c_str(), ec);
}

HWTEST_F(UtilsUnitTest, vectorToString_WhenPidsIsEmpty, TestSize.Level0)
{
    std::vector<pid_t> pids;
    std::string result = VectorToString(pids);
    EXPECT_EQ(result, "");
}

HWTEST_F(UtilsUnitTest, vectorToString_WhenPidsHasOneElement, TestSize.Level0)
{
    std::vector<pid_t> pids = {123};
    std::string result = VectorToString(pids);
    EXPECT_EQ(result, "123");
}

HWTEST_F(UtilsUnitTest, vectorToString_WhenPidsHasMultipleElements, TestSize.Level0)
{
    std::vector<pid_t> pids = {123, 456, 789};
    std::string result = VectorToString(pids);
    EXPECT_EQ(result, "123,456,789");
}
} // updater_ut
