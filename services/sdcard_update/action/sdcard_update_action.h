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

#ifndef SDCARD_UPDATE_ACTION_H
#define SDCARD_UPDATE_ACTION_H

#include "sdcard_update.h"
#include "sdcard_update_adapter_interface.h"
#include "sdcard_update_adapter.h"
#include "updater/updater.h"

namespace Updater {

class ISdcardUpdateAction {
public:
    ISdcardUpdateAction() {}
    virtual ~ISdcardUpdateAction() = default;
    void SetAdapter(std::unique_ptr<ISdcardUpdateAdapter> &&adapter);
    virtual UpdaterStatus Execute(UpdaterParams &upParams) = 0;
protected:
    std::unique_ptr<ISdcardUpdateAdapter> adapter_ = std::make_unique<SdcardUpdateAdapter>();
};

class IMountAction : public ISdcardUpdateAction {
public:
    IMountAction() : ISdcardUpdateAction() {}
    ~IMountAction() override {};
    explicit IMountAction(const std::string &path) : mountPath_(path) {}
protected:
    std::string mountPath_;
};

class MountPathAction : public IMountAction {
public:
    MountPathAction() : IMountAction() {}
    explicit MountPathAction(const std::string &path) : IMountAction(path) {}
    ~MountPathAction() override {};
    UpdaterStatus Execute(UpdaterParams &upParams) override;
};

class IFindPkgAction : public ISdcardUpdateAction {
public:
    IFindPkgAction() : ISdcardUpdateAction() {}
    ~IFindPkgAction() override {};
};

class IUmountAction : public ISdcardUpdateAction {
public:
    IUmountAction() : ISdcardUpdateAction() {}
    ~IUmountAction() override {};
    explicit IUmountAction(const std::string &path) : umountPath_(path) {}
protected:
    std::string umountPath_;
};

class UmountPathAction : public IUmountAction {
public:
    UmountPathAction() = default;
    ~UmountPathAction() override {};
    explicit UmountPathAction(const std::string &path) : IUmountAction(path) {}
    UpdaterStatus Execute(UpdaterParams &upParams) override;
};

class MountSdCardAction : public IMountAction {
public:
    MountSdCardAction() : IMountAction() {}
    MountSdCardAction(std::string item, std::string mountPoint)
        : IMountAction(), item_(item), mountPoint_(mountPoint) {}
    ~MountSdCardAction() override {};
    UpdaterStatus Execute(UpdaterParams &upParams) override;
private:
    std::string item_;
    std::string mountPoint_;
};

class FindSdPkgAction : public IFindPkgAction {
public:
    FindSdPkgAction() : IFindPkgAction() {}
    ~FindSdPkgAction() override {};
    UpdaterStatus Execute(UpdaterParams &upParams) override;
#ifndef UPDATER_UT
protected:
#endif
    UpdaterStatus FindAndMountSdcard(UpdaterParams &upParams);
    virtual UpdaterStatus GetSdcardPkgsPath(UpdaterParams &upParams);
private:
    bool MountAndGetPkgs(std::vector<std::string> &sdCardStr,
        const std::string &mountPoint, UpdaterParams &upParams);
};

}
#endif // SDCARD_UPDATE_ACTION_H