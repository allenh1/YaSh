%{#include <memory>%}
%token	<string_val> WORD
%token  <string_val> BACKTIK

%token 	NOTOKEN GREAT NEWLINE GTFO LESS TOOGREAT TOOGREATAND PIPE AMPERSAND
%token GREATAND TAB SRC ANDAND ALIAS GETS PUSHD POPD FG BG

%union	{
    char * string_val;
}

%left PIPE
%{
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
goal:			commands;

commands: 		command commands 
		| 		command
				;

command:
				pipe_list iomodifier_list background_optional NEWLINE {
					Command::currentCommand.execute();
				}
		| 		SRC WORD { reader.setFile(std::string($2)); delete[] $2; }
		|		FG {
          			pid_t current = tcgetpgrp(0);
			        job _back = Command::currentCommand.m_jobs.back();
					tcsetpgrp(0, _back.pgid);
			        tcsetattr(0, TCSADRAIN, &reader.oldtermios);
					if (kill(- _back.pgid, SIGCONT) < 0) perror("kill");

					waitpid(_back.pgid, 0, WUNTRACED);
					tcsetpgrp(0, current);
					tcgetattr(0, &reader.oldtermios);
					tcsetattr(0, TCSADRAIN, &reader.oldtermios);
				}	
		| 		ALIAS WORD WORD {
			       char * alias, * word, * equals;
				   if (!(equals = strchr($2, '='))) {
					   std::cerr<<"Invalid syntax: alias needs to be set!"<<std::endl;
				   } else {	
					   alias = strndup($2, strlen($2) - 1);
					   /* word = WORD + length before '=' + 1 (for '='). */
					   word  = strdup($3);

					   Command::currentCommand.setAlias(alias, word);

					   free(alias); free(word); delete[] $2; delete[] $3;
				   }
				}
		| 		PUSHD WORD {
			       Command::currentCommand.pushDir(strdup($2));
				   delete[] $2;
                }
        | PUSHD {
	  	std::cerr<<"pushd: no provided directory"<<std::endl;
	  }
	| POPD {
			       Command::currentCommand.popDir();
                }
		| 		NEWLINE { Command::currentCommand.prompt(); }
		| 		error NEWLINE { yyerrok; std::cout<<std::endl; Command::currentCommand.prompt(); };

command_and_args:
				command_word argument_list {
					Command::currentCommand.insertSimpleCommand(Command::currentSimpleCommand);
				};

argument_list: 	argument
		|		argument argument_list
		|		/*empty */;

argument:		WORD {
					std::string temp = tilde_expand(std::string($1));
					delete[] $1;
					char * expand_upon_me = strndup(temp.c_str(), temp.size());
					wildcard_expand(expand_upon_me); free(expand_upon_me);

					for (auto && arg : Command::currentCommand.wc_collector) {
						char * temp = strndup(arg.c_str(), arg.size());
						Command::currentSimpleCommand->insertArgument(temp);
						free(temp);
					}
				    Command::currentCommand.wc_collector.clear();
				    Command::currentCommand.wc_collector.shrink_to_fit();
				}
		| 		BACKTIK { Command::currentCommand.subShell($1); delete[] $1; };

command_word:	WORD {
					Command::currentSimpleCommand =
						std::unique_ptr<SimpleCommand>(new SimpleCommand());
					char * _ptr = strdup($1); delete[] $1;
					Command::currentSimpleCommand->insertArgument(_ptr);
					free(_ptr);
				};

pipe_list:		command_and_args PIPE pipe_list
		| 		command_and_args;

iomodifier_list:
			    iomodifier_opt iomodifier_list
		| 		iomodifier_opt;

iomodifier_opt:
				GREAT WORD {
					if (Command::currentCommand.outIsSet())
						yyerror("Ambiguous output redirect.\n");
					Command::currentCommand.set_out_file($2);
				}
		| 		TOOGREAT WORD {
			       if (Command::currentCommand.outIsSet())
					   yyerror("Ambiguous output redirect.\n");
				   Command::currentCommand.setAppend(true);
				   Command::currentCommand.set_out_file($2);
				}
		| 		GREATAND WORD {
			       if (Command::currentCommand.outIsSet())
					   yyerror("Ambiguous output redirect.\n");
				   else if (Command::currentCommand.errIsSet())
					   yyerror("Ambiguous error redirect.\n");
				   Command::currentCommand.set_out_file($2);
				   Command::currentCommand.set_err_file($2);
				}
		| 		TOOGREATAND WORD {
                   if (Command::currentCommand.outIsSet())
					   yyerror("Ambiguous output redirect.\n");
				   else if (Command::currentCommand.errIsSet())
					   yyerror("Ambiguous error redirect.\n");
				   Command::currentCommand.setAppend(true);
				   Command::currentCommand.set_out_file($2);
				   Command::currentCommand.set_err_file($2);
				}
		| 		LESS WORD {
                   if (Command::currentCommand.inIsSet())
					   yyerror("Ambiguous input redirect.\n");
				   Command::currentCommand.set_in_file($2);
				}
		| /* allow empty */;

background_optional:
				AMPERSAND { Command::currentCommand.setBackground(true); }
		|       /** Accept empty **/;
%%

void yyerror(const char * s)
{ fprintf(stderr,"%s", s); }

