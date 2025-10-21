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
#include "log_unittest.h"
#include <fstream>
#include <iostream>
#include <string>
#include "log/log.h"

using namespace testing::ext;
using namespace UpdaterUt;
using namespace Updater;
using namespace std;

namespace UpdaterUt {
#define CSTYLE_LOGI(format, ...) Logger(Updater::INFO, (UPDATER_LOG_FILE_NAME), (__LINE__), format, ##__VA_ARGS__)
#define CSTYLE_LOGI_LITE(format, ...) \
    LoggerLite(Updater::INFO, (UPDATER_LOG_FILE_NAME), (__LINE__), format, ##__VA_ARGS__)

void LogUnitTest::SetUpTestCase(void)
{
    cout << "SetUpTestCase" << endl;
}

void LogUnitTest::TearDownTestCase(void)
{
    cout << "TearDownTestCase" << endl;
}

void LogUnitTest::SetUp()
{
    SetLogLevel(INFO);
}

void LogUnitTest::TearDown()
{
}

HWTEST_F(LogUnitTest, log_test_001, TestSize.Level1)
{
    InitUpdaterLogger("UPDATER_UT", "", "", "");
    SetLogLevel(ERROR);
    LOG(ERROR) << "this is ut";
    STAGE(UPDATE_STAGE_BEGIN) << "this is ut";
    ERROR_CODE(CODE_VERIFY_FAIL);
    SUCCEED();

    SetLogLevel(INFO);
    LOG(ERROR) << "this is ut";
    STAGE(UPDATE_STAGE_BEGIN) << "this is ut";
    SUCCEED();

    SetLogLevel(ERROR);
    LOG(ERROR) << "this is ut";
    STAGE(UPDATE_STAGE_BEGIN) << "this is ut";
    InitUpdaterLogger("UPDATER_UT", "/data/updater/m_log.txt", "/data/updater/m_stage.txt", "/data/updater/m_code.txt");

    LOG(ERROR) << "this is ut";
    STAGE(UPDATE_STAGE_BEGIN) << "this is ut";
    ERROR_CODE(CODE_VERIFY_FAIL);
    fstream f;
    f.open("/data/updater/m_log.txt", ios::in);
    if (!f) {
        SUCCEED();
    };
    char ch[100];
    f.getline(ch, 100);
    string result = ch;
    auto ret = result.find("this is ut");
    if (ret != string::npos) {
        f.close();
        unlink("/data/updater/m_log.txt");
        unlink("/data/updater/m_stage.txt");
        EXPECT_NE(ret, string::npos);
        SUCCEED();
    } else {
        f.close();
        unlink("/data/updater/m_log.txt");
        unlink("/data/updater/m_stage.txt");
        FAIL();
    }
}

class UpdaterLoggerTest : public UpdaterLogger {
public:
    explicit UpdaterLoggerTest(int level) : UpdaterLogger(level) {}

    std::string GetLogText()
    {
        return oss_.str();
    }
};

HWTEST_F(LogUnitTest, log_test_with_source_file_info, TestSize.Level1)
{
    std::string src = (strrchr((__FILE_NAME__), '/') != nullptr) ? strrchr((__FILE_NAME__), '/') + 1 : (__FILE_NAME__);
    UpdaterLoggerTest loggerTest(Updater::WARNING);
    UpdaterLogger &logger = loggerTest;
    logger.OutputUpdaterLog((UPDATER_LOG_FILE_NAME), (__LINE__)) << "log with source file info";
    std::string logText = loggerTest.GetLogText();
    GTEST_LOG_(INFO) << logText;
    CSTYLE_LOGI("logSize: %zu, logText: %s", logText.size(), logText.c_str());
    ASSERT_NE(logText.find(src), std::string::npos);
}

class UpdaterLoggerLiteTest : public UpdaterLoggerLite {
public:
    explicit UpdaterLoggerLiteTest(int level) : UpdaterLoggerLite(level) {}

    std::string GetLogText()
    {
        return oss_.str();
    }
};

HWTEST_F(LogUnitTest, log_test_lite, TestSize.Level1)
{
    std::string src = (strrchr((__FILE_NAME__), '/') != nullptr) ? strrchr((__FILE_NAME__), '/') + 1 : (__FILE_NAME__);
    UpdaterLoggerLiteTest loggerTest(Updater::WARNING);
    UpdaterLoggerLite &logger = loggerTest;
    logger.OutputUpdaterLog() << "no source file info";
    std::string logText = loggerTest.GetLogText();
    GTEST_LOG_(INFO) << logText;
    CSTYLE_LOGI_LITE("logSize: %zu, logText: %s", logText.size(), logText.c_str());
    ASSERT_EQ(logText.find(src), std::string::npos);
}
} // namespace updater_ut
