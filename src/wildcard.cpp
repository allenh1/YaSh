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

#include "wildcard.hpp"

void wildcard_expand(char * arg) {
    // return if arg does not contain * or ?
    if((!strchr(arg, '*') && !strchr(arg, '?'))
       || !Command::currentCommand.get_expand()) {
        Command::currentSimpleCommand->insertArgument(arg);
        return;
    }

    bool hidden = false;

    char * a = arg;
  
    for (int i=0;*(a+1);a++, i++) {
        if ((*a == '/' && *(a+1) == '.' && *(a+1) == '*')
            || (i==0 && *(a)=='.' && *(a+1)=='*')) {
            hidden = true;
        }
    }
 
    glob_t results;
    if(hidden) {
        glob((const char*)arg, GLOB_PERIOD, (int)NULL, &results);
    } else { glob((const char*)arg, GLOB_ERR,(int)NULL, &results); }

    Command::currentCommand.wc_collector.clear();
    Command::currentCommand.wc_collector.shrink_to_fit();
	
    for(int i=0; i < results.gl_pathc; i++) {
        Command::currentCommand.wc_collector.push_back(results.gl_pathv[i]);
    }  globfree(&results);
}
