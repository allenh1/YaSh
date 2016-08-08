#ifndef __WILDCARD_HPP__
#define __WILDCARD_HPP__
/* STL (C++) includes */
#include <algorithm>
#include <iostream>
#include <queue>

/* C includes */
#include <string.h>
#include <dirent.h>
#include <regex.h>
#include <fcntl.h>

#include "command.hpp"

#define MAXLEN 1024

extern "C" void wildcard_expand(char * arg);
#endif
