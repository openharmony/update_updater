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
#include "updater_unittest.h"
#include <fstream>
#include <iostream>
#include <string>
#include "log/log.h"
#include "updater/updater.h"
 
using namespace Hpackage;
using namespace testing::ext;
using namespace UpdaterUt;
using namespace Updater;
using namespace std;
 
namespace UpdaterUt {
void UpdaterTest::SetUp(void)
{
    cout << "SetUp" << endl;
}
 
void UpdaterTest::TearDown(void)
{
    cout << "TearDown" << endl;
}
 
HWTEST_F(UpdaterTest, updatePackageIncludeBasePkg, TestSize.Level0)
{
    UpdaterParams upParams;
    upParams.updatePackage.push_back("test_base");
    EXPECT_TRUE(IsUpdateBasePkg(upParams));
}
 
HWTEST_F(UpdaterTest, updatePackageNotIncludeBasePkg, TestSize.Level0)
{
    UpdaterParams upParams;
    upParams.updatePackage.push_back("test");
    ProgressSmoothHandler(0, -1, upParams, false);
    ProgressSmoothHandler(0, 101, upParams, false);
    ProgressSmoothHandler(-1, 50, upParams, false);
    ProgressSmoothHandler(50, 50, upParams, false);
    EXPECT_FALSE(IsUpdateBasePkg(upParams));
}
 
HWTEST_F(UpdaterTest, updatePackageIsEmpty, TestSize.Level0)
{
    UpdaterParams upParams;
    EXPECT_FALSE(IsUpdateBasePkg(upParams));
}
 
HWTEST_F(UpdaterTest, getUpdatePackageInfo_NullPkgManager_Test, TestSize.Level0)
{
    std::string path = "/data/test";
    int ret = GetUpdatePackageInfo(nullptr, path);
    EXPECT_EQ(ret, UPDATE_CORRUPT);
}
 
HWTEST_F(UpdaterTest, getUpdatePackageInfo_LoadPackageFail_Test, TestSize.Level0)
{
    PkgManager::PkgManagerPtr pkgManager = PkgManager::CreatePackageInstance();
    std::string path = "/data/invalid_path";
    int ret = GetUpdatePackageInfo(pkgManager, path);
    EXPECT_EQ(ret, PKG_INVALID_FILE);
    PkgManager::ReleasePackageInstance(pkgManager);
}
} // namespace updater_ut