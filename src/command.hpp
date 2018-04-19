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
#ifndef COMMAND_HPP_
#define COMMAND_HPP_
#pragma once
#include <memory>
#include <string>
#include <vector>
#include <map>

/* File Includes */
#include "simple_command.hpp"
#include "job.hpp"

/* external structs */
struct SimpleCommand;
struct job;

/* job status enum */
enum job_status;

class Command
{
public:
  Command();

  void prompt();
  void print();
  void execute();
  void clear();

  void pushDir(const std::shared_ptr<char> new_dir);
  void popDir();

  void insertSimpleCommand(std::shared_ptr<SimpleCommand> simpleCommand);

  void set_in_file(const std::shared_ptr<char> _fd);
  void set_out_file(const std::shared_ptr<char> _fd);
  void set_err_file(const std::shared_ptr<char> _fd);

  void send_to_foreground(
    ssize_t process_num,
    bool & fg,
    termios & _oldtermios);

  void setAlias(const std::shared_ptr<char> _from, const std::shared_ptr<char> _to);

  int status = -1;

  const bool & inIsSet() {return inSet;}
  const bool & outIsSet() {return outSet;}
  const bool & errIsSet() {return errSet;}
  const bool & is_interactive() {return m_interactive;}
  const bool & get_expand() {return m_expand;}

  void setAppend(const bool & ap) {append = ap;}
  void setBackground(const bool & bg) {background = bg;}

  static Command currentCommand;
  static std::shared_ptr<SimpleCommand> currentSimpleCommand;

  const int & get_stdin() {return m_stdin;}
  const int & get_stdout() {return m_stdout;}
  const int & get_stderr() {return m_stderr;}

  void set_interactive(const bool & _interactive)
  {
    m_interactive = _interactive;
  }

  void set_time(const bool & _time)
  {
    m_time = _time;
  }

  void set_expand(const bool & _expand)
  {
    m_expand = _expand;
  }

  void print_jobs();

  pid_t m_shell_pgid = 0;
  pid_t m_pgid = 0;
  pid_t m_pid = 0;

  std::map<std::string, std::vector<std::string>> m_aliases;
  std::map<pid_t, size_t> m_job_map;
  std::shared_ptr<std::vector<job>> m_p_jobs;

  std::vector<std::string> wc_collector;  // Wild card collection tool

  bool printPrompt = true;

  /**
   * Returns a string for the command the user ran
   */
  friend std::string get_command_text(Command & cmd);

private:
  std::vector<std::string> string_split(std::string s, char delim)
  {
    std::vector<std::string> elems; std::stringstream ss(s);
    for (std::string item; std::getline(ss, item, delim); elems.push_back(item)) {}
    return elems;
  }

  int get_output_flags();

  std::unique_ptr<std::string> outFile = nullptr;
  std::unique_ptr<std::string> inFile = nullptr;
  std::unique_ptr<std::string> errFile = nullptr;

  bool append = false;
  bool background = false;
  bool m_time = false;
  bool m_interactive = false;
  bool m_expand = true;
  bool stopped = false;
  bool completed = false;

  int numOfSimpleCommands = 0;

  bool inSet = false; int m_stdin = 0;
  bool outSet = false; int m_stdout = 1;
  bool errSet = false; int m_stderr = 2;

  std::vector<std::string> m_dir_stack;
  std::vector<std::shared_ptr<SimpleCommand>> simpleCommands;
};

/**
 * Returns a string for the command the user ran
 */
std::string get_command_text(Command & cmd);
#endif  // COMMAND_HPP_

