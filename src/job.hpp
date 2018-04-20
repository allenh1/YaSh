// Copyright 2016 Hunter L. Allen
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
#ifndef JOB_HPP_
#define JOB_HPP_
#include <sys/types.h>
#include <sys/wait.h>

#include <string>

enum job_status
{
  STOPPED,       /* job got SIGTSTP */
  RUNNING,       /* job is running  */
  EXITED         /* job exited normally */
};

struct job
{
  pid_t pgid;

  int m_stdin;
  int m_stdout;
  int m_stderr;

  std::string command;

  /* @todo do we still need this? */
  void restore_io()
  {
    dup2(m_stdin, 0); close(m_stdin);
    dup2(m_stdout, 1); close(m_stdout);
    dup2(m_stderr, 2); close(m_stderr);
  }

  job_status status;
};
#endif  // JOB_HPP_
