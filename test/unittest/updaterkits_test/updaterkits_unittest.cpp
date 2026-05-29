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

#include "updaterkits_unittest.h"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "securec.h"
#include "updaterkits/updaterkits.h"

using namespace testing::ext;
using namespace UpdaterUt;
using namespace std;

namespace UpdaterUt {
const std::string MISC_FILE = "/data/updater/misc_ut";
const std::string TEST_PACKAGE_PATH = "/data/updater/test_package.zip";

void UpdaterKitsUnitTest::SetUpTestCase(void)
{
    cout << "Updater Unit UpdaterKitsUnitTest Begin!" << endl;
}

void UpdaterKitsUnitTest::TearDownTestCase(void)
{
    cout << "Updater Unit UpdaterKitsUnitTest End!" << endl;
}

HWTEST_F(UpdaterKitsUnitTest, updater_kits_test01, TestSize.Level1)
{
    const std::vector<std::string> packageName1 = {""};
    int ret = RebootAndInstallUpgradePackage(MISC_FILE, packageName1);
    EXPECT_EQ(ret, 2); // 2 : path not exit

    const std::vector<std::string> packageName2 = {"/data/updater/updater/updater_without_updater_binary.zip"};
    auto fp = std::unique_ptr<FILE, decltype(&fclose)>(fopen(MISC_FILE.c_str(), "wb+"), fclose);
    EXPECT_NE(fp, nullptr);
    ret = RebootAndInstallUpgradePackage(MISC_FILE, packageName2);
    EXPECT_EQ(ret, 0);
    ret = RebootAndInstallUpgradePackage(MISC_FILE, packageName2, UPGRADE_TYPE_SD);
    EXPECT_EQ(ret, 0);
    ret = RebootAndInstallUpgradePackage(MISC_FILE, packageName2, UPGRADE_TYPE_SD_INTRAL);
    EXPECT_EQ(ret, 0);
    unlink(MISC_FILE.c_str());
}

HWTEST_F(UpdaterKitsUnitTest, updater_kits_test02, TestSize.Level1)
{
    const std::string cmd1 = "";
    bool ret = RebootAndCleanUserData(MISC_FILE, cmd1);
    EXPECT_EQ(ret, false);

    std::vector<std::string> pkgPath {};
    ret = RebootAndInstallSdcardPackage(MISC_FILE, pkgPath);
    EXPECT_EQ(ret, true);

    std::string path = "data/sdcard/updater/updater.zip";
    pkgPath.push_back(path);
    ret = RebootAndInstallSdcardPackage(MISC_FILE, pkgPath);
    EXPECT_EQ(ret, true);

    char testPath[1024] = {0}; // 1024: max len
    for (int i = 0; i < sizeof(testPath) - 1; i++) {
        testPath[i] = 'a';
    }
    pkgPath.push_back(testPath);
    ret = RebootAndInstallSdcardPackage(MISC_FILE, pkgPath);
    EXPECT_EQ(ret, false);

    const std::string cmd2 = "--user_wipe_data";
    auto fp = std::unique_ptr<FILE, decltype(&fclose)>(fopen(MISC_FILE.c_str(), "wb+"), fclose);
    EXPECT_NE(fp, nullptr);
    ret = RebootAndCleanUserData(MISC_FILE, cmd2);
    EXPECT_EQ(ret, true);
    unlink(MISC_FILE.c_str());
}

// 测试 EstimatedEraseTime 函数
HWTEST_F(UpdaterKitsUnitTest, updater_kits_test03, TestSize.Level1)
{
    std::string eraseType = "secure_erase";
    uint32_t ret = EstimatedEraseTime(eraseType);
    EXPECT_GE(ret, 0);
}

HWTEST_F(UpdaterKitsUnitTest, updater_kits_test04, TestSize.Level1)
{
    std::string eraseType = "";
    uint32_t ret = EstimatedEraseTime(eraseType);
    EXPECT_GE(ret, 0);
}

// 测试 RebootAndSecureErase 函数
HWTEST_F(UpdaterKitsUnitTest, updater_kits_test05, TestSize.Level1)
{
    std::string eraseType = "secure_erase";
    auto fp = std::unique_ptr<FILE, decltype(&fclose)>(fopen(MISC_FILE.c_str(), "wb+"), fclose);
    EXPECT_NE(fp, nullptr);
    bool ret = RebootAndSecureErase(eraseType);
    EXPECT_EQ(ret, true);
    unlink(MISC_FILE.c_str());
}

HWTEST_F(UpdaterKitsUnitTest, updater_kits_test06, TestSize.Level1)
{
    std::string eraseType = "";
    bool ret = RebootAndSecureErase(eraseType);
    EXPECT_EQ(ret, true);
}

// 测试 OTA_INTRAL 升级模式
HWTEST_F(UpdaterKitsUnitTest, updater_kits_test07, TestSize.Level1)
{
    int fd = open(TEST_PACKAGE_PATH.c_str(), O_CREAT | O_RDWR, 0644);
    if (fd >= 0) {
        close(fd);
    }
    
    std::vector<std::string> packageName = {TEST_PACKAGE_PATH};
    auto fp = std::unique_ptr<FILE, decltype(&fclose)>(fopen(MISC_FILE.c_str(), "wb+"), fclose);
    EXPECT_NE(fp, nullptr);
    int ret = RebootAndInstallUpgradePackage(MISC_FILE, packageName, UPGRADE_TYPE_OTA_INTRAL);
    EXPECT_EQ(ret, 0);
    unlink(MISC_FILE.c_str());
    unlink(TEST_PACKAGE_PATH.c_str());
}

// 测试 SUBPKG_UPDATE 升级模式
HWTEST_F(UpdaterKitsUnitTest, updater_kits_test08, TestSize.Level1)
{
    int fd = open(TEST_PACKAGE_PATH.c_str(), O_CREAT | O_RDWR, 0644);
    if (fd >= 0) {
        close(fd);
    }
    
    std::vector<std::string> packageName = {TEST_PACKAGE_PATH};
    auto fp = std::unique_ptr<FILE, decltype(&fclose)>(fopen(MISC_FILE.c_str(), "wb+"), fclose);
    EXPECT_NE(fp, nullptr);
    int ret = RebootAndInstallUpgradePackage(MISC_FILE, packageName, UPGRADE_TYPE_SUBPKG_UPDATE);
    EXPECT_EQ(ret, 0);
    unlink(MISC_FILE.c_str());
    unlink(TEST_PACKAGE_PATH.c_str());
}

// 测试带回调函数的重启
HWTEST_F(UpdaterKitsUnitTest, updater_kits_test09, TestSize.Level1)
{
    int fd = open(TEST_PACKAGE_PATH.c_str(), O_CREAT | O_RDWR, 0644);
    if (fd >= 0) {
        close(fd);
    }
    
    std::vector<std::string> packageName = {TEST_PACKAGE_PATH};
    RebootFunType rebootFunc = []() { return 0; };
    auto fp = std::unique_ptr<FILE, decltype(&fclose)>(fopen(MISC_FILE.c_str(), "wb+"), fclose);
    EXPECT_NE(fp, nullptr);
    int ret = RebootAndInstallUpgradePackage(MISC_FILE, packageName, UPGRADE_TYPE_OTA, rebootFunc);
    EXPECT_EQ(ret, 0);
    unlink(MISC_FILE.c_str());
    unlink(TEST_PACKAGE_PATH.c_str());
}

// 测试回调函数失败场景
HWTEST_F(UpdaterKitsUnitTest, updater_kits_test10, TestSize.Level1)
{
    int fd = open(TEST_PACKAGE_PATH.c_str(), O_CREAT | O_RDWR, 0644);
    if (fd >= 0) {
        close(fd);
    }
    
    std::vector<std::string> packageName = {TEST_PACKAGE_PATH};
    RebootFunType rebootFunc = []() { return -1; };
    auto fp = std::unique_ptr<FILE, decltype(&fclose)>(fopen(MISC_FILE.c_str(), "wb+"), fclose);
    EXPECT_NE(fp, nullptr);
    int ret = RebootAndInstallUpgradePackage(MISC_FILE, packageName, UPGRADE_TYPE_OTA, rebootFunc);
    EXPECT_EQ(ret, 0);
    unlink(MISC_FILE.c_str());
    unlink(TEST_PACKAGE_PATH.c_str());
}

// 测试空包名OTA升级（应该返回错误）
HWTEST_F(UpdaterKitsUnitTest, updater_kits_test11, TestSize.Level1)
{
    std::vector<std::string> packageName;
    int ret = RebootAndInstallUpgradePackage(MISC_FILE, packageName, UPGRADE_TYPE_OTA);
    EXPECT_EQ(ret, 1);  // 1: invalid input
}

// 测试空包名OTA_INTRAL升级（应该返回错误）
HWTEST_F(UpdaterKitsUnitTest, updater_kits_test12, TestSize.Level1)
{
    std::vector<std::string> packageName;
    int ret = RebootAndInstallUpgradePackage(MISC_FILE, packageName, UPGRADE_TYPE_OTA_INTRAL);
    EXPECT_EQ(ret, 1);  // 1: invalid input
}

// 测试带 --force_update_action 参数
HWTEST_F(UpdaterKitsUnitTest, updater_kits_test13, TestSize.Level1)
{
    int fd = open(TEST_PACKAGE_PATH.c_str(), O_CREAT | O_RDWR, 0644);
    if (fd >= 0) {
        close(fd);
    }
    
    std::vector<std::string> packageName = {TEST_PACKAGE_PATH, "--force_update_action=poweroff"};
    auto fp = std::unique_ptr<FILE, decltype(&fclose)>(fopen(MISC_FILE.c_str(), "wb+"), fclose);
    EXPECT_NE(fp, nullptr);
    int ret = RebootAndInstallUpgradePackage(MISC_FILE, packageName, UPGRADE_TYPE_OTA);
    EXPECT_EQ(ret, 0);
    unlink(MISC_FILE.c_str());
    unlink(TEST_PACKAGE_PATH.c_str());
}

// 测试带 --night_update 参数
HWTEST_F(UpdaterKitsUnitTest, updater_kits_test14, TestSize.Level1)
{
    int fd = open(TEST_PACKAGE_PATH.c_str(), O_CREAT | O_RDWR, 0644);
    if (fd >= 0) {
        close(fd);
    }
    
    std::vector<std::string> packageName = {TEST_PACKAGE_PATH, "--night_update"};
    auto fp = std::unique_ptr<FILE, decltype(&fclose)>(fopen(MISC_FILE.c_str(), "wb+"), fclose);
    EXPECT_NE(fp, nullptr);
    int ret = RebootAndInstallUpgradePackage(MISC_FILE, packageName, UPGRADE_TYPE_OTA);
    EXPECT_EQ(ret, 0);
    unlink(MISC_FILE.c_str());
    unlink(TEST_PACKAGE_PATH.c_str());
}

// 测试带 --shrink_info 参数
HWTEST_F(UpdaterKitsUnitTest, updater_kits_test15, TestSize.Level1)
{
    int fd = open(TEST_PACKAGE_PATH.c_str(), O_CREAT | O_RDWR, 0644);
    if (fd >= 0) {
        close(fd);
    }
    
    std::vector<std::string> packageName = {TEST_PACKAGE_PATH, "--shrink_info=test"};
    auto fp = std::unique_ptr<FILE, decltype(&fclose)>(fopen(MISC_FILE.c_str(), "wb+"), fclose);
    EXPECT_NE(fp, nullptr);
    int ret = RebootAndInstallUpgradePackage(MISC_FILE, packageName, UPGRADE_TYPE_OTA);
    EXPECT_EQ(ret, 0);
    unlink(MISC_FILE.c_str());
    unlink(TEST_PACKAGE_PATH.c_str());
}

// 测试带 --virtual_shrink_info 参数
HWTEST_F(UpdaterKitsUnitTest, updater_kits_test16, TestSize.Level1)
{
    int fd = open(TEST_PACKAGE_PATH.c_str(), O_CREAT | O_RDWR, 0644);
    if (fd >= 0) {
        close(fd);
    }
    
    std::vector<std::string> packageName = {TEST_PACKAGE_PATH, "--virtual_shrink_info=test"};
    auto fp = std::unique_ptr<FILE, decltype(&fclose)>(fopen(MISC_FILE.c_str(), "wb+"), fclose);
    EXPECT_NE(fp, nullptr);
    int ret = RebootAndInstallUpgradePackage(MISC_FILE, packageName, UPGRADE_TYPE_OTA);
    EXPECT_EQ(ret, 0);
    unlink(MISC_FILE.c_str());
    unlink(TEST_PACKAGE_PATH.c_str());
}

// 测试带 --screen_state 参数
HWTEST_F(UpdaterKitsUnitTest, updater_kits_test17, TestSize.Level1)
{
    int fd = open(TEST_PACKAGE_PATH.c_str(), O_CREAT | O_RDWR, 0644);
    if (fd >= 0) {
        close(fd);
    }
    
    std::vector<std::string> packageName = {TEST_PACKAGE_PATH, "--screen_state=on"};
    auto fp = std::unique_ptr<FILE, decltype(&fclose)>(fopen(MISC_FILE.c_str(), "wb+"), fclose);
    EXPECT_NE(fp, nullptr);
    int ret = RebootAndInstallUpgradePackage(MISC_FILE, packageName, UPGRADE_TYPE_OTA);
    EXPECT_EQ(ret, 0);
    unlink(MISC_FILE.c_str());
    unlink(TEST_PACKAGE_PATH.c_str());
}

// 测试多个参数组合
HWTEST_F(UpdaterKitsUnitTest, updater_kits_test18, TestSize.Level1)
{
    int fd = open(TEST_PACKAGE_PATH.c_str(), O_CREAT | O_RDWR, 0644);
    if (fd >= 0) {
        close(fd);
    }
    
    std::vector<std::string> packageName = {
        TEST_PACKAGE_PATH,
        "--force_update_action=poweroff",
        "--night_update",
        "--shrink_info=test",
        "--virtual_shrink_info=test"
    };
    auto fp = std::unique_ptr<FILE, decltype(&fclose)>(fopen(MISC_FILE.c_str(), "wb+"), fclose);
    EXPECT_NE(fp, nullptr);
    int ret = RebootAndInstallUpgradePackage(MISC_FILE, packageName, UPGRADE_TYPE_OTA);
    EXPECT_EQ(ret, 0);
    unlink(MISC_FILE.c_str());
    unlink(TEST_PACKAGE_PATH.c_str());
}

// 测试工厂重置命令
HWTEST_F(UpdaterKitsUnitTest, updater_kits_test19, TestSize.Level1)
{
    std::string cmd = "--factory_wipe_data";
    auto fp = std::unique_ptr<FILE, decltype(&fclose)>(fopen(MISC_FILE.c_str(), "wb+"), fclose);
    EXPECT_NE(fp, nullptr);
    bool ret = RebootAndCleanUserData(MISC_FILE, cmd);
    EXPECT_EQ(ret, true);
    unlink(MISC_FILE.c_str());
}

// 测试菜单重置命令
HWTEST_F(UpdaterKitsUnitTest, updater_kits_test20, TestSize.Level1)
{
    std::string cmd = "--menu_wipe_data";
    auto fp = std::unique_ptr<FILE, decltype(&fclose)>(fopen(MISC_FILE.c_str(), "wb+"), fclose);
    EXPECT_NE(fp, nullptr);
    bool ret = RebootAndCleanUserData(MISC_FILE, cmd);
    EXPECT_EQ(ret, true);
    unlink(MISC_FILE.c_str());
}

// 测试空misc文件路径
HWTEST_F(UpdaterKitsUnitTest, updater_kits_test21, TestSize.Level1)
{
    std::string cmd = "--user_wipe_data";
    bool ret = RebootAndCleanUserData("", cmd);
    EXPECT_EQ(ret, false);
}

// 测试多个包路径场景
HWTEST_F(UpdaterKitsUnitTest, updater_kits_test22, TestSize.Level1)
{
    std::string pkg1 = "/data/updater/package1.zip";
    std::string pkg2 = "/data/updater/package2.zip";
    
    int fd1 = open(pkg1.c_str(), O_CREAT | O_RDWR, 0644);
    if (fd1 >= 0) {
        close(fd1);
    }
    
    int fd2 = open(pkg2.c_str(), O_CREAT | O_RDWR, 0644);
    if (fd2 >= 0) {
        close(fd2);
    }
    
    std::vector<std::string> packageName = {pkg1, pkg2};
    auto fp = std::unique_ptr<FILE, decltype(&fclose)>(fopen(MISC_FILE.c_str(), "wb+"), fclose);
    EXPECT_NE(fp, nullptr);
    int ret = RebootAndInstallUpgradePackage(MISC_FILE, packageName, UPGRADE_TYPE_OTA);
    EXPECT_EQ(ret, 0);
    unlink(MISC_FILE.c_str());
    unlink(pkg1.c_str());
    unlink(pkg2.c_str());
}

HWTEST_F(UpdaterKitsUnitTest, updater_kits_test23, TestSize.Level1)
{
    std::string eraseType = "";
    std::string cmd = "";

    bool ret = RebootAndSecureErase(eraseType, cmd);
    EXPECT_EQ(ret, false);
    ret = RebootAndSecureErase(eraseType);
    EXPECT_EQ(ret, true);

    eraseType = "DATA_AND_OS";
    cmd = "cmd";
    ret = RebootAndSecureErase(eraseType, cmd);
    EXPECT_EQ(ret, true);
    ret = RebootAndSecureErase(eraseType);
    EXPECT_EQ(ret, true);
}

HWTEST_F(UpdaterKitsUnitTest, updater_kits_test24, TestSize.Level1)
{
    std::string eraseType = "";

    uint32_t ret = EstimatedEraseTime(eraseType);
    EXPECT_GE(ret, 0);
}
} // namespace UpdaterUt
