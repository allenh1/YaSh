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
#include <utility>
#include <memory>
#include <string>
#include <vector>

#include "command.hpp"

std::vector<int> m_background;

SimpleCommand::SimpleCommand()
{SimpleCommand::p_jobs = Command::currentCommand.m_p_jobs;}

void SimpleCommand::insertArgument(const std::shared_ptr<char> argument)
{
  std::string as_string(argument.get());

  auto exists = Command::currentCommand.m_aliases.find(as_string);

  /* exists == m_aliases.end() => we are inserting an alias */
  if (exists != Command::currentCommand.m_aliases.end()) {
    auto to_insert = exists->second;
    for (auto && str : to_insert) {
      char * toPush = strndup(str.c_str(), str.size());
      arguments.push_back(toPush); ++numOfArguments;
    }
  } else {
    std::string arga = tilde_expand(as_string);
    std::string arg = env_expand(arga);

    char * str = new char[arg.size() + 1];
    memcpy(str, arg.c_str(), arg.size());
    str[arg.size()] = '\0';

    arguments.push_back(str), ++numOfArguments;
  }
}

inline int eval_to_buffer(char * const * cmd, char * outBuff, size_t buffSize)
{
  int fdpipe[2]; int pid = -1; size_t x = 0;

  if (pipe(fdpipe) < 0) {return -1;} else if ((pid = fork()) < 0) {return -1;} else if (pid == 0) {
    /** Child Process: write into the pipe **/
    close(fdpipe[0]);       // close unused read end
    dup2(fdpipe[1], 1);     // stdout to pipe
    dup2(fdpipe[1], 2);     // stderr to pipe
    close(fdpipe[1]);

    if (execvp(cmd[0], cmd)) {return -1;}
    _exit(0);
  } else {
    /** Parent Process: read from the pipe **/
    close(fdpipe[1]);       // close unused write end
    for (memset(outBuff, 0, buffSize); x = read(fdpipe[0], outBuff, buffSize); ) {
    }
    if (x == buffSize) {
      close(fdpipe[0]);
      return -1;
    }

    waitpid(pid, nullptr, 0);
    close(fdpipe[0]);
  } return 0;
}

int Command::get_output_flags()
{
  return O_CREAT | O_WRONLY | ((append) ? O_APPEND : O_TRUNC);
}

void Command::set_in_file(const std::shared_ptr<char> _fd)
{
  std::string expanded = tilde_expand(_fd.get());
  inFile = std::make_unique<std::string>(expanded);
  inSet = true;

  m_stdin = open(inFile->c_str(), O_RDONLY, 0600);

  if (m_stdin < 0) {
    perror("open");
    inSet = false;
    inFile = nullptr;
    m_stdin = 0;
  }
}

void Command::set_out_file(const std::shared_ptr<char> _fd)
{
  std::string expanded = tilde_expand(_fd.get());
  outFile = std::make_unique<std::string>((expanded));
  outSet = true;

  m_stdout = open(outFile->c_str(), get_output_flags(), 0600);

  if (m_stdout < 0) {
    perror("open");
    outSet = false;
    outFile = nullptr;
    m_stdout = 1;
  }
}

void Command::set_err_file(const std::shared_ptr<char> _fd)
{
  std::string expanded = tilde_expand(_fd.get());
  errFile = std::make_unique<std::string>(expanded);
  errSet = true;

  m_stderr = open(errFile->c_str(), get_output_flags(), 0600);

  if (m_stderr < 0) {
    perror("open");
    errSet = false;
    errFile = nullptr;
    m_stderr = 2;
  }
}

Command::Command()
{
  m_p_jobs = std::make_shared<std::vector<job>>();
}

void Command::insertSimpleCommand(std::shared_ptr<SimpleCommand> simpleCommand)
{simpleCommands.push_back(simpleCommand), ++numOfSimpleCommands;}

void Command::clear()
{
  if (m_stdin != 0) {close(m_stdin);}
  if (m_stdout != 1) {close(m_stdout);}
  if (m_stderr != 2) {close(m_stderr);}

  m_stdin = 0, m_stdout = 1, m_stderr = 2;

  simpleCommands.clear(),
    background = append = false,
    numOfSimpleCommands = 0, m_pgid = 0,
    outFile = nullptr, inFile = nullptr,
    errFile = nullptr, simpleCommands.shrink_to_fit(),
    m_p_jobs->shrink_to_fit(), m_expand = true;
  outSet = inSet = errSet = m_time = false;
}

void Command::print()
{
  std::cout << std::endl << std::endl;
  std::cout << "              COMMAND TABLE                " << std::endl;
  std::cout << std::endl;
  std::cout << "  #   Simple Commands" << std::endl;
  std::cout << "  --- ----------------------------------------------------------" << std::endl;

  for (int i = 0; i < numOfSimpleCommands; i++) {
    printf("  %-3d ", i);
    for (int j = 0; j < simpleCommands[i]->numOfArguments; j++) {
      std::cout << "\"" << simpleCommands[i]->arguments[j] << "\" \t";
    }
  }

  std::cout << std::endl << std::endl;
  std::cout << "  Output       Input        Error        Background" << std::endl;
  std::cout << "  ------------ ------------ ------------ ------------" << std::endl;
  printf("  %-12s %-12s %-12s %-12s\n",
    nullptr == outFile ? outFile->c_str() : "default",
    nullptr == inFile ? inFile->c_str() : "default",
    nullptr == errFile ? errFile->c_str() : "default",
    background ? "YES" : "NO");
  std::cout << std::endl << std::endl;
}

void Command::execute()
{
  int fdin = 0, fdout = 1, fderr = 2;
  int fdpipe[2];
  pid_t pid = 0;

  time_t rs, us, ss;
  int rsf, usf, ssf;
  int cpu;

  struct rusage selfb, selfa;
  struct rusage kidsb, kidsa;

  struct timeval real, user, sys;
  struct timeval before, after;

  /* @todo this is not posix compliant */
  struct timezone dtz;

  if (m_time) {
    /* get the time of day */
    gettimeofday(&before, &dtz);
    /* call rusage */
    getrusage(RUSAGE_SELF, &selfb);
    getrusage(RUSAGE_CHILDREN, &selfb);
  }
  /* check for dank memes */
  char * dbg = getenv("SHELL_DBG");
  if (dbg && !strcmp(dbg, "YES")) {print();}

  char * lolz = getenv("LOLZ");
  if (lolz && !strcmp(lolz, "YES")) {
    /// Because why not?
    std::shared_ptr<SimpleCommand> lul(new SimpleCommand());
    std::shared_ptr<char> _ptr = std::shared_ptr<char>(strdup("lolcat"), free);
    lul->insertArgument(_ptr);
    if (strcmp(simpleCommands.back().get()->arguments[0], "cd") &&
      strcmp(simpleCommands.back().get()->arguments[0], "clear") &&
      strcmp(simpleCommands.back().get()->arguments[0], "ssh") &&
      strcmp(simpleCommands.back().get()->arguments[0], "setenv") &&
      strcmp(simpleCommands.back().get()->arguments[0], "unsetenv"))
    {
      this->insertSimpleCommand(lul);
    }
  }

  auto clear_and_prompt = make_scope_exit([this]() {
        clear();
        if (m_interactive) {prompt();}
      });
  /* point fdin (fderr) to this pipeline's input (error) */
  fdin = m_stdin; fderr = m_stderr;
  for (int x = 0; x < numOfSimpleCommands; ++x) {
    /* manage commands */
    std::vector<char *> curr = simpleCommands.at(x).get()->arguments;
    char ** d_args;
    curr.push_back(nullptr);
    d_args = curr.data();

    /* add nullptr to the end of the simple command (for exec) */
    simpleCommands.at(x).get()->arguments.push_back(nullptr);

    if (x != numOfSimpleCommands - 1) {
      /* thank you Gustavo for the outer if statement. */
      if (pipe(fdpipe)) {
        /* pipe failed */
        perror("pipe");
        return;
      }
    }

    /* redirect output */
    if (x != numOfSimpleCommands - 1) {fdout = fdpipe[1];} else {fdout = m_stdout;}

    if (simpleCommands.at(x)->handle_builtins(fdin, fdout, fderr)) {
      goto cleanup;
    } else if ((pid = fork()) < 0) {
      /* fork failed */
      perror("fork");
      return;
    } else if (pid == 0) {
      /* child process: exec into the process group */
      simpleCommands.at(x).get()->launch(
        fdin, fdout, fderr, m_pgid, background, m_interactive);
    } else {
      /* parent process */
      simpleCommands.at(x).get()->pid = pid;
      if (m_interactive) {
        if (!m_pgid) {m_pgid = pid;}
        /* add pid to the pipeline's process group */
        setpgid(pid, m_pgid);
      }
    }

cleanup:
    /* cleanup pipeline */
    if (fdin != m_stdin) {close(fdin);}
    if (fdout != m_stdout) {close(fdout);}
    /* set up the input for the next command */
    fdin = fdpipe[0];
  }

  /* prep to save */
  job current;
  int status = -1;   /* shut up, GCC */
  current.pgid = m_pgid;
  current.command = get_command_text(std::ref(*this));
  current.m_stdin = m_stdin;
  current.m_stdout = m_stdout;
  current.m_stderr = m_stderr;
  current.status = job_status::RUNNING;
  /* waitpid:
   *   pid <  -1 => wait for absolute value of pid
   *   pid == -1 => wait for any child process
   *   pid ==  0 => wait for any child whose pgid is ours
   *   pid >   0 => wait for the specified pid
   */

  if (!background) {
    /* put the job in the foreground */
    if (m_interactive) {
      tcsetpgrp(STDIN_FILENO, m_pgid);
      waitpid(pid, &status, WUNTRACED);
      tcsetpgrp(STDIN_FILENO, m_shell_pgid);
    } else {waitpid(pid, &status, WUNTRACED);}
  } else {m_p_jobs->push_back(current), m_job_map[m_pgid] = m_p_jobs->size() - 1;}

  if (WIFSTOPPED(status)) {
    current.status = job_status::STOPPED;
    m_p_jobs->push_back(current);
    std::cout << "[" << (m_job_map[pid] = m_p_jobs->size() - 1) <<
      "]+\tstopped\t" << pid << std::endl;
  }

  for (pid_t _pid = 0; (_pid = waitpid(-1, &status,
    WUNTRACED | WNOHANG)) > 0; )
  {
    const auto & x = m_job_map.find(_pid);
    if (x != m_job_map.end()) {
      /* x->second is the value */
      std::cout << "[" << (m_job_map[_pid] = m_p_jobs->size() - 1) <<
        "]-\texited" << std::endl;
    }
  }

  /* stop times */
  if (m_time) {
    gettimeofday(&after, &dtz);
    getrusage(RUSAGE_SELF, &selfa);     /* @todo do error checking */
    getrusage(RUSAGE_CHILDREN, &kidsa);     /* @todo do error checking */

    rs = us = ss = 0;
    rsf = usf = ssf = 0;

    /* get the real time */
    real = after - before;
    timeval_to_secs(real, rs, rsf);     /* convert time */


    addtimeval(user, difftimeval(after, selfb.ru_utime, selfa.ru_utime),
      difftimeval(before, kidsb.ru_utime, kidsa.ru_utime));
    timeval_to_secs(user, us, usf);

    addtimeval(sys, difftimeval(after, selfb.ru_stime, selfa.ru_stime),
      difftimeval(before, kidsb.ru_stime, kidsa.ru_stime));
    timeval_to_secs(sys, ss, ssf);

    /* display the times */
    fprintf(stderr, "\n  Real:\t%ld.%03ds", rs, rsf);
    fprintf(stderr, "\n  User:\t%ld.%03ds", us, usf);
    fprintf(stderr, "\nSystem:\t%ld.%03ds", ss, ssf);
    std::cerr << std::endl;
  }
}

void Command::prompt()
{
  if (!printPrompt) {return;}
  std::string PROMPT; char * pmt = getenv("PROMPT");
  if (pmt) {PROMPT = std::string(pmt);} else {PROMPT = std::string("default");}

  if (isatty(0) && PROMPT == std::string("default")) {
    std::string _user = std::string(getenv("USER"));
    char buffer[100]; std::string _host;
    if (!gethostname(buffer, 100)) {_host = std::string(buffer);} else {
      _host = std::string("localhost");
    }
    char * _wd = nullptr, * _hme = nullptr;
    auto _pwd = std::shared_ptr<char>(
        new char[4], [] (auto s) { delete[] s; });
    snprintf(_pwd.get(), sizeof(_pwd.get()), "pwd");
    char cdirbuff[2048];
    char * const pwd[2] = {
        _pwd.get(),
        nullptr
    };
    eval_to_buffer(pwd, cdirbuff, 2048);
    std::string _cdir = std::string(cdirbuff);
    char * _curr_dur = strndup(_cdir.c_str(), _cdir.size() - 1);
    if (setenv("PWD", _curr_dur, 1)) {perror("setenv");}
    std::string _home = std::string(_hme = getenv("HOME"));

    // Replace the user home with ~
    if (strstr(_curr_dur, _hme)) {_cdir.replace(0, _home.size(), std::string("~"));}

    if (_user != std::string("root")) {
      std::cout << "\x1b[36;1m" << _user << "@" << _host << " ";
      std::cout << "\x1b[33;1m" << _cdir << "$ " << "\x1b[0m";
      fflush(stdout);
    } else {
      // Root prompt is cooler than you
      std::cout << "\x1b[31;1m" << _user << "@" << _host << " ";
      std::cout << "\x1b[35;1m" << _cdir << "# " << "\x1b[0m";
      fflush(stdout);
    } free(_curr_dur);
  } else {fflush(stdout);}
}

Command Command::currentCommand;
std::shared_ptr<SimpleCommand> Command::currentSimpleCommand;

int yyparse(void);

std::vector<std::string> splitta(std::string s, char delim)
{
  std::vector<std::string> elems; std::stringstream ss(s);
  std::string item;
  for (; std::getline(ss, item, delim); elems.push_back(std::move(item))) {
  }
  return elems;
}

void Command::setAlias(const std::shared_ptr<char> _from, const std::shared_ptr<char> _to)
{
  std::string from(_from.get()); std::string to(_to.get());
  std::vector<std::string> split = splitta(to, ' ');

  /**
   * We really don't care if the alias has been
   * set. We should just overwrite the current
   * alias. So, we just use the [] operator
   * in map.
   */

  m_aliases[from] = split;
}

void Command::pushDir(const std::shared_ptr<char> new_dir)
{
  char * _pwd = getenv("PWD");
  if (_pwd == nullptr) {
    perror("pwd");
    return;
  } else if (new_dir == nullptr || *new_dir == '\0') {
    std::cerr << "Invalid new directory!" << std::endl;
    return;
  }
  std::string curr_dir = std::string(getenv("PWD"));
  std::string news(new_dir.get());
  news = tilde_expand(news);

  if (news.find_first_of("*") != std::string::npos) {news = curr_dir + "/" + news;}
  auto to_expand = std::shared_ptr<char>(strndup(news.c_str(), news.size()), free);
  wildcard_expand(to_expand);

  if (!wc_collector.size() && changedir(news)) {
    m_dir_stack.insert(m_dir_stack.begin(), curr_dir);
  } else if (wc_collector.size()) {
    for (int y = wc_collector.size() - 1; y--; ) {
      auto x = wc_collector[y];
      if (is_directory(x)) {m_dir_stack.insert(m_dir_stack.begin(), x);}
    }
    if (m_dir_stack.size()) {
      changedir(m_dir_stack[0]);
      m_dir_stack.erase(m_dir_stack.begin(), m_dir_stack.begin() + 1);
      m_dir_stack.push_back(curr_dir);
    } else {goto clear_and_exit;}
  } else {goto clear_and_exit;}

  for (auto && a : m_dir_stack) {
    std::cout << a << " ";
  }
  if (!m_dir_stack.empty()) {std::cout << std::endl;}
clear_and_exit:
  wc_collector.clear();
  wc_collector.shrink_to_fit();
}

void Command::popDir()
{
  if (!m_dir_stack.size()) {
    std::cerr << "No directories left to pop!" << std::endl;
    return;
  }
  std::string dir = tilde_expand(m_dir_stack.front());
  if (changedir(dir)) {
    m_dir_stack.erase(m_dir_stack.begin(), m_dir_stack.begin() + 1);
  }
  for (auto && a : m_dir_stack) {
    std::cout << a << " ";
  }
  std::cout << std::endl;
}

void Command::send_to_foreground(
  ssize_t job_num,
  bool & fg,
  termios & _oldtermios)
{
  pid_t current = m_shell_pgid;
  if (m_p_jobs->size()) {
    /* did they pass an argument? */
    job_num = (job_num < 0) ? m_p_jobs->size() - 1 : job_num;
    job _job = m_p_jobs->at(job_num);
    m_p_jobs->erase(     /* remove the job */
      m_p_jobs->begin() + job_num,
      m_p_jobs->begin() + job_num + 1);
    tcsetpgrp(0, _job.pgid);
    tcsetattr(0, TCSADRAIN, &_oldtermios);
    if (kill(_job.pgid, SIGCONT) < 0) {perror("kill");}

    /* prep to save */
    int status = -1;     /* shut up, GCC */
    /* waitpid:
     *   pid <  -1 => wait for absolute value of pid
     *   pid == -1 => wait for any child process
     *   pid ==  0 => wait for any child whose pgid is ours
     *   pid >   0 => wait for the specified pid
     */

    if (!background) {
      /* put the job in the foreground */
      if (m_interactive) {
        tcsetpgrp(STDIN_FILENO, m_pgid);
        waitpid(_job.pgid, &status, WUNTRACED);
        tcsetpgrp(STDIN_FILENO, m_shell_pgid);
      } else {waitpid(_job.pgid, &status, WUNTRACED);}
    } else {m_p_jobs->push_back(_job), m_job_map[m_pgid] = m_p_jobs->size() - 1;}

    /* check if the job is stopped */
    if (WIFSTOPPED(status)) {
      _job.status = job_status::STOPPED;
      m_p_jobs->push_back(_job);
      std::cout << "[" << (m_job_map[_job.pgid] = m_p_jobs->size() - 1) <<
        "]+\tstopped\t" << _job.pgid << std::endl;
    }
  } else {
    std::cerr << "fg: no such job" << std::endl;
  } fg = false;
}

std::string get_command_text(Command & cmd)
{
  /* beggining of the string */
  std::string ret = "";
  bool first_cmd = true, first_arg = true;
  for (auto & x : cmd.simpleCommands) {
    ret += (first_cmd) ? (first_cmd = false, "") : " |";
    for (auto & y : x.get()->arguments) {
      if (y == nullptr) {
        continue;                       /* skip over the first one */
      }
      ret += ((first_arg) ? (first_arg = false, "") :
        std::string(" ")) + y;
    }
  }
  return ret;
}
