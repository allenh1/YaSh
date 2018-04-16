// Copyright 2016 YaSh Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __WILDCARD_HPP__
#define __WILDCARD_HPP__
/* STL (C++) includes */
#include <algorithm>
#include <iostream>
#include <glob.h>

#include "command.hpp"

#define MAXLEN 1024

void wildcard_expand(std::shared_ptr<char> arg);
#endif
