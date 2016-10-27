#ifndef __WILDCARD_HPP__
#define __WILDCARD_HPP__
/* STL (C++) includes */
#include <algorithm>
#include <iostream>
#include <glob.h>

#include "command.hpp"

#define MAXLEN 1024

extern "C" void wildcard_expand(char * arg);
#endif
