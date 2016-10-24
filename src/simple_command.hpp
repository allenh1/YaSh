#ifndef __SIMPLE_COMMAND_HPP__
#define __SIMPLE_COMMAND_HPP__
/* UNIX Includes */
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/wait.h>
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

	void launch(int fdin, int fdout, int fderr,
				int pgid, bool background, bool interactive);
	void setup_process_io(int fdin, int fdout, int fderr);
	bool handle_builtins(int,int,int);
	void handle_modified_commands();

	bool handle_cd(int,int,int);
	bool handle_cl(int,int,int);
	bool handle_setenv(int,int,int);
	bool handle_unsetenv(int,int,int);
	bool completed = false;
	bool stopped = false;
	int status = -1;
	void handle_ls();
	void handle_grep();
	void handle_printenv();
	pid_t pid;
};
#endif
