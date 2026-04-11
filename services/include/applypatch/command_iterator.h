/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef COMMAND_ITERATOR_H
#define COMMAND_ITERATOR_H

#include <string>
#include <vector>

namespace Updater {
class CommandIterator {
public:
    CommandIterator(const std::vector<std::string>::const_iterator &ct,
        const std::vector<std::string>::const_iterator &ctEnd);
    virtual ~CommandIterator() = default;
    CommandIterator *operator->();
    CommandIterator &operator++();
    CommandIterator operator++(int);
    std::string operator*();
    virtual bool Check();
    virtual void Start();
    virtual bool Done();
    virtual std::string Current();
    virtual void Next();

protected:
    std::vector<std::string>::const_iterator ct_ {};
    std::vector<std::string>::const_iterator ctEnd_ {};
};
} // namespace Updater
#endif // COMMAND_ITERATOR_H
