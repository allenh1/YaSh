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

/* Command Data Structure */
struct SimpleCommand {
	SimpleCommand();
	~SimpleCommand() { release(); }
	std::vector<char* > arguments;
	void insertArgument(char * argument);
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
	bool handle_builtins(const int &, const int &, const int&);
	void handle_modified_commands();

	bool handle_cd(const int &,const int &,const int &);
	bool handle_cl(const int &, const int &, const int &);
	bool handle_setenv(const int &, const int &, const int &);
	bool handle_unsetenv(const int &, const int &, const int &);
	bool handle_history();
	bool completed = false;
	bool stopped = false;
	int status = -1;
	void handle_ls();
	void handle_grep();
	void handle_printenv();

	static std::vector<std::string> * history;
	pid_t pid;
};
#endif
