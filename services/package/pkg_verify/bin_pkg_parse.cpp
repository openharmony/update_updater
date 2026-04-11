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

#include "bin_pkg_parse.h"
#include <vector>
#include "dump.h"
#include "pkg_utils.h"

namespace Hpackage {
int32_t BinPkgParse::ParsePkg(Hpackage::PkgStreamPtr pkgStream, PkgSignComment &pkgSignComment) const
{
    Updater::UPDATER_INIT_RECORD;
    if (pkgStream == nullptr) {
        UPDATER_LAST_WORD(PKG_INVALID_PARAM, "pkgStream is null");
        return PKG_INVALID_PARAM;
    }
    uint16_t signCommentAppendLen = 0;
    uint16_t signCommentTotalLen = 0;
    size_t readLen = 0;
    if (auto ret = ReadFooterFromStream(pkgStream, readLen, signCommentAppendLen, signCommentTotalLen);
        ret != PKG_SUCCESS) {
        PKG_LOGE("ParsePkg failed for bin pkg %d", ret);
        return ret;
    }
    size_t fileLen = pkgStream->GetFileLength();
    if (fileLen <= signCommentTotalLen) {
        PKG_LOGE("file len[%zu] < signCommentTotalLen[%zu]", fileLen, signCommentTotalLen);
        UPDATER_LAST_WORD(PKG_INVALID_PKG_FORMAT, fileLen, signCommentTotalLen);
        return PKG_INVALID_FILE;
    }
    pkgSignComment.signCommentTotalLen = signCommentTotalLen;
    pkgSignComment.signCommentAppendLen = signCommentAppendLen;
    return PKG_SUCCESS;
}

uint32_t BinPkgParse::GetFixedPartLen(void) const
{
    return 0;
}

bool BinPkgParse::IsSupportOldSig(void) const
{
    return false;
}
}  // namespace Hpackage