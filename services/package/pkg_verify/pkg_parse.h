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

#ifndef PKG_PARSE_H
#define PKG_PARSE_H

#include "pkg_stream.h"

namespace Hpackage {
class PkgParse {
public:
    virtual ~PkgParse() = default;
    virtual int32_t ParsePkg(Hpackage::PkgStreamPtr pkgStream, PkgSignComment &pkgSignComment) const = 0;
    virtual uint32_t GetFixedPartLen(void) const = 0;
    virtual bool IsSupportOldSig(void) const = 0;
};
} // namespace Hpackage
#endif