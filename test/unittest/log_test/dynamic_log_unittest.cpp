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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <thread>
#include "log/dynamic_log.h"

using namespace testing::ext;
using namespace Updater;

namespace UpdaterUt {
class DynamicLogUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
        InitUpdaterLogger("DYNAMIC_LOG_UT", "updater_log.log", "updater_status.log", "error_code.log");
    }

    static void TearDownTestCase(void)
    {
    }

    void SetUp()
    {
        SetDynamicLogLevel(INFO);
    }

    void TearDown()
    {
        SetDynamicLogLevel(INFO);
    }
};

HWTEST_F(DynamicLogUnitTest, LOGDYN_Test, TestSize.Level1)
{
    SetDynamicLogLevel(INFO);
    LOGDYN("simple log message");
    LOGDYN("formatted anwser %s %d", "message", 42);
    LOGDYN("multiple args: %d %f %s %c %02hx", 123, 3.14, "test", 'X', 253);
    std::string longString(500, 'A');
    LOGDYN("long string: %s", longString.c_str());
    {
        DynamicLoggerGuard guard(ERROR);
        LOGDYN("inside guard");
        EXPECT_EQ(GetDynamicLogLevel(), ERROR);
    }
    EXPECT_EQ(GetDynamicLogLevel(), INFO);
    SUCCEED();
}

HWTEST_F(DynamicLogUnitTest, LOGDYN_SEN_Test, TestSize.Level1)
{
    SetDynamicLogLevel(INFO);
    LOGDYN_SEN("simple log message");
    LOGDYN_SEN("formatted anwser %s %d", "message", 42);
    LOGDYN_SEN("multiple args: %d %f %s %c %02hx", 123, 3.14, "test", 'X', 253);
    std::string longString(500, 'A');
    LOGDYN_SEN("long string: %s", longString.c_str());
    {
        DynamicLoggerGuard guard(DEBUG);
        LOGDYN_SEN("inside guard");
        EXPECT_EQ(GetDynamicLogLevel(), DEBUG);
    }
    EXPECT_EQ(GetDynamicLogLevel(), INFO);
    SUCCEED();
}

HWTEST_F(DynamicLogUnitTest, LOGDYN_SEN_TooLargeText, TestSize.Level1)
{
    SetDynamicLogLevel(ERROR);
    std::string largeText(2048, 'E');
    LOGDYN_SEN("too large text: %s", largeText.c_str());
    EXPECT_EQ(GetDynamicLogLevel(), ERROR);
    SUCCEED();
}

HWTEST_F(DynamicLogUnitTest, SetDynamicLogLevel_ValidLevels, TestSize.Level1)
{
    SetDynamicLogLevel(DEBUG);
    EXPECT_EQ(GetDynamicLogLevel(), DEBUG);

    SetDynamicLogLevel(INFO);
    EXPECT_EQ(GetDynamicLogLevel(), INFO);

    SetDynamicLogLevel(WARNING);
    EXPECT_EQ(GetDynamicLogLevel(), WARNING);

    SetDynamicLogLevel(ERROR);
    EXPECT_EQ(GetDynamicLogLevel(), ERROR);

    SetDynamicLogLevel(FATAL);
    EXPECT_EQ(GetDynamicLogLevel(), FATAL);
}

HWTEST_F(DynamicLogUnitTest, SetDynamicLogLevel_InvalidLevel, TestSize.Level0)
{
    int originalLevel = GetDynamicLogLevel();
    SetDynamicLogLevel(DEBUG - 1);
    EXPECT_EQ(GetDynamicLogLevel(), originalLevel);

    originalLevel = GetDynamicLogLevel();
    SetDynamicLogLevel(FATAL + 1);
    EXPECT_EQ(GetDynamicLogLevel(), originalLevel);

    originalLevel = GetDynamicLogLevel();
    SetDynamicLogLevel(-100);
    EXPECT_EQ(GetDynamicLogLevel(), originalLevel);

    originalLevel = GetDynamicLogLevel();
    SetDynamicLogLevel(1000);
    EXPECT_EQ(GetDynamicLogLevel(), originalLevel);
}

HWTEST_F(DynamicLogUnitTest, GetDynamicLogLevel_DefaultValue, TestSize.Level0)
{
    std::thread task([]() {
        ASSERT_EQ(GetDynamicLogLevel(), INFO);
    });
    if (task.joinable()) {
        task.join();
    }
}

HWTEST_F(DynamicLogUnitTest, DynamicLoggerGuard_ConstructorWithValidLevel, TestSize.Level1)
{
    SetDynamicLogLevel(DEBUG);
    {
        DynamicLoggerGuard guard(WARNING);
        EXPECT_EQ(GetDynamicLogLevel(), WARNING);
    }
    EXPECT_EQ(GetDynamicLogLevel(), DEBUG);
}

HWTEST_F(DynamicLogUnitTest, DynamicLoggerGuard_ConstructorWithInvalidLowLevel, TestSize.Level0)
{
    SetDynamicLogLevel(DEBUG);
    {
        DynamicLoggerGuard guard(DEBUG - 1);
        EXPECT_EQ(GetDynamicLogLevel(), INFO);
    }
    EXPECT_EQ(GetDynamicLogLevel(), DEBUG);
}

HWTEST_F(DynamicLogUnitTest, DynamicLoggerGuard_ConstructorWithInvalidHighLevel, TestSize.Level0)
{
    SetDynamicLogLevel(DEBUG);
    {
        DynamicLoggerGuard guard(FATAL + 1);
        EXPECT_EQ(GetDynamicLogLevel(), INFO);
    }
    EXPECT_EQ(GetDynamicLogLevel(), DEBUG);
}

HWTEST_F(DynamicLogUnitTest, DynamicLoggerGuard_NestedGuards, TestSize.Level1)
{
    SetDynamicLogLevel(DEBUG);
    {
        DynamicLoggerGuard guard1(WARNING);
        EXPECT_EQ(GetDynamicLogLevel(), WARNING);
        {
            DynamicLoggerGuard guard2(ERROR);
            EXPECT_EQ(GetDynamicLogLevel(), ERROR);
        }
        EXPECT_EQ(GetDynamicLogLevel(), WARNING);
    }
    EXPECT_EQ(GetDynamicLogLevel(), DEBUG);
}

HWTEST_F(DynamicLogUnitTest, DynamicLoggerGuard_NestedSilentGuards, TestSize.Level1)
{
    SetDynamicLogLevel(DEBUG);
    {
        DynamicLoggerGuard guard1(WARNING, true);
        EXPECT_EQ(GetDynamicLogLevel(), WARNING);
        {
            DynamicLoggerGuard guard2(ERROR, true);
            EXPECT_EQ(GetDynamicLogLevel(), ERROR);
        }
        EXPECT_EQ(GetDynamicLogLevel(), WARNING);
    }
    EXPECT_EQ(GetDynamicLogLevel(), DEBUG);
}
} // namespace UpdaterUt
