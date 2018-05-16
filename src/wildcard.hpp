// Copyright 2016 Hunter L. Allen
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

#ifndef WILDCARD_HPP_
#define WILDCARD_HPP_
/* C includes */
#include <glob.h>

/*
 * Note(allenh1): This is needed, since GLOB_PERIOD is not on OS X.
 * This macro comes from the bug report on another project.
 *   https://sft.its.cern.ch/jira/si/jira.issueviews:issue-html/CVM-927/CVM-927.html
 * This can potentially be removed in the future.
 */
#ifdef __APPLE__
#define GLOB_PERIOD (1 << 7)
#endif

/* STL (C++) includes */
#include <algorithm>
#include <iostream>
#include <memory>

#include "command.hpp"

#define MAXLEN 1024

void wildcard_expand(std::shared_ptr<char> arg);
#endif  // WILDCARD_HPP_
