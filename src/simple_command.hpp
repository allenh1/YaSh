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

#ifndef __SIMPLE_COMMAND_HPP__
#define __SIMPLE_COMMAND_HPP__
#pragma once
/* UNIX Includes */
#include <sys/resource.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/stat.h>

/* Linux Includes */
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <fcntl.h>
#include <pwd.h>

/* C includes */
#include <stdio.h>

/* STL (C++) includes */
#include <functional>
#include <algorithm>
#include <typeinfo>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstring>
#include <sstream>
#include <thread>
#include <vector>
#include <memory>
#include <stack>
#include <map>

/* File Includes */
#include "shell-utils.hpp"
#include "wildcard.hpp"
#include "job.hpp"

/* Command Data Structure */
struct SimpleCommand {
    SimpleCommand();
    ~SimpleCommand() { release(); }
    std::vector<char* > arguments;
    void insertArgument(const std::shared_ptr<char> argument);
    ssize_t numOfArguments = 0;
    void release() {
        for (size_t x = 0; x < arguments.size(); ++x) delete[] arguments[x];
        arguments.clear();
        arguments.shrink_to_fit();
        numOfArguments = 0;
    };

    void launch(const int & fdin, const int & fdout, const int & fderr,
                const int & pgid, const bool & background, const bool & interactive);
    void setup_process_io(const int & fdin, const int & fdout, const int & fderr);
    void save_io(const int & fdin, const int & fdout, const int & fderr,
                 int & saved_fdin, int & saved_fdout, int & saved_fderr);
    void resume_io(const int & fdin, const int & fdout, const int & fderr);
    bool handle_builtins(const int &, const int &, const int&);
    void handle_modified_commands();

    bool handle_cd(const int &,const int &,const int &);
    bool handle_cl(const int &, const int &, const int &);
    bool handle_setenv(const int &, const int &, const int &);
    bool handle_unsetenv(const int &, const int &, const int &);

    void handle_ls();
    void handle_grep();
    void handle_jobs();
    void handle_history();
    void handle_printenv();

    bool completed = false;
    bool stopped = false;
    int status = -1;

    static std::shared_ptr<std::vector<std::string>> history;
    static std::shared_ptr<std::vector<job>> p_jobs;

    pid_t pid;
};
#endif
