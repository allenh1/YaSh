
%{#include <memory>%}
%token  <string_val> WORD
%token	<string_val> BACKTIK

%token 	NOTOKEN GREAT NEWLINE GTFO LESS TOOGREAT TOOGREATAND PIPE AMPERSAND
%token GREATAND TAB SRC ANDAND ALIAS GETS POPD TIME

%union	{
    char   *string_val;
}

%{
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
/* STL (C++) Includes */
#include <algorithm>
#include <iostream>
#include <cstring>

/* C Includes */
#include <dirent.h>
#include <stdio.h>
#include <fcntl.h>
#include <regex.h>

/* File Includes */
#include "shell-readline.hpp"
#include "shell-utils.hpp"
#include "wildcard.hpp"

int yylex();
int yyparse();

void yyrestart (FILE * in);
void yyerror(const char * s);

extern read_line reader;

bool fg=false, bg=false, pushd=false;
%}

%%

 /**
  * Lex:
  * <reg. exp> <whitespace> <action>
  *
  * <reg. exp>: beginning of line up to first non-escaped white space.
  *
  * <action>: A single C statement. Multiple statements are wrapped in braces.
  */

goal:	commands
;

commands:
commands command
| command
;

command:
full_command { Command::currentCommand.execute(); }
| POPD { Command::currentCommand.popDir(); }
| SRC WORD { reader.setFile(std::string($2)); delete[] $2; }
| ALIAS WORD WORD {
	std::shared_ptr<char> alias = nullptr, word = nullptr, equals = nullptr;
	if (!(equals = std::shared_ptr<char>(strdup(strchr($2, '=')), free))) {
		std::cerr<<"Invalid syntax: alias needs to be set!"<<std::endl;
	} else {
		alias = std::shared_ptr<char>(strndup($2, strlen($2) - 1), free);
		/* word = WORD + length before '=' + 1 (for '='). */
		word = std::shared_ptr<char>(strdup($3), free);
		Command::currentCommand.setAlias(alias, word);
	} delete[] $2; delete[] $3;
}
| TIME NEWLINE { Command::currentCommand.prompt(); }
| NEWLINE { Command::currentCommand.prompt(); }
| error NEWLINE { yyerrok; std::cout<<std::endl; Command::currentCommand.prompt(); }
;

cmd_and_arg:
command_word argument_list {
	Command::currentCommand.insertSimpleCommand(Command::currentSimpleCommand);
}
| command_word {
	if(pushd) {
		std::cerr<<"pushd: no provided directory"<<std::endl; pushd=false;
	} else if(fg) {
	    Command::currentCommand.send_to_foreground(-1, fg, reader.oldtermios);
	} else if(bg) {
		job _back = Command::currentCommand.m_p_jobs->back();
		/* don't restore io, just resume. */
		if (kill(_back.pgid, SIGCONT) < 0) perror("kill"); bg=false;
	} else {
		Command::currentCommand.insertSimpleCommand(Command::currentSimpleCommand);
        }
}
;

command_word:
WORD {
        Command::currentSimpleCommand = std::make_unique<SimpleCommand>();
        auto _ptr = std::shared_ptr<char>(strdup($1), free);
        if(!strcmp($1, "fg")) fg=true;
        else if(!strcmp($1, "bg")) bg=true;
	else if(!strcmp($1, "pushd")) pushd=true;
        else Command::currentSimpleCommand->insertArgument(_ptr);
        delete[] $1;
}
;

argument_list:
argument_list argument
| argument
;

argument:
WORD {
	std::string temp = tilde_expand(std::string($1));
	auto expand_upon_me = std::shared_ptr<char>(strndup(temp.c_str(), temp.size()), free);
	wildcard_expand(expand_upon_me);

	if(fg) {
		try {
			size_t as_num = static_cast<size_t>(std::stoi(std::string($1)));
			if (as_num >= Command::currentCommand.m_p_jobs->size()) {
				std::cerr<<"fg: no such job"<<std::endl;
			} else {
			  	Command::currentCommand.send_to_foreground(
				   as_num, fg, reader.oldtermios);
		    }
		} catch ( ... ) {
			std::cerr<<"fg: \""<<$1<<"\" is not a number."
					 <<std::endl;
		} fg=false;
	} else if(bg) {
		try {
			size_t as_num = static_cast<size_t>(std::stoi(std::string($1)));
			if (as_num >= Command::currentCommand.m_p_jobs->size()) {
				job _back = Command::currentCommand.m_p_jobs->at(as_num);
				/* erase */
				Command::currentCommand.m_p_jobs->erase(
					Command::currentCommand.m_p_jobs->begin() + as_num,
					Command::currentCommand.m_p_jobs->begin() + as_num + 1);

                /* don't restore io, just resume */
				if (kill(_back.pgid, SIGCONT) < 0) perror("kill");
			}
		} catch ( ... ) {
			std::cerr<<"bg: \""<<$1<<"\" is not a number."
					 <<std::endl;
		} bg=false;
	} else if(pushd) {
		Command::currentCommand.pushDir(std::shared_ptr<char>($1)); pushd=false;
                $1 = nullptr;
	} else {
		for (auto && arg : Command::currentCommand.wc_collector) {
			std::shared_ptr<char> temp = std::shared_ptr<char>(
                        strndup(arg.c_str(), arg.size()), free);
			Command::currentSimpleCommand->insertArgument(temp);
		}
		Command::currentCommand.wc_collector.clear();
		Command::currentCommand.wc_collector.shrink_to_fit();
	} delete[] $1;
}
;

io_modifier:
GREAT WORD {
	if (Command::currentCommand.outIsSet())
		yyerror("Ambiguous output redirect.\n");
	Command::currentCommand.set_out_file(
            std::shared_ptr<char>($2, [] (auto s) { delete[] s; }));
}
| TOOGREAT WORD {
	if (Command::currentCommand.outIsSet())
		yyerror("Ambiguous output redirect.\n");
	Command::currentCommand.setAppend(true);
	Command::currentCommand.set_out_file(
            std::shared_ptr<char>($2, [] (auto s) { delete[] s; }));
}
| GREATAND WORD {
	if (Command::currentCommand.outIsSet())
		yyerror("Ambiguous output redirect.\n");
	else if (Command::currentCommand.errIsSet())
		yyerror("Ambiguous error redirect.\n");
	Command::currentCommand.set_err_file(std::shared_ptr<char>(strdup($2), free));
        Command::currentCommand.set_out_file(
            std::shared_ptr<char>($2, [] (auto s) { delete[] s; }));
}
| TOOGREATAND WORD {
	if (Command::currentCommand.outIsSet())
		yyerror("Ambiguous output redirect.\n");
	else if (Command::currentCommand.errIsSet())
		yyerror("Ambiguous error redirect.\n");
	Command::currentCommand.setAppend(true);
	Command::currentCommand.set_err_file(std::shared_ptr<char>(strdup($2), free));
        Command::currentCommand.set_out_file(
            std::shared_ptr<char>($2, [] (auto s) { delete[] s; }));
}
| LESS WORD {
	if (Command::currentCommand.inIsSet())
		yyerror("Ambiguous input redirect.\n");
	Command::currentCommand.set_in_file(
            std::shared_ptr<char>($2, [] (auto s) { delete[] s; }));
}
;

io_modifier_list:
io_modifier_list io_modifier
|	io_modifier
;

pipe_list:
pipe_list PIPE cmd_and_arg
|	cmd_and_arg
;

background:
AMPERSAND { Command::currentCommand.setBackground(true); }
;

time:
TIME { Command::currentCommand.set_time(true); }
;

command_line:
pipe_list io_modifier_list background
|	time pipe_list io_modifier_list
|	pipe_list io_modifier_list
|	pipe_list background
|	time pipe_list
|	pipe_list
;

full_command:
command_line {
     Command::currentCommand.printPrompt = false;
     Command::currentCommand.execute();
     Command::currentCommand.printPrompt = true;
   } ANDAND full_command
|  command_line NEWLINE
|  NEWLINE

%%

void yyerror(const char * s)
{ fprintf(stderr,"%s", s); }
