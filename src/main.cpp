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
#include "shell-readline.hpp"
#include "shell_bison.hh"
#include "command.hpp"

extern FILE * yyin;
extern FILE * yyout;

extern int yylex();  
extern int yyparse();

extern void yyrestart (FILE * in);
extern void yyerror(const char * s);
extern read_line reader;
std::vector<std::string> * SimpleCommand::history = NULL;
std::vector<job> * SimpleCommand::p_jobs = NULL;

int main()
{
    bool is_interactive = false;  
    Command::currentCommand.set_interactive((is_interactive = isatty(0)));

    std::string expanded_home = tilde_expand("~/.yashrc");

    char * rcfile = strndup(expanded_home.c_str(), expanded_home.size());

    yyin = fopen(rcfile, "r"); free(rcfile);

    /* From Brian P. Hays */
    if (yyin != NULL) {
        Command::currentCommand.printPrompt = false;
        yyparse();
        fclose(yyin);

        yyin = stdin;
        yyrestart(yyin);
        Command::currentCommand.printPrompt = true;
    } 

    if (is_interactive) {
        /* loop until we are in the foreground */
        for (; tcgetpgrp(STDIN_FILENO) != (Command::currentCommand.m_pgid = getpgrp());) {
            if (kill(0, SIGTTIN) < 0) perror("kill");
        }
		
        /* go to our process group */
        pid_t shell_pgid = getpid();
        if ((shell_pgid != getpgrp()) && setpgid(0, shell_pgid) < 0) perror("setpgid");

        Command::currentCommand.m_shell_pgid = shell_pgid;
        /* Ignore interactive and job-control signals */
        signal(SIGINT,  SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);
        signal(SIGCHLD, SIG_IGN);

        tcsetpgrp(STDIN_FILENO, Command::currentCommand.m_shell_pgid);
        tcgetattr(STDIN_FILENO, &reader.oldtermios);
    }
  
    Command::currentCommand.prompt();  
    yyparse();

    return 0;
}

