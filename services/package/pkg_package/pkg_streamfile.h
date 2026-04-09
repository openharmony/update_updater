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
#ifndef STREAM_PKG_FILE_H
#define STREAM_PKG_FILE_H

#include "pkg_upgradefile.h"

namespace Hpackage {
class StreamPkgFile : public UpgradePkgFile {
public:
    StreamPkgFile(PkgManager::PkgManagerPtr manager, PkgStreamPtr headStream, PkgStreamPtr entryStream,
        PkgInfoPtr header) : UpgradePkgFile(manager, headStream, header), entryStream_(entryStream)
    {}
    int32_t SetUpradeEntryStream(UpgradeFileEntry *entry) override;
    PkgType GetPkgType() const override;
    PkgStreamPtr GetPkgEntryStream() const override;
private:
    PkgStreamPtr entryStream_ {nullptr};
};
} // namespace Hpackage
#endif