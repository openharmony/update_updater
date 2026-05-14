/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include <utility>
#include "gtest/gtest.h"
#include "ui_test_graphic_engine.h"
#include "strategy/background_strategy.h"
#include "view/component/component_register.h"
#include "view/component/img_view_adapter.h"
#include "view/component/text_label_adapter.h"
#include "view/page/page_manager.h"

using namespace Updater;
using namespace std;
using namespace testing::ext;

namespace {
constexpr const char *MAIN_PAGE_ID = "page1";

inline PageManager &GetInstance()
{
    return PageManager::GetInstance();
}

class BackgroundStrategyUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
        TestGraphicEngine::GetInstance();
    }
    static void TearDownTestCase(void) {}
    void SetUp() override
    {
        RegisterComponents();
        pageInfos_ = MakeUxPages();
        ASSERT_TRUE(GetInstance().Init(pageInfos_, MAIN_PAGE_ID));
    }
    void TearDown() override
    {
        GetInstance().Reset();
        pageInfos_.clear();
    }

protected:
    static std::vector<UxPageInfo> MakeUxPages();

private:
    static UxPageInfo MakeTestPage();
    inline static std::vector<Updater::UxPageInfo> pageInfos_;
};

template <typename T>
UxViewInfo CreateViewInfo(UxViewCommonInfo commonInfo, typename T::SpecificInfoType specInfo)
{
    UxViewInfo info {commonInfo, ComponentFactory::CreateSpecificInfo(T::COMPONENT_TYPE)};
    EXPECT_NE(info.specificInfo.get(), nullptr);
    static_cast<SpecificInfoWrapper<T> *>(info.specificInfo.get())->data = specInfo;
    return info;
}

UxPageInfo BackgroundStrategyUnitTest::MakeTestPage()
{
    UxPageInfo page;
    page.id = "page1";
    page.bgColor = "#000000ff";
    page.viewInfos.emplace_back(CreateViewInfo<TextLabelAdapter>(UxViewCommonInfo {300, 400, 600, 200,
        "foreground_label", "UILabel", true}, UxLabelInfo {50, "foreground label", "center", "#ff0000ff",
        "#000000ff", "normal", {"#ff0000ff", "#000000ff", false}, false, "ellipsis"}));
    page.viewInfos.emplace_back(CreateViewInfo<ImgViewAdapter>(UxViewCommonInfo {300, 700, 400, 400,
        "background_img", "UIImageView", false}, UxImageInfo {"/resources/img1", "empty", 100, 0}));
    page.viewInfos.emplace_back(CreateViewInfo<ImgViewAdapter>(UxViewCommonInfo {100, 100, 200, 200,
        "anim_bg", "UIImageView", false}, UxImageInfo {"/resources/anim", "frame", 10, 100}));
    return page;
}

std::vector<UxPageInfo> BackgroundStrategyUnitTest::MakeUxPages()
{
    std::vector<UxPageInfo> pages;
    auto page = MakeTestPage();
    pages.emplace_back(std::move(page));
    return pages;
}

HWTEST_F(BackgroundStrategyUnitTest, test_factory_create_anim_background, TestSize.Level1)
{
    ComInfo bgId {"page1", "anim_bg"};
    std::vector<ForegroundComId> foregroundIds {"foreground_label", 10};
    auto bgStrategy = BackgroundStrategy::Factory("anim", bgId, foregroundIds);
    ASSERT_NE(bgStrategy, nullptr);
    bgStrategy->Show();
    auto *imgAdapter = GetInstance()[bgId].As<ImgViewAdapter>();
    EXPECT_NE(imgAdapter, nullptr);
    GetInstance()[bgId].As<ImgViewAdapter>()->Stop();
}

HWTEST_F(BackgroundStrategyUnitTest, test_factory_create_static_background, TestSize.Level1)
{
    ComInfo bgId {"page1", "background_img"};
    std::vector<ForegroundComId> foregroundIds {"foreground_label", 10};
    auto bgStrategy = BackgroundStrategy::Factory("img", bgId, foregroundIds);
    ASSERT_NE(bgStrategy, nullptr);
    EXPECT_NO_FATAL_FAILURE(bgStrategy->Show());
    EXPECT_NO_FATAL_FAILURE(bgStrategy->Hide());

    auto bgStrategyInvalid = BackgroundStrategy::Factory("invalid_type", bgId, foregroundIds);
    ASSERT_NE(bgStrategyInvalid, nullptr);
    EXPECT_NO_FATAL_FAILURE(bgStrategyInvalid->Show());
    EXPECT_NO_FATAL_FAILURE(bgStrategyInvalid->Hide());
}

HWTEST_F(BackgroundStrategyUnitTest, test_animator_background_hide, TestSize.Level1)
{
    ComInfo bgId {"page1", "anim_bg"};
    std::vector<ForegroundComId> foregroundIds {"foreground_label", 10};
    auto bgStrategy = BackgroundStrategy::Factory("anim", bgId, foregroundIds);
    ASSERT_NE(bgStrategy, nullptr);
    bgStrategy->Show();
    bgStrategy->Hide();
}

HWTEST_F(BackgroundStrategyUnitTest, test_animator_background_with_empty_foreground_ids, TestSize.Level1)
{
    ComInfo bgId {"page1", "anim_bg"};
    std::vector<ForegroundComId> emptyForegroundIds {};
    auto bgStrategy = BackgroundStrategy::Factory("anim", bgId, emptyForegroundIds);
    ASSERT_NE(bgStrategy, nullptr);
    EXPECT_NO_FATAL_FAILURE(bgStrategy->Show());
    EXPECT_NO_FATAL_FAILURE(bgStrategy->Hide());
}

HWTEST_F(BackgroundStrategyUnitTest, test_multiple_foreground_components_zindex, TestSize.Level1)
{
    ComInfo pageId {"page1", ""};
    GetInstance().Reset();
    std::vector<UxPageInfo> pages;
    UxPageInfo page;
    page.id = "page1";
    page.bgColor = "#000000ff";
    page.viewInfos.emplace_back(CreateViewInfo<TextLabelAdapter>(UxViewCommonInfo {300, 400, 600, 200,
        "fg_label1", "UILabel", true}, UxLabelInfo {50, "label1", "center", "#ff0000ff",
        "#000000ff", "normal", {"#ff0000ff", "#000000ff", false}, false, "ellipsis"}));
    page.viewInfos.emplace_back(CreateViewInfo<TextLabelAdapter>(UxViewCommonInfo {300, 500, 600, 200,
        "fg_label2", "UILabel", true}, UxLabelInfo {50, "label2", "center", "#00ff00ff",
        "#000000ff", "normal", {"#00ff00ff", "#000000ff", false}, false, "ellipsis"}));
    page.viewInfos.emplace_back(CreateViewInfo<ImgViewAdapter>(UxViewCommonInfo {300, 700, 400, 400,
        "bg_img", "UIImageView", false}, UxImageInfo {"/resources/img1", "empty", 100, 0}));

    pages.emplace_back(std::move(page));
    ASSERT_TRUE(GetInstance().Init(pages, "page1"));

    ComInfo bgId {"page1", "bg_img"};
    std::vector<ForegroundComId> foregroundIds {{"fg_label1", 10}, {"fg_label2", 20}};
    auto bgStrategy = BackgroundStrategy::Factory("img", bgId, foregroundIds);
    ASSERT_NE(bgStrategy, nullptr);
    EXPECT_NO_FATAL_FAILURE(bgStrategy->Show());
}
} // namespace