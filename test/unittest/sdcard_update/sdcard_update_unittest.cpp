/*
 * Copyright (c) 2022-2026 Huawei Device Co., Ltd.
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
#include <gmock/gmock.h>
#include <memory>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "log/log.h"
#include "securec.h"
#include "updater/updater_const.h"
#include "updater/updater.h"
#include "sdcard_update.h"
#include "sdcard_update_action.h"
#include "sdcard_update_adapter_interface.h"
#include "sdcard_update_adapter.h"
#include "sdcard_update_chain_builder.h"
#include "sdcard_update_group.h"
#include "sdcard_update_process_manager.h"
#include "sdcard_update_step.h"
#include "utils.h"

using namespace Updater;
using namespace std;
using namespace testing::ext;
using namespace testing;

namespace {

class MockSdcardUpdateAdapter : public ISdcardUpdateAdapter {
public:
    MockSdcardUpdateAdapter() = default;
    ~MockSdcardUpdateAdapter() = default;

    MOCK_METHOD(int32_t, MountSdcardPath, (const std::string &path, const std::string &mountPoint), (override));
    MOCK_METHOD(int32_t, UmountPath, (const std::string &path), (override));
    MOCK_METHOD(int32_t, MountPath, (const std::string &path), (override));
    MOCK_METHOD(const std::vector<std::string>, GetBlockDevices, (const std::string &mountPoint), (override));
    MOCK_METHOD(std::string, GetVc, (), (override));
    MOCK_METHOD(std::string, GetDevModel, (), (override));
    MOCK_METHOD(std::string, GetOemMode, (), (override));
    MOCK_METHOD(bool, IsMountPathSuccess, (const std::string &path), (override));
};

class SdcardUpdateUnittest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() override
    {
        InitUpdaterLogger("UPDATER", "updater_log.log", "updater_status.log", "error_code.log");
    }
    void TearDown() override {}
};

UpdaterStatus DummyFindPkgSuccess(UpdaterParams &upParams)
{
    return UPDATE_SUCCESS;
}

UpdaterStatus DummyFindPkgError(UpdaterParams &upParams)
{
    return UPDATE_ERROR;
}

HWTEST_F(SdcardUpdateUnittest, SdcardUpdateStartFindPkg_Normal, TestSize.Level1)
{
    UpdaterParams upParams {};
    upParams.updatePackage.clear();
    upParams.updatePackage.push_back("/sdcard/updater/updater.zip");
    UpdaterStatus status = SdcardUpdate(upParams);
    EXPECT_EQ(status, UPDATE_SUCCESS);
}

HWTEST_F(SdcardUpdateUnittest, SdcardUpdateStartFindPkg_PackageEmpty, TestSize.Level1)
{
    UpdaterParams upParams {};
    upParams.updatePackage.clear();
    UpdaterStatus status = SdcardUpdate(upParams);
    EXPECT_EQ(status, UPDATE_SUCCESS);
}

HWTEST_F(SdcardUpdateUnittest, FindSdPkgAction_Execute_WithPackage, TestSize.Level1)
{
    UpdaterParams upParams {};
    upParams.updatePackage.clear();
    upParams.updatePackage.push_back("/sdcard/updater/updater.zip");
    auto action = std::make_unique<FindSdPkgAction>();
    UpdaterStatus status = action->Execute(upParams);
    EXPECT_EQ(status, UPDATE_SUCCESS);
}

HWTEST_F(SdcardUpdateUnittest, FindSdPkgAction_Execute_EmptyPackage, TestSize.Level1)
{
    UpdaterParams upParams {};
    upParams.updatePackage.clear();
    auto action = std::make_unique<FindSdPkgAction>();
    UpdaterStatus status = action->Execute(upParams);
    EXPECT_EQ(status, UPDATE_SUCCESS);
}

HWTEST_F(SdcardUpdateUnittest, FindSdPkgAction_GetSdcardPkgsPath_FromMisc, TestSize.Level1)
{
    UpdaterParams upParams {};
    upParams.updatePackage.clear();
    upParams.updatePackage.push_back("/data/updater/updater.zip");
    auto action = std::make_unique<FindSdPkgAction>();
    UpdaterStatus status = action->Execute(upParams);
    EXPECT_EQ(status, UPDATE_SUCCESS);
}

HWTEST_F(SdcardUpdateUnittest, FindSdPkgAction_WithMockAdapter_EmptyDevices, TestSize.Level1)
{
    auto action = std::make_unique<FindSdPkgAction>();
    auto mockAdapter = std::make_unique<MockSdcardUpdateAdapter>();
    EXPECT_CALL(*mockAdapter, GetBlockDevices(_))
        .WillRepeatedly(Return(std::vector<std::string>{}));
    action->SetAdapter(std::move(mockAdapter));
    UpdaterParams upParams {};
    upParams.updatePackage.clear();
    UpdaterStatus status = action->Execute(upParams);
    EXPECT_EQ(status, UPDATE_SUCCESS);
}

HWTEST_F(SdcardUpdateUnittest, FindSdPkgAction_WithMockAdapter_WithDevices, TestSize.Level1)
{
    auto action = std::make_unique<FindSdPkgAction>();
    auto mockAdapter = std::make_unique<MockSdcardUpdateAdapter>();
    std::string path = "/dev/block/sda1";
    EXPECT_CALL(*mockAdapter, GetBlockDevices(_))
        .WillRepeatedly(Return(std::vector<std::string>{path}));
    EXPECT_CALL(*mockAdapter, MountSdcardPath(_, _))
        .WillRepeatedly(Return(0));
    EXPECT_CALL(*mockAdapter, UmountPath(_))
        .WillRepeatedly(Return(0));
    action->SetAdapter(std::move(mockAdapter));
    UpdaterParams upParams {};
    upParams.updatePackage.clear();
    UpdaterStatus status = action->Execute(upParams);
    EXPECT_EQ(status, UPDATE_SUCCESS);
}

HWTEST_F(SdcardUpdateUnittest, MountSdCardAction_MountSuccess, TestSize.Level1)
{
    auto action = std::make_unique<MountSdCardAction>("sda1", "/sdcard");
    auto mockAdapter = std::make_unique<MockSdcardUpdateAdapter>();
    EXPECT_CALL(*mockAdapter, MountSdcardPath(_, _))
        .WillRepeatedly(Return(0));
    action->SetAdapter(std::move(mockAdapter));
    UpdaterParams upParams {};
    UpdaterStatus status = action->Execute(upParams);
    EXPECT_EQ(status, UPDATE_SUCCESS);
}

HWTEST_F(SdcardUpdateUnittest, MountSdCardAction_MountFailed, TestSize.Level1)
{
    auto action = std::make_unique<MountSdCardAction>("sda1", "/sdcard");
    auto mockAdapter = std::make_unique<MockSdcardUpdateAdapter>();
    EXPECT_CALL(*mockAdapter, MountSdcardPath(_, _))
        .WillRepeatedly(Return(-1));
    action->SetAdapter(std::move(mockAdapter));
    UpdaterParams upParams {};
    UpdaterStatus status = action->Execute(upParams);
    EXPECT_EQ(status, UPDATE_ERROR);
}

HWTEST_F(SdcardUpdateUnittest, MountPathAction_MountSuccess, TestSize.Level1)
{
    auto action = std::make_unique<MountPathAction>();
    auto mockAdapter = std::make_unique<MockSdcardUpdateAdapter>();
    EXPECT_CALL(*mockAdapter, MountPath(_))
        .WillRepeatedly(Return(0));
    action->SetAdapter(std::move(mockAdapter));
    UpdaterParams upParams {};
    UpdaterStatus status = action->Execute(upParams);
    EXPECT_EQ(status, UPDATE_SUCCESS);
}

HWTEST_F(SdcardUpdateUnittest, MountPathAction_MountFailed, TestSize.Level1)
{
    auto action = std::make_unique<MountPathAction>();
    auto mockAdapter = std::make_unique<MockSdcardUpdateAdapter>();
    EXPECT_CALL(*mockAdapter, MountPath(_))
        .WillRepeatedly(Return(-1));
    action->SetAdapter(std::move(mockAdapter));
    UpdaterParams upParams {};
    UpdaterStatus status = action->Execute(upParams);
    EXPECT_EQ(status, UPDATE_ERROR);
}

HWTEST_F(SdcardUpdateUnittest, UmountPathAction_NotMounted, TestSize.Level1)
{
    auto action = std::make_unique<UmountPathAction>("/sdcard");
    auto mockAdapter = std::make_unique<MockSdcardUpdateAdapter>();
    EXPECT_CALL(*mockAdapter, IsMountPathSuccess(_))
        .WillRepeatedly(Return(false));
    action->SetAdapter(std::move(mockAdapter));
    UpdaterParams upParams {};
    UpdaterStatus status = action->Execute(upParams);
    EXPECT_EQ(status, UPDATE_ERROR);
}

HWTEST_F(SdcardUpdateUnittest, UmountPathAction_IsMountedUmountFailed, TestSize.Level1)
{
    auto action = std::make_unique<UmountPathAction>("/sdcard");
    auto mockAdapter = std::make_unique<MockSdcardUpdateAdapter>();
    EXPECT_CALL(*mockAdapter, IsMountPathSuccess(_))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mockAdapter, UmountPath(_))
        .WillRepeatedly(Return(-1));
    action->SetAdapter(std::move(mockAdapter));
    UpdaterParams upParams {};
    UpdaterStatus status = action->Execute(upParams);
    EXPECT_EQ(status, UPDATE_ERROR);
}

HWTEST_F(SdcardUpdateUnittest, UmountPathAction_IsMountedUmountSuccess, TestSize.Level1)
{
    auto action = std::make_unique<UmountPathAction>("/sdcard");
    auto mockAdapter = std::make_unique<MockSdcardUpdateAdapter>();
    EXPECT_CALL(*mockAdapter, IsMountPathSuccess(_))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mockAdapter, UmountPath(_))
        .WillRepeatedly(Return(0));
    action->SetAdapter(std::move(mockAdapter));
    UpdaterParams upParams {};
    UpdaterStatus status = action->Execute(upParams);
    EXPECT_EQ(status, UPDATE_ERROR);
}

HWTEST_F(SdcardUpdateUnittest, UpdateChainBuilder_BuildChain_SingleAction, TestSize.Level1)
{
    auto chain = UpdateChainBuilder()
        .Add(std::make_unique<FindSdPkgAction>())
        .Build();
    ASSERT_NE(chain, nullptr);
    UpdaterParams upParams {};
    upParams.updatePackage.push_back("/sdcard/updater/updater.zip");
    UpdaterStatus status = chain->Handle(upParams);
    EXPECT_EQ(status, UPDATE_SUCCESS);
}

HWTEST_F(SdcardUpdateUnittest, UpdateChainBuilder_BuildChain_MultipleActions, TestSize.Level1)
{
    auto chain = UpdateChainBuilder()
        .Add(std::make_unique<FindSdPkgAction>())
        .Add(std::make_unique<MountSdCardAction>())
        .Build();
    ASSERT_NE(chain, nullptr);
    UpdaterParams upParams {};
    upParams.updatePackage.push_back("/sdcard/updater/updater.zip");
    UpdaterStatus status = chain->Handle(upParams);
    EXPECT_EQ(status, UPDATE_ERROR);
}

HWTEST_F(SdcardUpdateUnittest, UpdateChainBuilder_BuildChain_Empty, TestSize.Level1)
{
    auto chain = UpdateChainBuilder().Build();
    EXPECT_EQ(chain, nullptr);
}

HWTEST_F(SdcardUpdateUnittest, UpdateChainBuilder_NextGroup_SingleGroup, TestSize.Level1)
{
    auto chain = UpdateChainBuilder()
        .Add(std::make_unique<FindSdPkgAction>())
        .NextGroup()
        .Add(std::make_unique<MountSdCardAction>())
        .Build();
    ASSERT_NE(chain, nullptr);
    UpdaterParams upParams {};
    upParams.updatePackage.push_back("/sdcard/updater/updater.zip");
    UpdaterStatus status = chain->Handle(upParams);
    EXPECT_EQ(status, UPDATE_SUCCESS);
}

HWTEST_F(SdcardUpdateUnittest, UpdateChainBuilder_NextGroup_MultipleGroups, TestSize.Level1)
{
    auto chain = UpdateChainBuilder()
        .Add(std::make_unique<FindSdPkgAction>())
        .NextGroup()
        .Add(std::make_unique<MountSdCardAction>())
        .NextGroup()
        .Add(std::make_unique<FindSdPkgAction>())
        .Build();
    ASSERT_NE(chain, nullptr);
    UpdaterParams upParams {};
    upParams.updatePackage.push_back("/sdcard/updater/updater.zip");
    UpdaterStatus status = chain->Handle(upParams);
    EXPECT_EQ(status, UPDATE_SUCCESS);
}

HWTEST_F(SdcardUpdateUnittest, UpdateChainBuilder_NextGroupTwice, TestSize.Level1)
{
    auto chain = UpdateChainBuilder()
        .Add(std::make_unique<FindSdPkgAction>())
        .NextGroup()
        .NextGroup()
        .Add(std::make_unique<MountSdCardAction>())
        .Build();
    ASSERT_NE(chain, nullptr);
    UpdaterParams upParams {};
    upParams.updatePackage.push_back("/sdcard/updater/updater.zip");
    UpdaterStatus status = chain->Handle(upParams);
    EXPECT_EQ(status, UPDATE_SUCCESS);
}

HWTEST_F(SdcardUpdateUnittest, UpdateGroup_AddStep_SingleStep, TestSize.Level1)
{
    auto group = std::make_shared<UpdateGroup>();
    auto action = std::make_unique<FindSdPkgAction>();
    group->AddStep(std::make_shared<UpdateStep>(std::move(action)));
    UpdaterParams upParams {};
    upParams.updatePackage.push_back("/sdcard/updater/updater.zip");
    UpdaterStatus status = group->Handle(upParams);
    EXPECT_EQ(status, UPDATE_SUCCESS);
}

HWTEST_F(SdcardUpdateUnittest, UpdateGroup_AddStep_MultipleSteps, TestSize.Level1)
{
    auto group = std::make_shared<UpdateGroup>();
    auto action1 = std::make_unique<FindSdPkgAction>();
    auto action2 = std::make_unique<FindSdPkgAction>();
    group->AddStep(std::make_shared<UpdateStep>(std::move(action1)));
    group->AddStep(std::make_shared<UpdateStep>(std::move(action2)));
    UpdaterParams upParams {};
    upParams.updatePackage.push_back("/sdcard/updater/updater.zip");
    UpdaterStatus status = group->Handle(upParams);
    EXPECT_EQ(status, UPDATE_SUCCESS);
}

HWTEST_F(SdcardUpdateUnittest, UpdateGroup_Handle_EmptySteps_NoNextGroup, TestSize.Level1)
{
    auto group = std::make_shared<UpdateGroup>();
    UpdaterParams upParams {};
    upParams.updatePackage.push_back("/sdcard/updater/updater.zip");
    UpdaterStatus status = group->Handle(upParams);
    EXPECT_EQ(status, UPDATE_ERROR);
}

HWTEST_F(SdcardUpdateUnittest, UpdateGroup_Handle_EmptySteps_WithNextGroup, TestSize.Level1)
{
    auto group1 = std::make_shared<UpdateGroup>();
    auto group2 = std::make_shared<UpdateGroup>();
    auto action2 = std::make_unique<FindSdPkgAction>();
    group2->AddStep(std::make_shared<UpdateStep>(std::move(action2)));
    group1->SetNextGroup(group2);
    UpdaterParams upParams {};
    upParams.updatePackage.push_back("/sdcard/updater/updater.zip");
    UpdaterStatus status = group1->Handle(upParams);
    EXPECT_EQ(status, UPDATE_SUCCESS);
}

HWTEST_F(SdcardUpdateUnittest, UpdateGroup_SetNextGroup, TestSize.Level1)
{
    auto group1 = std::make_shared<UpdateGroup>();
    auto group2 = std::make_shared<UpdateGroup>();
    auto action2 = std::make_unique<FindSdPkgAction>();
    group2->AddStep(std::make_shared<UpdateStep>(std::move(action2)));
    group1->SetNextGroup(group2);
    UpdaterParams upParams {};
    upParams.updatePackage.push_back("/sdcard/updater/updater.zip");
    UpdaterStatus status = group1->Handle(upParams);
    EXPECT_EQ(status, UPDATE_SUCCESS);
}

HWTEST_F(SdcardUpdateUnittest, UpdateGroup_SetNextGroupChaining, TestSize.Level1)
{
    auto group1 = std::make_shared<UpdateGroup>();
    auto group2 = std::make_shared<UpdateGroup>();
    auto group3 = std::make_shared<UpdateGroup>();
    auto action1 = std::make_unique<FindSdPkgAction>();
    auto action3 = std::make_unique<FindSdPkgAction>();
    group1->AddStep(std::make_shared<UpdateStep>(std::move(action1)));
    group3->AddStep(std::make_shared<UpdateStep>(std::move(action3)));
    group1->SetNextGroup(group2);
    group2->SetNextGroup(group3);
    UpdaterParams upParams {};
    upParams.updatePackage.push_back("/sdcard/updater/updater.zip");
    UpdaterStatus status = group1->Handle(upParams);
    EXPECT_EQ(status, UPDATE_SUCCESS);
}

HWTEST_F(SdcardUpdateUnittest, UpdateStep_SetNext, TestSize.Level1)
{
    auto action = std::make_unique<FindSdPkgAction>();
    auto step1 = std::make_shared<UpdateStep>(std::move(action));
    auto action2 = std::make_unique<FindSdPkgAction>();
    auto step2 = std::make_shared<UpdateStep>(std::move(action2));
    step1->SetNext(step2);
    UpdaterParams upParams {};
    upParams.updatePackage.push_back("/sdcard/updater/updater.zip");
    UpdaterStatus status = step1->Handle(upParams);
    EXPECT_EQ(status, UPDATE_SUCCESS);
}

HWTEST_F(SdcardUpdateUnittest, UpdateStep_Handle_NoNext, TestSize.Level1)
{
    auto action = std::make_unique<FindSdPkgAction>();
    auto step = std::make_shared<UpdateStep>(std::move(action));
    UpdaterParams upParams {};
    upParams.updatePackage.push_back("/sdcard/updater/updater.zip");
    UpdaterStatus status = step->Handle(upParams);
    EXPECT_EQ(status, UPDATE_SUCCESS);
}

HWTEST_F(SdcardUpdateUnittest, SdcardUpdateProcessManager_GetInstance, TestSize.Level1)
{
    auto &instance1 = SdcardUpdateProcessManager::GetInstance();
    auto &instance2 = SdcardUpdateProcessManager::GetInstance();
    EXPECT_EQ(&instance1, &instance2);
}

HWTEST_F(SdcardUpdateUnittest, SdcardUpdateProcessManager_SetDefaultFunc, TestSize.Level1)
{
    auto &manager = SdcardUpdateProcessManager::GetInstance();
    manager.SetDefaultSdcardUpdateFunc(DummyFindPkgSuccess);
    UpdaterParams upParams {};
    upParams.updatePackage.push_back("/sdcard/updater/updater.zip");
    UpdaterStatus status = manager.SdcardUpdateProcess(upParams);
    EXPECT_EQ(status, UPDATE_SUCCESS);
}

HWTEST_F(SdcardUpdateUnittest, SdcardUpdateProcessManager_SetFunc, TestSize.Level1)
{
    auto &manager = SdcardUpdateProcessManager::GetInstance();
    manager.SetSdcardUpdateFunc(DummyFindPkgSuccess);
    UpdaterParams upParams {};
    UpdaterStatus status = manager.SdcardUpdateProcess(upParams);
    EXPECT_EQ(status, UPDATE_SUCCESS);
}

HWTEST_F(SdcardUpdateUnittest, SdcardUpdateProcessManager_RegisterSdUpdateMap_Normal, TestSize.Level1)
{
    auto &manager = SdcardUpdateProcessManager::GetInstance();
    manager.RegisterSdUpdateMap("sdcard", DummyFindPkgSuccess);
    UpdaterParams upParams {};
    upParams.updatePackage.push_back("/sdcard/updater/updater.zip");
    UpdaterStatus status = manager.SdcardUpdateProcess(upParams);
    EXPECT_EQ(status, UPDATE_SUCCESS);
}

HWTEST_F(SdcardUpdateUnittest, SdcardUpdateProcessManager_RegisterSdUpdateMap_Nullptr, TestSize.Level1)
{
    auto &manager = SdcardUpdateProcessManager::GetInstance();
    manager.RegisterSdUpdateMap("sdcard", nullptr);
    UpdaterParams upParams {};
    UpdaterStatus status = manager.SdcardUpdateProcess(upParams);
    EXPECT_EQ(status, UPDATE_SUCCESS);
}

HWTEST_F(SdcardUpdateUnittest, SdcardUpdateProcessManager_RegisterSdUpdateExtMap_Normal, TestSize.Level1)
{
    auto &manager = SdcardUpdateProcessManager::GetInstance();
    manager.RegisterSdUpdateExtMap("sdcard_ext", DummyFindPkgSuccess);
    manager.ProcessSdcardUpdateExtMap();
    UpdaterParams upParams {};
    UpdaterStatus status = manager.SdcardUpdateProcess(upParams);
    EXPECT_EQ(status, UPDATE_SUCCESS);
}

HWTEST_F(SdcardUpdateUnittest, SdcardUpdateProcessManager_RegisterSdUpdateExtMap_Nullptr, TestSize.Level1)
{
    auto &manager = SdcardUpdateProcessManager::GetInstance();
    manager.RegisterSdUpdateExtMap("sdcard_ext", nullptr);
    manager.ProcessSdcardUpdateExtMap();
    UpdaterParams upParams {};
    UpdaterStatus status = manager.SdcardUpdateProcess(upParams);
    EXPECT_EQ(status, UPDATE_SUCCESS);
}

HWTEST_F(SdcardUpdateUnittest, SdcardUpdateProcessManager_ProcessExtMap_Empty, TestSize.Level1)
{
    auto &manager = SdcardUpdateProcessManager::GetInstance();
    manager.ProcessSdcardUpdateExtMap();
    UpdaterParams upParams {};
    UpdaterStatus status = manager.SdcardUpdateProcess(upParams);
    EXPECT_EQ(status, UPDATE_SUCCESS);
}

HWTEST_F(SdcardUpdateUnittest, SdcardUpdateProcessManager_InitSdUpdateFunc_UseDefault, TestSize.Level1)
{
    auto &manager = SdcardUpdateProcessManager::GetInstance();
    manager.SetDefaultSdcardUpdateFunc(DummyFindPkgSuccess);
    manager.SetSdcardUpdateFunc(nullptr);
    bool result = manager.InitSdUpdateFunc();
    EXPECT_EQ(result, true);
}

HWTEST_F(SdcardUpdateUnittest, SdcardUpdateProcessManager_InitSdUpdateFunc_UseCustom, TestSize.Level1)
{
    auto &manager = SdcardUpdateProcessManager::GetInstance();
    manager.SetSdcardUpdateFunc(DummyFindPkgSuccess);
    bool result = manager.InitSdUpdateFunc();
    EXPECT_EQ(result, true);
}

HWTEST_F(SdcardUpdateUnittest, SdcardUpdateProcessManager_InitSdUpdateFunc_AllNullptr, TestSize.Level1)
{
    auto &manager = SdcardUpdateProcessManager::GetInstance();
    manager.SetDefaultSdcardUpdateFunc(nullptr);
    manager.SetSdcardUpdateFunc(nullptr);
    bool result = manager.InitSdUpdateFunc();
    EXPECT_EQ(result, false);
}

HWTEST_F(SdcardUpdateUnittest, SdcardUpdateProcessManager_SdcardUpdateProcess_DefaultSuccess, TestSize.Level1)
{
    auto &manager = SdcardUpdateProcessManager::GetInstance();
    manager.SetDefaultSdcardUpdateFunc(DummyFindPkgSuccess);
    manager.SetSdcardUpdateFunc(nullptr);
    UpdaterParams upParams {};
    UpdaterStatus status = manager.SdcardUpdateProcess(upParams);
    EXPECT_EQ(status, UPDATE_SUCCESS);
}

HWTEST_F(SdcardUpdateUnittest, SdcardUpdateProcessManager_SdcardUpdateProcess_CustomSuccess, TestSize.Level1)
{
    auto &manager = SdcardUpdateProcessManager::GetInstance();
    manager.SetSdcardUpdateFunc(DummyFindPkgSuccess);
    UpdaterParams upParams {};
    UpdaterStatus status = manager.SdcardUpdateProcess(upParams);
    EXPECT_EQ(status, UPDATE_SUCCESS);
}

HWTEST_F(SdcardUpdateUnittest, SdcardUpdateProcessManager_SdcardUpdateProcess_Error, TestSize.Level1)
{
    auto &manager = SdcardUpdateProcessManager::GetInstance();
    manager.SetSdcardUpdateFunc(DummyFindPkgError);
    UpdaterParams upParams {};
    UpdaterStatus status = manager.SdcardUpdateProcess(upParams);
    EXPECT_EQ(status, UPDATE_ERROR);
}

HWTEST_F(SdcardUpdateUnittest, SdcardUpdateProcessManager_SdcardUpdateProcess_InitFailed, TestSize.Level1)
{
    auto &manager = SdcardUpdateProcessManager::GetInstance();
    manager.SetDefaultSdcardUpdateFunc(nullptr);
    manager.SetSdcardUpdateFunc(nullptr);
    UpdaterParams upParams {};
    UpdaterStatus status = manager.SdcardUpdateProcess(upParams);
    EXPECT_EQ(status, UPDATE_ERROR);
}

HWTEST_F(SdcardUpdateUnittest, ISdcardUpdateAction_SetAdapter, TestSize.Level1)
{
    class TestAction : public ISdcardUpdateAction {
    public:
        TestAction() : ISdcardUpdateAction() {}
        ~TestAction() override = default;
        UpdaterStatus Execute(UpdaterParams &upParams) override {
            return UPDATE_SUCCESS;
        }
    };
    auto action = std::make_unique<TestAction>();
    auto mockAdapter = std::make_unique<MockSdcardUpdateAdapter>();
    action->SetAdapter(std::move(mockAdapter));
    UpdaterParams upParams {};
    UpdaterStatus status = action->Execute(upParams);
    EXPECT_EQ(status, UPDATE_SUCCESS);
}

HWTEST_F(SdcardUpdateUnittest, IMountAction_DefaultConstructor, TestSize.Level1)
{
    auto action = std::make_unique<MountPathAction>();
    auto mockAdapter = std::make_unique<MockSdcardUpdateAdapter>();
    EXPECT_CALL(*mockAdapter, MountPath(_))
        .WillRepeatedly(Return(-1));
    action->SetAdapter(std::move(mockAdapter));
    UpdaterParams upParams {};
    UpdaterStatus status = action->Execute(upParams);
    EXPECT_EQ(status, UPDATE_ERROR);
}

HWTEST_F(SdcardUpdateUnittest, MountSdCardAction_Constructor, TestSize.Level1)
{
    auto action = std::make_unique<MountSdCardAction>("sda1", "/sdcard");
    auto mockAdapter = std::make_unique<MockSdcardUpdateAdapter>();
    EXPECT_CALL(*mockAdapter, MountSdcardPath(_, _))
        .WillRepeatedly(Return(-1));
    action->SetAdapter(std::move(mockAdapter));
    UpdaterParams upParams {};
    UpdaterStatus status = action->Execute(upParams);
    EXPECT_EQ(status, UPDATE_ERROR);
}

HWTEST_F(SdcardUpdateUnittest, UmountPathAction_Constructor, TestSize.Level1)
{
    auto action = std::make_unique<UmountPathAction>("/sdcard");
    auto mockAdapter = std::make_unique<MockSdcardUpdateAdapter>();
    EXPECT_CALL(*mockAdapter, IsMountPathSuccess(_))
        .WillRepeatedly(Return(false));
    action->SetAdapter(std::move(mockAdapter));
    UpdaterParams upParams {};
    UpdaterStatus status = action->Execute(upParams);
    EXPECT_EQ(status, UPDATE_ERROR);
}

HWTEST_F(SdcardUpdateUnittest, SdcardUpdateStartFindPkg_WithMockAdapter, TestSize.Level1)
{
    auto action = std::make_unique<FindSdPkgAction>();
    auto mockAdapter = std::make_unique<MockSdcardUpdateAdapter>();
    EXPECT_CALL(*mockAdapter, GetBlockDevices(_))
        .WillRepeatedly(Return(std::vector<std::string>{}));
    action->SetAdapter(std::move(mockAdapter));

    auto chain = UpdateChainBuilder()
        .Add(std::move(action))
        .Build();
    ASSERT_NE(chain, nullptr);
    UpdaterParams upParams {};
    upParams.updatePackage.push_back("/sdcard/updater/updater.zip");
    UpdaterStatus status = chain->Handle(upParams);
    EXPECT_EQ(status, UPDATE_SUCCESS);
}

HWTEST_F(SdcardUpdateUnittest, UpdateGroup_MultiGroupChain, TestSize.Level1)
{
    auto group1 = std::make_shared<UpdateGroup>();
    auto action1 = std::make_unique<FindSdPkgAction>();
    group1->AddStep(std::make_shared<UpdateStep>(std::move(action1)));

    auto group2 = std::make_shared<UpdateGroup>();
    auto action2 = std::make_unique<FindSdPkgAction>();
    group2->AddStep(std::make_shared<UpdateStep>(std::move(action2)));

    group1->SetNextGroup(group2);

    UpdaterParams upParams {};
    upParams.updatePackage.push_back("/sdcard/updater/updater.zip");
    UpdaterStatus status = group1->Handle(upParams);
    EXPECT_EQ(status, UPDATE_SUCCESS);
}

HWTEST_F(SdcardUpdateUnittest, FindSdPkgAction_GetSdcardPkgsPath_PackageAlreadyExists, TestSize.Level1)
{
    FindSdPkgAction action;
    UpdaterParams upParams {};
    upParams.updatePackage.push_back("/data/updater/updater.zip");
    UpdaterStatus status = action.GetSdcardPkgsPath(upParams);
    EXPECT_EQ(status, UPDATE_SUCCESS);
    EXPECT_EQ(upParams.updatePackage.size(), 1);
    EXPECT_EQ(upParams.updatePackage[0], "/data/updater/updater.zip");
}
} // namespace
