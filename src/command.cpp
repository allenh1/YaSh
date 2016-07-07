/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <stdio.h>
#include <fcntl.h>
#include <thread>
#include <vector>
#include <pwd.h>

#include "command.hpp"

std::vector<int> m_background;

SimpleCommand::SimpleCommand() { /** Used to call malloc for five stuffs here **/ }

/**
 * @brief      Inserts an argument to the current list of args.
 *
 * @param      argument  argument to insert.
 */

char * longest_substring(const std::vector<std::string> & _vct) {
  char * _substr = NULL; std::vector<char*> vct;
  for (auto && x : _vct) vct.push_back(strndup(x.c_str(), x.size()));
  std::sort(vct.begin(), vct.end(), Lensort());
  size_t minlen = strlen(vct[0]); char * last = NULL; int y = 1;
  for (char * s = strndup(vct[0],1); y < minlen; s = strndup(vct[0], y++)) {
    register volatile unsigned short count = 0;
    for (auto && x : vct) if (!strncmp(x, s, minlen)) ++count;
    if (count == vct.size()) free(last), last = s;
    else free(s);
  } return last;
}

size_t size_of_longest(const std::vector<std::string> & _vct) {
  size_t max = 0;
  for (auto && x : _vct) max = (x.size() > max) ? x.size() : max;
  return max;
}

void printEvenly(std::vector<std::string> & _vct) {
  size_t longst = size_of_longest(_vct); std::string * y;
  struct winsize w; ioctl(1, TIOCGWINSZ, &w);
  for(; w.ws_col % longst; ++longst);
  int inc = _vct.size() / longst;
  for (size_t x = 0; x < _vct.size();) {
    for (size_t width = 0; width != w.ws_col; width += longst) {
      if (x == _vct.size()) break;
      y = new std::string(_vct[x++]);
      for(;y->size() < longst; *y += " ");
      std::cerr<<*y;
    } std::cerr<<std::endl;
  }
}

inline std::string tilde_expand(std::string input)
{
  std::string substr = input.substr(0, input.find_first_of('/'));
  if (*substr.c_str() == '~') {
    std::string user = substr.substr(1, substr.size());
    if (user.size() > 0) {
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
    } else if (*str == '\\' && *(str + 1) == '#') {
      temp[index++] = '#'; ++str;
    } else if (*str == '\\' && *(str + 1) == '<') {
      temp[index++] = '<'; ++str;
    } else if (*str == '\\' && *(str + 1) == '\\' && *(str+2) == '\\') {
      temp[index++] = '\\'; ++str; ++str;
    } else if (*str == '\\' && *(str + 1) == '\\'){
      temp[index++] = '\\'; ++str;
    } else if (*str == '\\' && *(str + 1) == ' ') {
      temp[index++] = ' '; ++str;
    } else {
      temp[index++] = *str;
    }
  } free(t_str);
  char * toPush = new char[index + 1]; memset(toPush, 0, index + 1);
  strcpy(toPush, temp);
  arguments.push_back(toPush), ++numOfArguments;
  free(temp);
}

inline int eval_to_buffer(char * const* cmd, char * outBuff, size_t buffSize)
{
  int fdpipe[2]; int pid = -1; size_t x = 0;

  if (pipe(fdpipe) < 0) return -1;
  else if ((pid = fork()) < 0) return -1;
  else if (pid == 0) {
    /** Child Process: write into the pipe **/
    close(fdpipe[0]);   // close unused read end
    dup2(fdpipe[1], 1); // stdout to pipe
    dup2(fdpipe[1], 2); // stderr to pipe
    close(fdpipe[1]);

    if (execvp(cmd[0], cmd)) return -1;
    _exit(0);
  } else {
    /** Parent Process: read from the pipe **/
    close(fdpipe[1]);   // close unused write end
    for (memset(outBuff, 0, buffSize);x = read(fdpipe[0], outBuff, buffSize););
    if (x == buffSize) return -1;
    waitpid(pid, NULL, 0);
  } return 0;
}

#define SUBSH_MAX_LEN 4096
void Command::subShell(char * arg)
{
  int fdpipe[2]; int pid = -1; size_t x = 0;
  int gdpipe[2]; const size_t buffSize = SUBSH_MAX_LEN;
  // Run /proc/self/exe with "arg" as input
  if (pipe(fdpipe) < 0) { perror("pipe"); return; }
  else if (pipe(gdpipe) < 0) { perror("pipe"); return; }
  else if ((pid = fork()) < 0) { perror("fork"); return; }
  else if (pid == 0) {
    /** Child Process: write into the pipe **/
    dup2(fdpipe[0], 0); // pipe to stdin
    close(fdpipe[0]);
    dup2(fdpipe[1], 1); // stdout to pipe
    dup2(fdpipe[1], 2); // stderr to pipe
    close(fdpipe[1]);
    
    if (execlp("hsh", "hsh", NULL)) { perror("execlp"); return; }
    _exit(0);
  } else {
    /** Parent Process: read from the pipe **/
    char * outBuff = (char*) alloca(buffSize);
    std::string cmd = std::string(arg) + "\nexit";
    char * _arg = strndup(cmd.c_str(), cmd.size());
    std::thread _thread1([_arg, fdpipe]() {
	for (char * a = _arg; *a && write(fdpipe[1], a, 1); ++a);
      }); _thread1.join();
    std::thread _thread2([outBuff, buffSize, fdpipe]() {
	size_t x = 0;
	for (memset(outBuff, 0, buffSize);x = read(fdpipe[0], outBuff, buffSize););
      }); _thread2.join();
    if (x == buffSize) return;
    waitpid(pid, NULL, 0);

    free(_arg); // free already used command input
    char * _cpy = strdup(outBuff); _cpy[strlen(_cpy) - 1] = '\0';
    Command::currentSimpleCommand->insertArgument(_cpy);
  }
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
      std::cout<<"\""<< simpleCommands[i]->arguments[j] <<"\" \t";
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

  auto changedir = [] (std::string & s) {
    std::vector<std::string> dir_structure;
    if (*s.c_str() && *s.c_str() != '/') {
      // we need to append the current directory.
      for (;s.back() == '/'; s.pop_back());
      s = std::string(getenv("PWD")) + "/" + s;
    } else if (!*s.c_str()) {
      for (;s.back() == '/'; s.pop_back());
      passwd * _passwd = getpwuid(getuid());
      std::string user_home = _passwd->pw_dir;
      return chdir(user_home.c_str());
    } for(; *s.c_str() != '/' && s.back() == '/'; s.pop_back());
    return chdir(s.c_str());
  };

  // Print contents of Command data structure
  char * dbg = getenv("SHELL_DBG");
  if (dbg && !strcmp(dbg, "YES")) print();

  char * lolz = getenv("LOLZ");
  if (lolz && !strcmp(lolz, "YES")) {
    /// Because why not?
    std::shared_ptr<SimpleCommand> lul(new SimpleCommand());
    char * _ptr = strdup("lolcat");
    lul->insertArgument(_ptr);
    free(_ptr);
    if (strcmp(simpleCommands.back().get()->arguments[0], "cd") &&
	strcmp(simpleCommands.back().get()->arguments[0], "clear") &&
	strcmp(simpleCommands.back().get()->arguments[0], "ssh")) {
      this->insertSimpleCommand(lul);
    }
  }
  
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
    std::vector<char *> curr = simpleCommands.at(x).get()->arguments;
    char ** d_args = new char*[curr.size() + 1];
    for (int y = 0; y < curr.size(); ++y) {
      d_args[y] = strdup(curr[y]);
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
	std::string _empty = "";
	cd = changedir(_empty);
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
	  for (;new_dir.back() == '/'; new_dir.pop_back());
	  new_dir += "/" + std::string(d_args[1]);
	} else new_dir = std::string(d_args[1]);

	for (; *new_dir.c_str() != '/' && new_dir.back() == '/'; new_dir.pop_back());
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
	temp[y] = strdup(curr[y - 1]);
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
	temp[y] = strdup(curr[y]);
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
  if (!printPrompt) return;
  std::string PROMPT; char * pmt = getenv("PROMPT");
  if (pmt) PROMPT = std::string(pmt);
  else PROMPT = std::string("default");

  if (isatty(0) && PROMPT == std::string("default")) {
    std::string _user = std::string(getenv("USER"));
    char buffer[100]; std::string _host;
    if (!gethostname(buffer, 100)) _host = std::string(buffer);
    else _host = std::string("localhost");
    char * _wd = NULL, * _hme = NULL;
    char cdirbuff[2048]; char * const _pwd[2] = { (char*) "pwd", (char*) NULL };
    eval_to_buffer(_pwd, cdirbuff, 2048);
    std::string _cdir = std::string(cdirbuff);
    char * _curr_dur = strndup(_cdir.c_str(), _cdir.size() - 1);
    if (setenv("PWD", _curr_dur, 1)) perror("setenv");
    std::string _home = std::string(_hme = getenv("HOME"));

    // Replace the user home with ~
    if (strstr(_curr_dur, _hme)) _cdir.replace(0, _home.size(), std::string("~"));

    if (_user != std::string("root")) {
      std::cout<<"\x1b[36;1m"<<_user<<"@"<<_host<<" ";
      std::cout<<"\x1b[33;1m"<<_cdir<<"$ "<<"\x1b[0m";
      fflush(stdout);
    } else {
      // Root prompt is cooler than you
      std::cout<<"\x1b[31;1m"<<_user<<"@"<<_host<<" ";
      std::cout<<"\x1b[35;1m"<<_cdir<<"# "<<"\x1b[0m";
      fflush(stdout);
    } free(_curr_dur);
  } else {
    std::cout<<PROMPT;
    fflush(stdout);
  }
}

Command Command::currentCommand;
std::shared_ptr<SimpleCommand> Command::currentSimpleCommand;

int yyparse(void);

void ctrlc_handler(int signum)
{
  /** kill my kids **/
  kill(signum, SIGINT);
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


