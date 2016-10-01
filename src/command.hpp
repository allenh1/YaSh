#ifndef __COMMAND_HPP__
#define __COMMAND_HPP__
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
};

class Command
{
public:
   Command();

   void prompt();
   void print();
   void execute();
   void clear();

   void pushDir(const char * new_dir);
   void popDir();
   
   void ctrlc_handler(int);
   void sigchld_handler(int);
	       
   void insertSimpleCommand(std::shared_ptr<SimpleCommand> simpleCommand);

   void setInFile(char * fd)  { inFile  = std::unique_ptr<char>(fd);  inSet = true; }
   void setOutFile(char * fd) { outFile = std::unique_ptr<char>(fd); outSet = true; }
   void setErrFile(char * fd) { errFile = std::unique_ptr<char>(fd); errSet = true; }

   void setAlias(const char * _from, const char * _to);

   void subShell(char * arg);

   const bool & outIsSet() { return outSet; }
   const bool & errIsSet() { return errSet; }
   const bool & inIsSet()  { return inSet; }

   void setAppend(const bool & ap) { append = ap; }
   void setBackground(bool && bg) { background = bg; }

   static Command currentCommand;
   static std::shared_ptr<SimpleCommand> currentSimpleCommand;

   std::map<std::string, std::vector<std::string> > m_aliases;
   
   std::vector<std::string> wc_collector;// Wild card collection tool

   bool printPrompt = true;
private:
   std::vector<std::string> string_split(std::string s, char delim) {
	  std::vector<std::string> elems; std::stringstream ss(s);
	  for (std::string item;std::getline(ss, item, delim); elems.push_back(item));
	  return elems;
   }

   std::unique_ptr<char> outFile;
   std::unique_ptr<char> inFile;
   std::unique_ptr<char> errFile;

   bool append = false;
   bool background = false;
   int numOfSimpleCommands = 0;

   bool outSet = false;
   bool errSet = false;
   bool inSet  = false;

   std::vector<std::string> m_dir_stack;
   
   std::vector<std::shared_ptr<SimpleCommand> > simpleCommands;
   std::vector<std::string> m_history;
};

/**
 * For the sorting of the wild card output
 */
struct Comparator {
   char toLower(char c) { return ('A' <= c && c <= 'Z') ? 'a' + (c - 'A') : c; }
   bool isAlnum(char c) { return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z')
		 || ('0' <= c && c <= '9'); }

   bool operator() (const std::string & s1, const std::string & s2) {
	  const char * temp1 = s1.c_str(), *temp2 = s2.c_str();
	  if (s1 == "files" && s2 == "file-list") return false;
	  for (; *temp1 && *temp2; ++temp1, ++temp2) {
		 if (*temp1 != *temp2) {
			if (!(isAlnum(*temp1) && isAlnum(*temp2))) {
			   if (!isAlnum(*temp1) && isAlnum(*temp2)) return false;
			   else if (isAlnum(*temp1) && !isAlnum(*temp2)) return true;
			   else return *temp1 < *temp2;
			}
			if (toLower(*temp1) != toLower(*temp2)) return toLower(*temp1) < toLower(*temp2);
		 }
	  }
	  return s1.size() < s2.size();
   }
};

/* Signal Handlers */
void sigchld_handler(int signum);
void ctrlc_handler(int signum);
#endif
