/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <fcntl.h>
#include <gtest/gtest.h>
#include <memory>
#include <sys/ioctl.h>
#include "log/log.h"
#include "securec.h"
#include "updater/updater_const.h"
#include "updater/updater.h"
#include "updater/updater_preprocess.h"
#include "sdcard_update/sdcard_update.h"
#include "fs_manager/mount.h"
#include "misc_info/misc_info.h"
#include "updater_main.h"
#include "updater_ui_stub.h"
#include "utils.h"

using namespace Updater;
using namespace std;
using namespace testing::ext;

namespace {
constexpr uint32_t MAX_ARG_SIZE = 24;
class UpdaterUtilUnitTest : public testing::Test {
public:
    UpdaterUtilUnitTest()
    {
        InitUpdaterLogger("UPDATER", TMP_LOG, TMP_STAGE_LOG, TMP_ERROR_CODE_PATH);
    }
    ~UpdaterUtilUnitTest() {}

    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() {}
    void TearDown() {}
    void TestBody() {}
};

HWTEST_F(UpdaterUtilUnitTest, DeleteUpdaterPath, TestSize.Level1)
{
    std::string path = "/data/test/test/test";
    bool ret = DeleteUpdaterPath(path);
    EXPECT_EQ(ret, true);

    path = "/data/test";
    ret = DeleteUpdaterPath(path);
    EXPECT_EQ(ret, true);
}

HWTEST_F(UpdaterUtilUnitTest, ClearMisc, TestSize.Level1)
{
    bool ret = ClearMisc();
    EXPECT_EQ(ret, true);
}

HWTEST_F(UpdaterUtilUnitTest, IsSDCardExist, TestSize.Level1)
{
    std::string sdcardStr = "";
    bool ret = IsSDCardExist(sdcardStr);
    EXPECT_EQ(ret, false);
}

HWTEST_F(UpdaterUtilUnitTest, IsFlashd, TestSize.Level1)
{
    EXPECT_EQ(IsFlashd({"boot_updater", "", "boot_flash"}), true);
    EXPECT_EQ(IsFlashd({"boot_updater", "", ""}), false);
}

HWTEST_F(UpdaterUtilUnitTest, IsUpdater, TestSize.Level1)
{
    EXPECT_EQ(IsUpdater({"boot_updater", "", ""}), true);
    EXPECT_EQ(IsUpdater({"boot_updater", "", "boot_flash"}), false);
    EXPECT_EQ(IsUpdater({"boot_updater", "", "xxx"}), true);
}

HWTEST_F(UpdaterUtilUnitTest, SelectMode, TestSize.Level1)
{
    // clear already registered mode
    GetBootModes().clear();

    auto dummyEntry = [] (int argc, char **argv) -> int { return 0; };
    // register modes
    RegisterMode({ IsFlashd, "FLASHD", "", dummyEntry });
    RegisterMode({ IsUpdater, "UPDATER", "", dummyEntry });

    // test select mode
    auto mode = SelectMode({"boot_updater", "", ""});
    ASSERT_NE(mode, std::nullopt);
    EXPECT_EQ(mode->modeName, "UPDATER");

    mode = SelectMode({"boot_updater", "", "boot_flash"});
    ASSERT_NE(mode, std::nullopt);
    EXPECT_EQ(mode->modeName, "FLASHD");

    mode = SelectMode({"invalid_command", "", ""});
    EXPECT_EQ(mode, std::nullopt);
}

HWTEST_F(UpdaterUtilUnitTest, ParseParams, TestSize.Level1)
{
    UpdateMessage boot {};
    std::string commandMsg = "";
    std::string updateMsg = "";
    const std::string commandFile = "/data/updater/command";
    auto fp = std::unique_ptr<FILE, decltype(&fclose)>(fopen(commandFile.c_str(), "wb"), fclose);
    EXPECT_NE(fp, nullptr);
    EXPECT_EQ(strncpy_s(boot.command, sizeof(boot.command) - 1, commandMsg.c_str(), commandMsg.size()), 0);
    EXPECT_EQ(strncpy_s(boot.update, sizeof(boot.update) - 1, updateMsg.c_str(), updateMsg.size()), 0);
    bool bRet = WriteUpdaterMessage(commandFile, boot);
    EXPECT_EQ(bRet, true);
    char **argv = new char*[1];
    argv[0] = new char[MAX_ARG_SIZE];
    std::string str = "./UpdaterMain";
    EXPECT_EQ(strncpy_s(argv[0], MAX_ARG_SIZE, str.c_str(), str.size()), 0);
    int argc = 1;
    std::vector<std::string> args = Utils::ParseParams(argc, argv);
    std::string res = "";
    for (auto s : args) {
        res += s;
    }
    EXPECT_EQ("./UpdaterMain", res);

    commandMsg = "boot_updater";
    updateMsg = "--update_package=updater_full.zip";
    EXPECT_EQ(strncpy_s(boot.command, sizeof(boot.command) - 1, commandMsg.c_str(), commandMsg.size()), 0);
    EXPECT_EQ(strncpy_s(boot.update, sizeof(boot.update) - 1, updateMsg.c_str(), updateMsg.size()), 0);
    bRet = WriteUpdaterMessage(commandFile, boot);
    EXPECT_EQ(bRet, true);

    args = Utils::ParseParams(argc, argv);
    res = "";
    for (auto s : args) {
        res += s;
    }
    EXPECT_EQ("./UpdaterMain--update_package=updater_full.zip", res);
}

HWTEST_F(UpdaterUtilUnitTest, UpdaterMain, TestSize.Level1)
{
    UpdateMessage boot {};
    if (access("/data/updater/", 0)) {
        int ret = mkdir("/data/updater/", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
        ASSERT_EQ(ret, 0);
    }
    const std::string commandFile = "/data/updater/command";
    auto fp = std::unique_ptr<FILE, decltype(&fclose)>(fopen(commandFile.c_str(), "wb"), fclose);
    EXPECT_NE(fp, nullptr);
    const std::string commandMsg = "boot_updater";
    const std::string updateMsg = "--update_package=/data/updater/updater/updater_full.zip";
    EXPECT_EQ(strncpy_s(boot.command, sizeof(boot.command) - 1, commandMsg.c_str(), commandMsg.size()), 0);
    EXPECT_EQ(strncpy_s(boot.update, sizeof(boot.update) - 1, updateMsg.c_str(), updateMsg.size()), 0);
    bool bRet = WriteUpdaterMessage(commandFile, boot);
    EXPECT_EQ(bRet, true);
    char **argv = new char*[1];
    argv[0] = new char[MAX_ARG_SIZE];
    EXPECT_EQ(strncpy_s(argv[0], MAX_ARG_SIZE, "./UpdaterMain", MAX_ARG_SIZE), 0);
    int argc = 1;

    int ret = UpdaterMain(argc, argv);
    EXPECT_EQ(ret, 0);
    PostUpdater(true);
    delete argv[0];
    delete []argv;
}

HWTEST_F(UpdaterUtilUnitTest, UpdaterFromSdcardTest, TestSize.Level1)
{
    UpdateMessage boot {};
    if (access("/data/updater/", 0)) {
        int ret = mkdir("/data/updater/", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
        ASSERT_EQ(ret, 0);
    }
    const std::string commandFile = "/data/updater/command";
    auto fp = std::unique_ptr<FILE, decltype(&fclose)>(fopen(commandFile.c_str(), "wb"), fclose);
    EXPECT_NE(fp, nullptr);
    const std::string commandMsg = "boot_updater";
    const std::string updateMsg = "--sdcard_update";
    EXPECT_EQ(strncpy_s(boot.command, sizeof(boot.command) - 1, commandMsg.c_str(), commandMsg.size()), 0);
    EXPECT_EQ(strncpy_s(boot.update, sizeof(boot.update) - 1, updateMsg.c_str(), updateMsg.size()), 0);
    bool bRet = WriteUpdaterMessage(commandFile, boot);
    EXPECT_EQ(bRet, true);
    char **argv = new char*[1];
    argv[0] = new char[MAX_ARG_SIZE];
    EXPECT_EQ(strncpy_s(argv[0], MAX_ARG_SIZE, "./UpdaterMain", MAX_ARG_SIZE), 0);
    int argc = 1;
    EXPECT_EQ(UpdaterMain(argc, argv), 0);
    delete argv[0];
    delete []argv;
}

HWTEST_F(UpdaterUtilUnitTest, DoInstallUpdaterPackageTest, TestSize.Level1)
{
    UpdaterParams upParams;
    upParams.callbackProgress = nullptr;
    std::vector<std::string> output;
    EXPECT_EQ(DoInstallUpdaterPackage(nullptr, upParams, HOTA_UPDATE), UPDATE_CORRUPT);
    upParams.callbackProgress = [] (float value) {};
    EXPECT_EQ(DoInstallUpdaterPackage(nullptr, upParams, HOTA_UPDATE), UPDATE_CORRUPT);
    upParams.retryCount = 0;
    EXPECT_EQ(DoInstallUpdaterPackage(nullptr, upParams, HOTA_UPDATE), UPDATE_CORRUPT);
    upParams.retryCount = 1;
    EXPECT_EQ(DoInstallUpdaterPackage(nullptr, upParams, HOTA_UPDATE), UPDATE_CORRUPT);
}

HWTEST_F(UpdaterUtilUnitTest, updater_ExtractUpdaterBinary, TestSize.Level1)
{
    Hpackage::PkgManager::PkgManagerPtr pkgManager = Hpackage::PkgManager::CreatePackageInstance();
    std::string path = "xxx";
    int32_t ret = ExtractUpdaterBinary(pkgManager, path, UPDATER_BINARY);
    EXPECT_EQ(ret, 1);
    path = "/data/updater/updater/updater_full.zip";
    ret = ExtractUpdaterBinary(pkgManager, path, UPDATER_BINARY);
    Hpackage::PkgManager::ReleasePackageInstance(pkgManager);
    EXPECT_EQ(ret, 1);
}

HWTEST_F(UpdaterUtilUnitTest, updater_IsSpaceCapacitySufficient, TestSize.Level1)
{
    UpdaterParams upParams {};
    UpdaterStatus status = IsSpaceCapacitySufficient(upParams);
    EXPECT_EQ(status, UPDATE_ERROR);
    upParams.updatePackage.push_back("/data/updater/updater/updater_full.zip");
    status = IsSpaceCapacitySufficient(upParams);
    EXPECT_EQ(status, UPDATE_SUCCESS);
    upParams.updatePackage.push_back("xxx");
    ProgressSmoothHandler(0, 0);
    ProgressSmoothHandler(-1, 0);
    ProgressSmoothHandler(0, 1);
    status = IsSpaceCapacitySufficient(upParams);
    EXPECT_EQ(status, UPDATE_ERROR);
}

HWTEST_F(UpdaterUtilUnitTest, updater_HandleChildOutput, TestSize.Level1)
{
    std::string buf = "xxx";
    bool retryUpdate = false;
    UpdaterParams upParams;
    HandleChildOutput(buf, 0, retryUpdate, upParams);
    EXPECT_EQ(retryUpdate, false);
    HandleChildOutput(buf, buf.size(), retryUpdate, upParams);
    EXPECT_EQ(retryUpdate, false);
    buf = "write_log:xxx";
    HandleChildOutput(buf, buf.size(), retryUpdate, upParams);
    EXPECT_EQ(retryUpdate, false);
    buf = "retry_update:xxx";
    HandleChildOutput(buf, buf.size(), retryUpdate, upParams);
    EXPECT_EQ(retryUpdate, true);
    buf = "ui_log:xxx";
    HandleChildOutput(buf, buf.size(), retryUpdate, upParams);
    EXPECT_EQ(retryUpdate, true);
    buf = "show_progress:xxx";
    HandleChildOutput(buf, buf.size(), retryUpdate, upParams);
    EXPECT_EQ(retryUpdate, true);
    buf = "show_progress:xxx:xxx";
    HandleChildOutput(buf, buf.size(), retryUpdate, upParams);
    EXPECT_EQ(retryUpdate, true);
    buf = "set_progress:xxx";
    HandleChildOutput(buf, buf.size(), retryUpdate, upParams);
    EXPECT_EQ(retryUpdate, true);
    buf = "xxx:xxx";
    HandleChildOutput(buf, buf.size(), retryUpdate, upParams);
    EXPECT_EQ(retryUpdate, true);
}

HWTEST_F(UpdaterUtilUnitTest, InstallUpdaterPackageTest, TestSize.Level1)
{
    UpdaterParams upParams;
    upParams.retryCount = 0;
    upParams.callbackProgress = [] (float value) { UPDATER_UI_INSTANCE.ShowProgress(value); };
    upParams.updatePackage.push_back("/data/updater/updater/updater_full.zip");
    Hpackage::PkgManager::PkgManagerPtr pkgManager = Hpackage::PkgManager::CreatePackageInstance();
    EXPECT_EQ(InstallUpdaterPackage(upParams, pkgManager), UPDATE_ERROR);
    upParams.updateMode = SDCARD_UPDATE;
    upParams.retryCount = 1;
    EXPECT_EQ(InstallUpdaterPackage(upParams, pkgManager), UPDATE_ERROR);
}

HWTEST_F(UpdaterUtilUnitTest, DoUpdatePackagesTest, TestSize.Level1)
{
    UpdaterParams upParams;
    EXPECT_EQ(DoUpdatePackages(upParams), UPDATE_ERROR);
    upParams.updatePackage.push_back("/data/updater/updater/updater_full.zip");
    EXPECT_EQ(DoUpdatePackages(upParams), UPDATE_CORRUPT);
    upParams.callbackProgress = [] (float value) { UPDATER_UI_INSTANCE.ShowProgress(value); };
    upParams.installTime.push_back(std::chrono::duration<double>(0));
    EXPECT_EQ(DoUpdatePackages(upParams), UPDATE_ERROR);
}

HWTEST_F(UpdaterUtilUnitTest, StartUpdaterEntryTest, TestSize.Level1)
{
    UpdaterParams upParams;
    upParams.factoryResetMode = "factory_wipe_data";
    EXPECT_EQ(DoUpdatePackages(upParams), UPDATE_ERROR);
    upParams.factoryResetMode = "user_wipe_data";
    EXPECT_EQ(DoUpdatePackages(upParams), UPDATE_ERROR);
    upParams.factoryResetMode = "menu_wipe_data";
    EXPECT_EQ(DoUpdatePackages(upParams), UPDATE_ERROR);
    upParams.factoryResetMode = "";
    EXPECT_EQ(DoUpdatePackages(upParams), UPDATE_ERROR);
}

HWTEST_F(UpdaterUtilUnitTest, StartUpdaterProcTest, TestSize.Level1)
{
    Hpackage::PkgManager::PkgManagerPtr pkgManager = Hpackage::PkgManager::CreatePackageInstance();
    UpdaterParams upParams;
    upParams.isLoadReduction = true;
    EXPECT_EQ(StartUpdaterProc(nullptr, upParams), UPDATE_CORRUPT);
    EXPECT_EQ(StartUpdaterProc(pkgManager, upParams), UPDATE_ERROR);
}

HWTEST_F(UpdaterUtilUnitTest, GetSdcardPkgsFromDev, TestSize.Level0)
{
    UpdaterParams upParams;
    EXPECT_EQ(GetSdcardPkgsFromDev(upParams), UPDATE_ERROR);
}

HWTEST_F(UpdaterUtilUnitTest, DoMountSdCard, TestSize.Level0)
{
    UpdaterParams upParams;
    std::string str = "";
    std::vector<std::string> vec;
    EXPECT_EQ(DoMountSdCard(vec, str, upParams), true);
}

HWTEST_F(UpdaterUtilUnitTest, GetSdcardPkgsPath, TestSize.Level0)
{
    UpdaterParams upParams;
    EXPECT_EQ(GetSdcardPkgsPath(upParams), UPDATE_SUCCESS);

    upParams.updatePackage.push_back("/sdcard/updater/updater_full.zip");
    EXPECT_EQ(GetSdcardPkgsPath(upParams), UPDATE_SUCCESS);
    upParams.updatePackage.clear();
}

HWTEST_F(UpdaterUtilUnitTest, CheckSdcardPkgs, TestSize.Level0)
{
    UpdaterParams upParams;
    EXPECT_EQ(CheckSdcardPkgs(upParams), UPDATE_SUCCESS);
    upParams.updatePackage.clear();

    upParams.updatePackage.push_back("/sdcard/updater/updater_full.zip");
    EXPECT_EQ(CheckSdcardPkgs(upParams), UPDATE_SUCCESS);
}

HWTEST_F(UpdaterUtilUnitTest, OtaUpdatePreCheckTest, TestSize.Level1)
{
    Hpackage::PkgManager::PkgManagerPtr pkgManager = nullptr;
    std::string packagePath = "/data/updater/updater_full.zip";
    int32_t ret = OtaUpdatePreCheck(pkgManager, packagePath);
    EXPECT_EQ(ret, UPDATE_CORRUPT);

    pkgManager = Hpackage::PkgManager::CreatePackageInstance();
    ret = OtaUpdatePreCheck(pkgManager, "/nonexistent/path/updater.zip");
    EXPECT_EQ(ret, Hpackage::PKG_INVALID_FILE);
    Hpackage::PkgManager::ReleasePackageInstance(pkgManager);
}

HWTEST_F(UpdaterUtilUnitTest, IsBatteryCapacitySufficientTest, TestSize.Level1)
{
    bool ret = IsBatteryCapacitySufficient();
    EXPECT_EQ(ret, true);
}

HWTEST_F(UpdaterUtilUnitTest, CheckStatvfsTest, TestSize.Level1)
{
    int ret = CheckStatvfs(0);
    EXPECT_TRUE(ret == UPDATE_SUCCESS || ret == UPDATE_ERROR);
}

HWTEST_F(UpdaterUtilUnitTest, GetCurrentPackagePathTest, TestSize.Level1)
{
    UpdaterParams upParams;
    upParams.pkgLocation = 0;
    upParams.updatePackage.push_back("/data/updater/test.zip");
    std::string path = GetCurrentPackagePath(upParams);
    EXPECT_EQ(path, "/data/updater/test.zip");

    upParams.pkgLocation = 10;
    path = GetCurrentPackagePath(upParams);
    EXPECT_EQ(path, "");
}

HWTEST_F(UpdaterUtilUnitTest, SetUpdateSlotParamTest, TestSize.Level1)
{
    UpdaterParams upParams;
    upParams.updatePackage.push_back("/data/updater/updater.zip");
    UpdaterStatus ret = SetUpdateSlotParam(upParams, false);
    EXPECT_EQ(ret, UPDATE_SUCCESS);
}

HWTEST_F(UpdaterUtilUnitTest, ClearUpdateSlotParamTest, TestSize.Level1)
{
    UpdaterStatus ret = ClearUpdateSlotParam();
    EXPECT_EQ(ret, UPDATE_SUCCESS);
}

HWTEST_F(UpdaterUtilUnitTest, IsUpdateBasePkgTest, TestSize.Level1)
{
    UpdaterParams upParams;
    upParams.updatePackage.push_back("/data/updater/update_base.zip");
    bool ret = IsUpdateBasePkg(upParams);
    EXPECT_EQ(ret, true);

    upParams.updatePackage.clear();
    upParams.updatePackage.push_back("/data/updater/update_full.zip");
    ret = IsUpdateBasePkg(upParams);
    EXPECT_EQ(ret, false);
}

HWTEST_F(UpdaterUtilUnitTest, ProgressSmoothHandlerTest, TestSize.Level1)
{
    ProgressSmoothHandler(0, FULL_PERCENT_PROGRESS);
    ProgressSmoothHandler(50, 100);
    ProgressSmoothHandler(-1, 0);
    ProgressSmoothHandler(0, FULL_PERCENT_PROGRESS + 1);
}

HWTEST_F(UpdaterUtilUnitTest, SetAndGetTmpProgressValueTest, TestSize.Level1)
{
    SetTmpProgressValue(50);
    EXPECT_EQ(GetTmpProgressValue(), 50);
    SetTmpProgressValue(100);
    EXPECT_EQ(GetTmpProgressValue(), 100);
}

HWTEST_F(UpdaterUtilUnitTest, SetAndGetTotalProgressRatioTest, TestSize.Level1)
{
    SetTotalProgressRatio(0.5f);
    EXPECT_EQ(GetTotalProgressRatio(), 0.5f);
    SetTotalProgressRatio(1.0f);
    EXPECT_EQ(GetTotalProgressRatio(), 1.0f);
}

HWTEST_F(UpdaterUtilUnitTest, SetAndGetCancelStatusTest, TestSize.Level1)
{
    SetCancelStatus(true);
    EXPECT_EQ(GetCancelStatus(), true);
    SetCancelStatus(false);
    EXPECT_EQ(GetCancelStatus(), false);
}

HWTEST_F(UpdaterUtilUnitTest, PreProcessTest, TestSize.Level1)
{
    UpdaterParams upParams;
    Hpackage::PkgManager::PkgManagerPtr pkgManager = nullptr;
    int32_t ret = Updater::PreProcess::GetInstance().DoUpdatePreProcess(upParams, pkgManager);
    EXPECT_EQ(ret, 109);

    ret = Updater::PreProcess::GetInstance().DoUpdateAuth("");
    EXPECT_EQ(ret, 0);

    ret = Updater::PreProcess::GetInstance().DoUpdateClear();
    EXPECT_EQ(ret, 0);
}

HWTEST_F(UpdaterUtilUnitTest, CheckVersionTest, TestSize.Level1)
{
    Hpackage::PkgManager::PkgManagerPtr pkgManager = nullptr;
    PackagesInfoPtr pkginfomanager = nullptr;
    int ret = CheckVersion(pkgManager, pkginfomanager);
    EXPECT_EQ(ret, Hpackage::PKG_INVALID_VERSION);

    pkgManager = Hpackage::PkgManager::CreatePackageInstance();
    ret = CheckVersion(pkgManager, pkginfomanager);
    EXPECT_EQ(ret, Hpackage::PKG_INVALID_VERSION);
    Hpackage::PkgManager::ReleasePackageInstance(pkgManager);
}

HWTEST_F(UpdaterUtilUnitTest, CheckBoardIdTest, TestSize.Level1)
{
    Hpackage::PkgManager::PkgManagerPtr pkgManager = nullptr;
    PackagesInfoPtr pkginfomanager = nullptr;
    int ret = CheckBoardId(pkgManager, pkginfomanager);
    EXPECT_EQ(ret, Hpackage::PKG_INVALID_VERSION);

    pkgManager = Hpackage::PkgManager::CreatePackageInstance();
    ret = CheckBoardId(pkgManager, pkginfomanager);
    EXPECT_EQ(ret, Hpackage::PKG_INVALID_VERSION);
    Hpackage::PkgManager::ReleasePackageInstance(pkgManager);
}

HWTEST_F(UpdaterUtilUnitTest, GetCpuCoresTest, TestSize.Level1)
{
    UpdaterParams upParams;
    upParams.cpuTypeCores = {4, 2, 2};
    unsigned int cores = GetCpuCores(upParams, 0);
    EXPECT_EQ(cores, 4);
    cores = GetCpuCores(upParams, 1);
    EXPECT_EQ(cores, 2);
    cores = GetCpuCores(upParams, 2);
    EXPECT_EQ(cores, 2);
    cores = GetCpuCores(upParams, -1);
    EXPECT_EQ(cores, 4);
    cores = GetCpuCores(upParams, 10);
    EXPECT_EQ(cores, 4);
}

HWTEST_F(UpdaterUtilUnitTest, AddBinaryTidsTest, TestSize.Level1)
{
    UpdaterParams upParams;
    AddBinaryTids(upParams, 12345);
    EXPECT_EQ(upParams.binaryTids.size(), 1);
    EXPECT_EQ(upParams.binaryTids[0], "12345");
}

HWTEST_F(UpdaterUtilUnitTest, SetCpuAffinityByPidTest, TestSize.Level1)
{
    UpdaterParams upParams;
    upParams.binaryPid = -1;
    bool ret = SetCpuAffinityByPid(upParams, 1);
    EXPECT_EQ(ret, false);

    upParams.binaryPid = 12345;
    upParams.binaryTids = {"99999"};
    ret = SetCpuAffinityByPid(upParams, 1);
    EXPECT_EQ(ret, false);
}

HWTEST_F(UpdaterUtilUnitTest, GetStashSizeListTest, TestSize.Level1)
{
    UpdaterParams upParams;
    upParams.pkgLocation = 0;
    upParams.updatePackage.push_back("/data/updater/updater/updater_full.zip");
    std::vector<uint64_t> stashList = GetStashSizeList(upParams);
    EXPECT_TRUE(stashList.size() > 0);
}

HWTEST_F(UpdaterUtilUnitTest, GetWorkPathTest, TestSize.Level1)
{
    std::string path = GetWorkPath();
    EXPECT_FALSE(path.empty());
}

HWTEST_F(UpdaterUtilUnitTest, UpdateBinaryTidsTest, TestSize.Level1)
{
    UpdaterParams upParams;
    std::vector<std::string> output = {"set_binary_tids", "12345,67890"};
    UpdateBinaryTids(output, upParams);
    EXPECT_EQ(upParams.binaryTids.size(), 2);

    output = {"set_binary_tids"};
    UpdateBinaryTids(output, upParams);
}

HWTEST_F(UpdaterUtilUnitTest, WriteInstallTimeTest, TestSize.Level1)
{
    UpdaterParams upParams;
    upParams.pkgLocation = 0;
    upParams.installTime.push_back(std::chrono::duration<double>(1.5));
    WriteInstallTime(upParams);
}

HWTEST_F(UpdaterUtilUnitTest, ReadInstallTimeTest, TestSize.Level1)
{
    UpdaterParams upParams;
    upParams.pkgLocation = 0;
    upParams.installTime.push_back(std::chrono::duration<double>(0));
    ReadInstallTime(upParams);
}

HWTEST_F(UpdaterUtilUnitTest, UpdatePreProcessTest, TestSize.Level1)
{
    UpdaterParams upParams;
    Hpackage::PkgManager::PkgManagerPtr pkgManager = nullptr;
    int32_t ret = UpdatePreProcess(upParams, pkgManager);
    EXPECT_EQ(ret, Hpackage::PKG_INVALID_VERSION);

    pkgManager = Hpackage::PkgManager::CreatePackageInstance();
    ret = UpdatePreProcess(upParams, pkgManager);
    Hpackage::PkgManager::ReleasePackageInstance(pkgManager);
}

HWTEST_F(UpdaterUtilUnitTest, RegisterModeTest, TestSize.Level1)
{
    GetBootModes().clear();
    auto dummyEntry = [] (int argc, char **argv) -> int { return 0; };
    RegisterMode({ IsFlashd, "TEST_MODE", "", dummyEntry });
    EXPECT_EQ(GetBootModes().size(), 1);
}

HWTEST_F(UpdaterUtilUnitTest, DoUpdaterEntryTest, TestSize.Level1)
{
    UpdaterParams upParams;
    upParams.updateMode = HOTA_UPDATE;
    upParams.updatePackage.clear();
    upParams.factoryResetMode = "";
    UpdaterStatus ret = DoUpdaterEntry(upParams);
    EXPECT_EQ(ret, UPDATE_UNKNOWN);

    upParams.factoryResetMode = "secure_erase";
    ret = DoUpdaterEntry(upParams);

    upParams.factoryResetMode = "disk_erase";
    ret = DoUpdaterEntry(upParams);
}

HWTEST_F(UpdaterUtilUnitTest, StartUpdaterEntryTest_SubPkg, TestSize.Level1)
{
    UpdaterParams upParams;
    upParams.updateMode = SUBPKG_UPDATE;
    UpdaterStatus ret = StartUpdaterEntry(upParams);
    EXPECT_TRUE(ret == UPDATE_SUCCESS || ret == UPDATE_ERROR);
}

HWTEST_F(UpdaterUtilUnitTest, InstallUpdaterPackagesTest, TestSize.Level1)
{
    UpdaterParams upParams;
    upParams.callbackProgress = nullptr;
    UpdaterStatus ret = InstallUpdaterPackages(upParams);
    EXPECT_FALSE(ret == UPDATE_SUCCESS || ret == UPDATE_ERROR || ret == UPDATE_SKIP);
}

HWTEST_F(UpdaterUtilUnitTest, GetUpdatePackageInfoTest, TestSize.Level1)
{
    Hpackage::PkgManager::PkgManagerPtr pkgManager = nullptr;
    int32_t ret = GetUpdatePackageInfo(pkgManager, "/data/updater/updater.zip");
    EXPECT_EQ(ret, UPDATE_CORRUPT);

    pkgManager = Hpackage::PkgManager::CreatePackageInstance();
    ret = GetUpdatePackageInfo(pkgManager, "/nonexistent/package.zip");
    EXPECT_NE(ret, Hpackage::PKG_SUCCESS);
    Hpackage::PkgManager::ReleasePackageInstance(pkgManager);
}

HWTEST_F(UpdaterUtilUnitTest, PostUpdaterTest, TestSize.Level1)
{
    PostUpdater(true);
    PostUpdater(false);
}

HWTEST_F(UpdaterUtilUnitTest, DeleteInstallTimeFileTest, TestSize.Level1)
{
    DeleteInstallTimeFile();
}

HWTEST_F(UpdaterUtilUnitTest, FactoryResetModeTest, TestSize.Level1)
{
    UpdaterParams upParams;
    upParams.factoryResetMode = "factory_wipe_data";
    UpdaterStatus ret = DoFactoryRstEntry(upParams);

    upParams.factoryResetMode = "user_wipe_data";
    ret = DoFactoryRstEntry(upParams);

    upParams.factoryResetMode = "menu_wipe_data";
    ret = DoFactoryRstEntry(upParams);
}
}
