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

#ifndef ZIP_PKG_PARSE_H
#define ZIP_PKG_PARSE_H

#include "pkg_parse.h"
#include "pkg_stream.h"

namespace Hpackage {
class ZipPkgParse : public PkgParse {
public:
    ZipPkgParse() {};
    ~ZipPkgParse() override {};
    int32_t ParsePkg(Hpackage::PkgStreamPtr pkgStream, PkgSignComment &pkgSignComment) const override;
    uint32_t GetFixedPartLen(void) const override;
    bool IsSupportOldSig(void) const override;
protected:
    int32_t ReadFooterFromStream(PkgStreamPtr pkgStream, size_t &readLen, uint16_t &signCommentAppendLen,
        uint16_t &signCommentTotalLen) const;
private:
    int32_t DoParseZipPkg(PkgStreamPtr pkgStream, PkgSignComment &pkgSignComment,
        size_t &readLen, const uint16_t &signCommentAppendLen, uint16_t &signCommentTotalLen) const;
    int32_t ParsePkgFooter(const uint8_t *footer, size_t length, uint16_t &signCommentAppendLen,
        uint16_t &signCommentTotalLen) const;

    int32_t CheckZipEocd(const uint8_t *eocd, size_t length, uint16_t signCommentTotalLen) const;
};
} // namespace Hpackage
#endif
