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

#include <memory>
#include "wildcard.hpp"

void wildcard_expand(const std::shared_ptr<char> arg)
{
  // return if arg does not contain * or ?
  if ((!strchr(arg.get(), '*') && !strchr(arg.get(), '?')) ||
    !Command::currentCommand.get_expand())
  {
    Command::currentSimpleCommand->insertArgument(arg);
    return;
  }

  bool hidden = false;

  char * a = arg.get();

  for (int i = 0; *(a + 1); a++, i++) {
    if ((*a == '/' && (*(a + 1) == '.' || *(a + 1) == '*')) ||
      (i == 0 && *(a) == '.' && *(a + 1) == '*'))
    {
      hidden = true;
    }
  }

  glob_t results;
  if (hidden) {
    glob(arg.get(), GLOB_PERIOD, nullptr, &results);
  } else {glob(arg.get(), GLOB_ERR, nullptr, &results);}

  Command::currentCommand.wc_collector.clear();
  Command::currentCommand.wc_collector.shrink_to_fit();

  for (size_t i = 0; i < results.gl_pathc; i++) {
    Command::currentCommand.wc_collector.push_back(results.gl_pathv[i]);
  }
  globfree(&results);
}
