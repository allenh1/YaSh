#ifndef __SHELL_UTILS_HPP__
#define __SHELL_UTILS_HPP__
#include <sys/ioctl.h>

#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <pwd.h>

#include <iostream>
#include <cstring>
#include <string>
#include <vector>

std::string longest_substring(const std::vector<std::string> & _vct);
size_t size_of_longest(const std::vector<std::string> & _vct);
void printEvenly(std::vector<std::string> & _vct);
std::string tilde_expand(std::string input);
inline std::string replace(std::string str, const char * sb, const char * rep);
std::string env_expand(std::string s);

struct Lensort {
	bool operator () (char*& ch1, char*& ch2) { return strlen(ch1) < strlen(ch2); }
};
#endif
