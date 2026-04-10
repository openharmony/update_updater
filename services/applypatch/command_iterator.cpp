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

#include "applypatch/command_iterator.h"
#include "log/log.h"

namespace Updater {
CommandIterator::CommandIterator(const std::vector<std::string>::const_iterator &ct,
    const std::vector<std::string>::const_iterator &ctEnd) :
    ct_(ct), ctEnd_(ctEnd) {}

CommandIterator *CommandIterator::operator->()
{
    return this;
}

CommandIterator &CommandIterator::operator++()
{
    Next();
    return *this;
}

CommandIterator CommandIterator::operator++(int)
{
    CommandIterator tmp(*this);
    Next();
    return tmp;
}

std::string CommandIterator::operator*()
{
    return Current();
}

bool CommandIterator::Check()
{
    if (ctEnd_ - ct_ < 1) {
        LOG(ERROR) << "context too small";
        return false;
    }
    return true;
}

void CommandIterator::Start()
{
    return;
}

bool CommandIterator::Done()
{
    return ct_ == ctEnd_;
}

std::string CommandIterator::Current()
{
    if (ct_ < ctEnd_) {
        return *ct_;
    }
    LOG(ERROR) << "context Out-of-bounds Access";
    return "";
}

void CommandIterator::Next()
{
    if (ct_ >= ctEnd_) {
        LOG(ERROR) << "context Out-of-bounds Access";
        return;
    }
    ++ct_;
}
} // namespace Updater
