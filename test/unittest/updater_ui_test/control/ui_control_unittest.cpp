/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "gtest/gtest.h"
#include "event_listener.h"
#include "event_manager.h"
#include "callback_manager.h"
#include <linux/input.h>
#include "dock/input_device.h"

using namespace testing::ext;
using namespace Updater;

namespace {
class UpdaterUiControlUnittest : public testing::Test {
public:
    KeyListener *keyListener;
    OHOS::UIView *view;

    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() override
    {
        keyListener = new KeyListener();
        view = new OHOS::UIView();
    }
    void TearDown() override
    {
        delete keyListener;
        keyListener = nullptr;
        delete view;
        view = nullptr;
    }
};

HWTEST_F(UpdaterUiControlUnittest, OnKeyAct01, TestSize.Level0)
{
    OHOS::KeyEvent *event = new OHOS::KeyEvent(KEY_POWER, OHOS::InputDevice::STATE_PRESS);
    bool ret = keyListener->OnKeyAct(*view, *event);
    EXPECT_EQ(ret, true);
    if (event != nullptr) {
        delete event;
        event = nullptr;
    }
}

HWTEST_F(UpdaterUiControlUnittest, OnKeyAct02, TestSize.Level0)
{
    OHOS::KeyEvent *event = new OHOS::KeyEvent(KEY_VOLUMEUP, OHOS::InputDevice::STATE_PRESS);
    bool ret = keyListener->OnKeyAct(*view, *event);
    EXPECT_EQ(ret, true);
    if (event != nullptr) {
        delete event;
        event = nullptr;
    }
}

HWTEST_F(UpdaterUiControlUnittest, OnKeyAct03, TestSize.Level0)
{
    OHOS::KeyEvent *event = new OHOS::KeyEvent(KEY_VOLUMEDOWN, OHOS::InputDevice::STATE_PRESS);
    bool ret = keyListener->OnKeyAct(*view, *event);
    EXPECT_EQ(ret, true);
    if (event != nullptr) {
        delete event;
        event = nullptr;
    }
}

HWTEST_F(UpdaterUiControlUnittest, OnKeyAct04, TestSize.Level0)
{
    OHOS::KeyEvent *event = new OHOS::KeyEvent(100, OHOS::InputDevice::STATE_PRESS);
    bool ret = keyListener->OnKeyAct(*view, *event);
    EXPECT_EQ(ret, false);
    if (event != nullptr) {
        delete event;
        event = nullptr;
    }
}

HWTEST_F(UpdaterUiControlUnittest, OnKeyAct05, TestSize.Level0)
{
    OHOS::KeyEvent *event = new OHOS::KeyEvent(KEY_VOLUMEUP, OHOS::InputDevice::STATE_RELEASE);
    bool ret = keyListener->OnKeyAct(*view, *event);
    EXPECT_EQ(ret, true);
    if (event != nullptr) {
        delete event;
        event = nullptr;
    }
}

HWTEST_F(UpdaterUiControlUnittest, OnKeyAct06, TestSize.Level0)
{
    OHOS::KeyEvent *event = new OHOS::KeyEvent(KEY_VOLUMEDOWN, OHOS::InputDevice::STATE_RELEASE);
    bool ret = keyListener->OnKeyAct(*view, *event);
    EXPECT_EQ(ret, true);
    if (event != nullptr) {
        delete event;
        event = nullptr;
    }
}

HWTEST_F(UpdaterUiControlUnittest, OnKeyAct07, TestSize.Level0)
{
    OHOS::KeyEvent *event = new OHOS::KeyEvent(KEY_POWER, OHOS::InputDevice::STATE_RELEASE);
    bool ret = keyListener->OnKeyAct(*view, *event);
    EXPECT_EQ(ret, true);
    if (event != nullptr) {
        delete event;
        event = nullptr;
    }
}

HWTEST_F(UpdaterUiControlUnittest, ProcessVolumeKeyButtonPressedTest, TestSize.Level0)
{
    KeyListener::SetButtonPressed(true);
    OHOS::UIView *testView = new OHOS::UIView();
    OHOS::KeyEvent *event = new OHOS::KeyEvent(KEY_VOLUMEUP, OHOS::InputDevice::STATE_RELEASE);
    bool ret = keyListener->ProcessVolumeKey(*testView, *event);
    EXPECT_EQ(ret, true);
    KeyListener::SetButtonPressed(false);
    delete testView;
    delete event;
}

HWTEST_F(UpdaterUiControlUnittest, ProcessVolumeKeyNotPressedTest, TestSize.Level0)
{
    KeyListener::SetButtonPressed(false);
    OHOS::UIView *testView = new OHOS::UIView();
    OHOS::KeyEvent *event = new OHOS::KeyEvent(KEY_VOLUMEUP, OHOS::InputDevice::STATE_RELEASE);
    bool ret = keyListener->ProcessVolumeKey(*testView, *event);
    EXPECT_EQ(ret, true);
    delete testView;
    delete event;
}

HWTEST_F(UpdaterUiControlUnittest, ProcessVolumeKeyPressStateTest, TestSize.Level0)
{
    KeyListener::SetButtonPressed(false);
    OHOS::UIView *testView = new OHOS::UIView();
    OHOS::KeyEvent *event = new OHOS::KeyEvent(KEY_VOLUMEUP, OHOS::InputDevice::STATE_PRESS);
    bool ret = keyListener->ProcessVolumeKey(*testView, *event);
    EXPECT_EQ(ret, true);
    delete testView;
    delete event;
}

HWTEST_F(UpdaterUiControlUnittest, ProcessVolumeKeyVolumeDownTest, TestSize.Level0)
{
    KeyListener::SetButtonPressed(false);
    OHOS::UIView *testView = new OHOS::UIView();
    OHOS::KeyEvent *event = new OHOS::KeyEvent(KEY_VOLUMEDOWN, OHOS::InputDevice::STATE_RELEASE);
    bool ret = keyListener->ProcessVolumeKey(*testView, *event);
    EXPECT_EQ(ret, true);
    delete testView;
    delete event;
}

HWTEST_F(UpdaterUiControlUnittest, ProcessPowerKeyTest, TestSize.Level0)
{
    OHOS::UIView *testView = new OHOS::UIView();
    OHOS::KeyEvent *event = new OHOS::KeyEvent(KEY_POWER, OHOS::InputDevice::STATE_PRESS);
    bool ret = keyListener->ProcessPowerKey(*testView, *event);
    EXPECT_EQ(ret, true);
    delete testView;
    delete event;
}

HWTEST_F(UpdaterUiControlUnittest, ProcessPowerKeyReleaseTest, TestSize.Level0)
{
    OHOS::UIView *testView = new OHOS::UIView();
    OHOS::KeyEvent *event = new OHOS::KeyEvent(KEY_POWER, OHOS::InputDevice::STATE_RELEASE);
    bool ret = keyListener->ProcessPowerKey(*testView, *event);
    EXPECT_EQ(ret, true);
    delete testView;
    delete event;
}

HWTEST_F(UpdaterUiControlUnittest, CallbackManagerRegisterFuncTest, TestSize.Level0)
{
    auto func = [](OHOS::UIView &view) {};
    bool ret = CallbackManager::RegisterFunc("testCallback", Callback{func, false});
    EXPECT_EQ(ret, true);
    ret = CallbackManager::RegisterFunc("testCallback", Callback{func, false});
    EXPECT_EQ(ret, false);
}

HWTEST_F(UpdaterUiControlUnittest, CallbackManagerInitTest, TestSize.Level0)
{
    EXPECT_NO_FATAL_FAILURE(CallbackManager::Init(false));
    EXPECT_NO_FATAL_FAILURE(CallbackManager::Init(true));
}

HWTEST_F(UpdaterUiControlUnittest, CallbackManagerRegisterTest, TestSize.Level0)
{
    auto func = [](OHOS::UIView &view) {};
    CallbackManager::RegisterFunc("testRegCallback", Callback{func, false});
    CallbackCfg cfg;
    cfg.pageId = "page1";
    cfg.comId = "com1";
    cfg.type = "CLICKEVENT";
    cfg.func = "testRegCallback";
    EXPECT_NO_FATAL_FAILURE(CallbackManager::Register(cfg));
}

HWTEST_F(UpdaterUiControlUnittest, CallbackManagerRegisterInvalidTypeTest, TestSize.Level0)
{
    auto func = [](OHOS::UIView &view) {};
    CallbackManager::RegisterFunc("testRegCallback2", Callback{func, false});
    CallbackCfg cfg;
    cfg.pageId = "page1";
    cfg.comId = "com1";
    cfg.type = "INVALID_EVENT";
    cfg.func = "testRegCallback2";
    EXPECT_NO_FATAL_FAILURE(CallbackManager::Register(cfg));
}

HWTEST_F(UpdaterUiControlUnittest, CallbackManagerRegisterInvalidFuncTest, TestSize.Level0)
{
    CallbackCfg cfg;
    cfg.pageId = "page1";
    cfg.comId = "com1";
    cfg.type = "CLICKEVENT";
    cfg.func = "nonExistentFunc";
    EXPECT_NO_FATAL_FAILURE(CallbackManager::Register(cfg));
}

HWTEST_F(UpdaterUiControlUnittest, EventManagerConstructorTest, TestSize.Level0)
{
    EventManager &mgr = EventManager::GetInstance();
    EXPECT_NE(&mgr, nullptr);
}
}