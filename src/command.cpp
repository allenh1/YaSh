/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <fcntl.h>
#include <signal.h>
#include <vector>
#include <pwd.h>
#include <iostream>
#include <fstream>
#include <cstring>

#include "command.hpp"

std::vector<int> m_background;

SimpleCommand::SimpleCommand() { /** Used to call malloc for five stuffs here **/ }

/**
 * @brief      Inserts an argument to the current list of args.
 *
 * @param      argument  argument to insert.
 */

inline std::string tilde_expand(std::string input)
{
  std::string substr = input.substr(0, input.find_first_of('/'));
  if (*substr.c_str() == '~') {
    std::string user = substr.substr(1, substr.size());
    if (user.size() > 0) {
      // Case of a user
      getenv("USER"); // your user
      passwd * _passwd = getpwnam(user.c_str());
      std::string _home = _passwd->pw_dir;
      input.replace(0, substr.size(), _home);
      return input;
    } else {
      // Case current user (that is, the usual case).
      passwd * _passwd = getpwuid(getuid());
      std::string user_home = _passwd->pw_dir;
      input.replace(0, substr.size(), user_home);
      return input;
    }
  }
  return input;
}

inline std::string replace(std::string str, const char * sb, const char * rep)
{
  std::string sub = std::string(sb);
  size_t pos = str.find(rep);
  if (pos != std::string::npos) {
    std::string p1 = str.substr(0, pos - 1);
    std::string p2 = str.substr(pos + std::string(sub).size() - 1);
    std::string tStr = p1 + rep + p2;
    return tStr;
  } else return str;
}

std::string env_expand(std::string s)
{
  const char * str = s.c_str();
  char * temp = (char*) calloc(1024, sizeof(char));
  int index;
  for (index = 0; *str; ++str) {
    // aight. Let's just do it.
    if (*str == '$') {
      // begin expansion
      if (*(++str) != '{') continue;
      // @todo maybe work without braces.
      char * temp2 = (char*) calloc(s.size(), sizeof(char)); ++str;
      for (char * tmp = temp2; *str && *str != '}'; *(tmp++) = *(str++));
      if (*str == '}') {
	++str; char * out = getenv(temp2);
	if (out == NULL) continue;;
	for (char * t = out; *t; temp[index++] = *(t++));
      } delete[] temp2;
    }// if not a variable, don't expand.
    temp[index++] = *str;
  } std::string ret = std::string((char*)temp);
  free(temp);
  return ret;
}

void SimpleCommand::insertArgument(char * argument)
{
  std::string arga = tilde_expand(std::string(argument));
  std::string arg  = env_expand(arga);
  char * str = strdup(arg.c_str()); int index; char * t_str = str;
  char * temp = (char*) calloc(arg.size() + 1, sizeof(char));
  for (index = 0; *str; ++str) {
    if (*str == '\\' && *(str + 1) == '\"') {
      temp[index++] = '\"'; ++str;
    } else if (*str == '\\' && *(str + 1) == '&') {
      temp[index++] = '&'; ++str;
    } else if (*str == '\\' && *(str + 1) == '<') {
      temp[index++] = '<'; ++str;
    } else if (*str == '\\' && *(str + 1) == '\\' && *(str+2) == '\\') {
      temp[index++] = '\\'; ++str; ++str;
    } else if (*str == '\\' && *(str + 1) == '\\'){
      temp[index++] = '\\'; ++str;
    } else {
      temp[index++] = *str;
    }
  } free(t_str);
  // char * toPush = (char*) calloc(index + 1, sizeof(char));
  char * toPush = new char[index + 1]; memset(toPush, 0, index + 1);
  strcpy(toPush, temp);
  arguments.push_back(std::shared_ptr<char>(toPush)), ++numOfArguments;
  free(temp);
}

void Command::subShell(char * arg)
{
  auto changedir = [] (std::string s) {
    std::vector<std::string> dir_structure;

    if (*s.c_str() && *s.c_str() != '/') {
      // we need to append the current directory.
      s = std::string(getenv("PWD")) + "/" + s;
      //std::cerr<<"Directory changed: "<<s<<std::endl;
    } else if (!*s.c_str()) {
      passwd * _passwd = getpwuid(getuid());
      std::string user_home = _passwd->pw_dir;
      return chdir(user_home.c_str());
    }
    return chdir(s.c_str());
  };

  //std::cerr<<"arg: "<<arg<<std::endl;
  std::string argument(arg);
  const char * temp = "/tmp/out.txt";
  const char * errr = "/tmp/err.txt";

  std::ofstream inout("/tmp/in.txt");
  inout << arg <<std::endl;
  inout.close();

  // We will run "/proc/self/exe"
  /**
   * Stragegy:
   * Run the shell's executable, passing the argument that
   * was input within back-ticks. The output is stored in
   * a file, which we will read in and place in Command's
   * current arguments.
   */

  // Definitely can call /proc/self/exe
  int pid;

  pid = fork(); // Fork it
  char ** args = new char*[6];// cmd, arg, NULL
  args[0] = strdup("/bin/bash");
  //args[1] = strdup("<");
  args[1] = strdup(argument.c_str());
  args[2] = strdup(">");
  args[3] = strdup("/tmp/out.txt");
  args[4] = NULL;

  /** Run the shell **/ 
  if (pid == 0) {
    execvp(args[0], args);
    perror("subshell");
    _exit(2);
  } delete[] args;

  /** Read back the file **/
  std::ifstream fin ("/tmp/out.txt");
  std::vector<std::string> words;
  for (std::string word; fin.eof(); words.push_back(word)) fin >> word;
  for (auto && w : words) currentSimpleCommand.get()->insertArgument(strdup(w.c_str()));
  fin.close();
}

Command::Command()
{ /** Constructor **/ }

void Command::insertSimpleCommand(std::shared_ptr<SimpleCommand> simpleCommand)
{ simpleCommands.push_back(simpleCommand), ++numOfSimpleCommands; }

void Command::clear()
{
  simpleCommands.clear(),
    background = append = false,
    numOfSimpleCommands = 0,
    outFile.release(), inFile.release(),
    errFile.release(), simpleCommands.shrink_to_fit();
  outSet = inSet = errSet = false;
}

void Command::print()
{
  std::cout<<std::endl<<std::endl;
  std::cout<<"              COMMAND TABLE                "<<std::endl;  
  std::cout<<std::endl; 
  std::cout<<"  #   Simple Commands"<<std::endl;
  std::cout<<"  --- ----------------------------------------------------------"<<std::endl;

  for (int i = 0; i < numOfSimpleCommands; i++) {
    printf("  %-3d ", i);
    for (int j = 0; j < simpleCommands[i]->numOfArguments; j++) {
      std::cout<<"\""<< simpleCommands[i]->arguments[j].get() <<"\" \t";
    }
  }

  std::cout<<std::endl<<std::endl;
  std::cout<<"  Output       Input        Error        Background"<<std::endl;
  std::cout<<"  ------------ ------------ ------------ ------------"<<std::endl;
  printf("  %-12s %-12s %-12s %-12s\n", outFile.get()?outFile.get():"default",
	 inFile.get()?inFile.get():"default", errFile.get()?errFile.get():"default",
	 background?"YES":"NO");
  std::cout<<std::endl<<std::endl;
}


void Command::execute()
{
  // Don't do anything if there are no simple commands
  if (numOfSimpleCommands == 0) {
    prompt();
    return;
  }

  auto changedir = [] (std::string s) {
    std::vector<std::string> dir_structure;

    if (*s.c_str() && *s.c_str() != '/') {
      // we need to append the current directory.
      s = std::string(getenv("PWD")) + "/" + s;
      //std::cerr<<"Directory changed: "<<s<<std::endl;
    } else if (!*s.c_str()) {
      passwd * _passwd = getpwuid(getuid());
      std::string user_home = _passwd->pw_dir;
      return chdir(user_home.c_str());
    }
    return chdir(s.c_str());
  };

  // Print contents of Command data structure
  //  print();

  // Add execution here
  // For every simple command fork a new process
  int pid = -1;
  int fdpipe [2]; //for de pipes
  int fderr, fdout, fdin; char * blak;
  int tmpin  = dup(0);
  int tmpout = dup(1);
  int tmperr = dup(2);

  // Redirect the input
  if (inFile.get()) {
    fdin  = open(inFile.get(),  O_WRONLY, 0700);
  } else fdin = dup(tmpin);

  if (outFile.get()) {
    if (this->append) fdout = open(outFile.get(), O_CREAT | O_WRONLY | O_APPEND, 0600);
    else fdout = open(outFile.get(), O_CREAT | O_WRONLY | O_TRUNC, 0600);
  } else fdout = dup(tmpout);

  if (errFile.get()) {
    if (this->append) fderr = open(errFile.get(), O_CREAT | O_WRONLY | O_APPEND, 0600);
    else fderr = open(errFile.get(), O_CREAT | O_WRONLY | O_TRUNC, 0600);
  } else fderr = dup(tmperr);

  if (fderr < 0) {
    perror("error file");
    exit(1);// could not open file
  } if (fdout < 0) {
    perror("out file");
    exit(1);
  } if (fdin < 0) {
    perror("in file");
    exit(1);
  }

  // Redirect stderr to file
  dup2(fderr, 2);
  close(fderr);
  for (int x = 0; x < numOfSimpleCommands; ++x) {
    std::vector<std::shared_ptr<char> > curr = simpleCommands.at(x).get()->arguments;
    char ** d_args = new char*[curr.size() + 1];
    for (int y = 0; y < curr.size(); ++y) {
      d_args[y] = strdup(curr[y].get());
    } // ... still better than managing myself!
    d_args[curr.size()] = NULL;
    // output redirection
    dup2(fdin, 0);
    close(fdin);

    // std::cerr<<"Append? "<<((this->append) ? "Yes." : "No.")<<std::endl;
    /** output direction **/
    if (x == numOfSimpleCommands - 1) {
      if (outFile.get()) {
	if (this->append) fdout = open(outFile.get(), O_CREAT|O_WRONLY|O_APPEND, 600);
	else fdout = open(outFile.get(), O_CREAT|O_WRONLY|O_TRUNC, 0600);
      } else fdout = dup(tmpout);

      if (errFile.get()) {
	if (this->append) fderr = open(outFile.get(), O_CREAT|O_WRONLY|O_APPEND, 600);
	else fderr = open(errFile.get(), O_CREAT|O_WRONLY|O_TRUNC, 0600);
      } else fderr = dup(tmperr);
    } // direct to requested upon last command

    else {
      int fdpipe[2];
      int pipe_res = pipe(fdpipe);
      // perror("pipe");
      fdout = fdpipe[1];
      fdin  = fdpipe[0];
    } // direct to input of next command

    dup2(fdout, 1);
    close(fdout);

    if (d_args[0] == std::string("cd")) {
      std::string curr_dir = std::string(getenv("PWD"));
      int cd; std::string new_dir;
      if (curr.size() < 2) {
	cd = changedir(std::string(""));
	setenv("PWD", getenv("HOME"), 1);
      } else {
	if (d_args[1] == std::string("pwd") ||
	    d_args[1] == std::string("/bin/pwd")) {
	  // Nothing to be done.
	  clear();
	  prompt();
	  return;
	} else if (*d_args[1] != '/') {
	  new_dir = std::string(getenv("PWD"));
	  new_dir += "/" + std::string(d_args[1]);
	} else new_dir = std::string(d_args[1]);

	cd = changedir(new_dir);
	setenv("PWD", new_dir.c_str(), 1);
      }

      if (cd != 0) {
	perror("cd failed");
	setenv("PWD", curr_dir.c_str(), 1);
      }
      // Regardless of errors, cd has finished.
      clear();
      prompt();
      return;
    } else if (d_args[0] == std::string("ls")) {
      char ** temp = new char*[curr.size() + 2];
      for (int y = 2; y <= curr.size(); ++y) {
	temp[y] = strdup(curr[y - 1].get());
      } // ... still better than managing myself!
      temp[0] = strdup("ls");
      temp[1] = strdup("--color=auto");
      temp[curr.size() + 1] = NULL;

      pid = fork();

      if (pid == 0) {
	execvp (temp[0], temp);
	perror("execvp");
	exit(2);
      } 
      for (int x = 0; x < curr.size() + 2; ++x) {
	free(temp[x]);
	temp[x] = NULL;
      } delete[] temp;
    } else if (d_args[0] == std::string("setenv")) {
      char * temp = (char*) calloc(strlen(d_args[1]) + 1, sizeof(char));
      char * pemt = (char*) calloc(strlen(d_args[2]) + 2, sizeof(char));
      strcpy(temp, d_args[1]); strcpy(pemt, d_args[2]);
      int result = setenv(temp, pemt, 1);
      if (result) perror("setenv");
      clear(); prompt(); free(temp); free(pemt);
      return;
    } else if (d_args[0] == std::string("unsetenv")) {
      int result = unsetenv(d_args[1]);
      if (result) perror("unsetenv");
      clear(); prompt();
      return;
    } else if (d_args[0] == std::string("grep")) {
      char ** temp = new char*[curr.size() + 2];
      for (int y = 1; y < curr.size(); ++y) {
	temp[y] = strdup(curr[y].get());
      } // ... still better than managing myself!
      temp[0] = strdup("grep");
      temp[curr.size()] = strdup("--color");
      temp[curr.size() + 1] = NULL;

      pid = fork();

      if (pid == 0) {
	execvp (temp[0], temp);
	perror("execvp");
	exit(2);
      } for (int x = 0; x < curr.size() + 2; ++x) {
	free(temp[x]); temp[x] = NULL;
      } delete[] temp;
    } else {
      // Time for a nice fork
      pid = fork();

      if (pid == 0) {
	if (d_args[0] == std::string("printenv")) {
	  char ** _env = environ;
	  for (; *_env; ++_env) std::cout<<*_env<<std::endl;
	  _exit (0);
	}

	execvp (d_args[0], d_args);
	perror("execvp");
	_exit(1);
      }
    }
    //delete d_args;
    for (int x = 0; x < curr.size(); ++x) {
      free(d_args[x]); d_args[x] = NULL;
    } delete[] d_args;
  }

  // Restore I/O defaults

  dup2(tmpin, 0);
  dup2(tmpout, 1);
  dup2(tmperr, 2);
  close(tmpin);
  close(tmpout);
  close(tmperr);

  if (!background) {
    // We are not running in the backround,
    // so we should wait for the final command
    // to execute.

    waitpid(pid, NULL, 0);
  } else m_background.push_back(pid);

  // Clear to prepare for next command
  clear();

  // Print new prompt if we are in a terminal.
  if (isatty(0)) {
    prompt();
  }
}

// Shell implementation

void Command::prompt()
{
  std::string PROMPT; char * pmt = getenv("PROMPT");
  if (pmt) PROMPT = std::string(pmt);
  else PROMPT = std::string("default");

  if (isatty(0) || PROMPT == std::string("default")) {
    std::string _user = std::string(getenv("USER"));
    char buffer[100]; std::string _host;
    if (!gethostname(buffer, 100)) _host = std::string(buffer);
    else _host = std::string("localhost");
    char * _wd = NULL, * _hme = NULL;
    std::string _cdir = std::string( _wd = getenv("PWD"));
    std::string _home = std::string(_hme = getenv("HOME"));

    // Replace the user home with ~
    if (strstr(_wd, _hme)) _cdir.replace(0, _home.size(), std::string("~"));

    if (_user != std::string("root")) {
      std::cout<<"\x1b[36;1m"<<_user<<"@"<<_host<<" ";
      std::cout<<"\x1b[33;1m"<<_cdir<<std::endl<<"$ "<<"\x1b[0m";
      fflush(stdout);
    } else {
      // Root prompt is cooler than you
      std::cout<<"\x1b[91;1m"<<_user<<"@"<<_host<<" ";
      std::cout<<"\x1b[95;1m"<<_cdir<<std::endl<<"# "<<"\x1b[0m";
      fflush(stdout);
    }
  }
}

Command Command::currentCommand;
std::shared_ptr<SimpleCommand> Command::currentSimpleCommand;

int yyparse(void);

void ctrlc_handler(int signum)
{
  /** kill my kids **/
  // std::cout<<std::endl;
  // Command::currentCommand.clear();
  // Command::currentCommand.prompt();
}

void sigchld_handler(int signum)
{
  int saved_errno = errno; int pid = -1;
  for(; pid = waitpid(-1, NULL, WNOHANG) > 0;) {
    bool found = false;
    for (auto && a : m_background) {
      if (a == pid) {
	found = true;
	break;
      }
    } if (found) {
      std::cout<<"["<<signum<<"] has exited."<<std::endl;
      Command::currentCommand.prompt();
    }
  } errno = saved_errno;
}

  int main()
  {
    struct sigaction ctrl_action;
    ctrl_action.sa_handler = ctrlc_handler;
    sigemptyset(&ctrl_action.sa_mask);
    ctrl_action.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &ctrl_action, NULL)) {
      perror("sigint");
      _exit(6);
    }

    struct sigaction chld;
    chld.sa_handler = sigchld_handler;
    sigemptyset(&chld.sa_mask);
    chld.sa_flags   = SA_RESTART | SA_NOCLDSTOP;

    if (sigaction(SIGCHLD, &chld, NULL) == -1) {
      perror("sigchild");
      _exit(7);
    } 
    Command::currentCommand.prompt();
    yyparse();

    return 0;
  }


