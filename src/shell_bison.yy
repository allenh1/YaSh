%{#include <memory>%}

%token	<string_val> WORD
%token  <string_val> BACKTIK

%token 	NOTOKEN GREAT NEWLINE GTFO LESS TOOGREAT TOOGREATAND PIPE AMPERSAND
%token GREATAND TAB SRC ANDAND ALIAS GETS 
%union	{
    char * string_val;
}

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

void yyerror(const char * s);
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
goal:	
commands
;

commands: 
commands command 
| command
;

command:
simple_command
|
;

simple_command:	
pipe_list iomodifier_list background_optional NEWLINE {
    Command::currentCommand.execute();
}
| SRC WORD { reader.setFile(std::string($2)); }
| ALIAS WORD WORD {
	char * alias, * word, * equals;
	if (!(equals = strchr($2, '='))) {
		std::cerr<<"Invalid syntax: alias needs to be set!"<<std::endl;
	} else {	
		alias = strndup($2, strlen($2) - 1);
		/* word = WORD + length before '=' + 1 (for '='). */
		word  = strdup($3);

		Command::currentCommand.setAlias(alias, word);

		free(alias); free(word);
	}
}
| NEWLINE { Command::currentCommand.prompt(); }
| error NEWLINE { yyerrok; std::cout<<std::endl; Command::currentCommand.prompt(); }
;

command_and_args:
command_word argument_list {
    Command::currentCommand.insertSimpleCommand(Command::currentSimpleCommand);
}
;

argument_list:
argument_list argument
| /* can be empty */
;

argument:
WORD {
	std::string temp = tilde_expand(std::string($1));
	
	char * expand_upon_me = strndup(temp.c_str(), temp.size());
    wildcard_expand(expand_upon_me); free(expand_upon_me);
    std::string * array = Command::currentCommand.wc_collector.data();

    for (auto && arg : Command::currentCommand.wc_collector) {
	char * temp = strdup(arg.c_str());
	Command::currentSimpleCommand->insertArgument(temp);
	free(temp);
    }
    Command::currentCommand.wc_collector.clear();
    Command::currentCommand.wc_collector.shrink_to_fit();
}
| BACKTIK {
    Command::currentCommand.subShell($1);
  }
;

command_word:
WORD {
  Command::currentSimpleCommand = std::unique_ptr<SimpleCommand>(new SimpleCommand());
  char * _ptr = strdup($1);
  Command::currentSimpleCommand->insertArgument(_ptr);
  free(_ptr);
}
;

pipe_list:
pipe_list PIPE command_and_args
| command_and_args
;

iomodifier_list:
iomodifier_list iomodifier_opt
| iomodifier_opt
|
;

iomodifier_opt:
GREAT WORD {
    if (Command::currentCommand.outIsSet())
	yyerror("Ambiguous output redirect.\n");
    Command::currentCommand.setOutFile($2);
}
| TOOGREAT WORD {
    if (Command::currentCommand.outIsSet())
	yyerror("Ambiguous output redirect.\n");
    Command::currentCommand.setOutFile($2);
    Command::currentCommand.setAppend(true);
}
| GREATAND WORD {
    if (Command::currentCommand.outIsSet())
	yyerror("Ambiguous output redirect.\n");
    else if (Command::currentCommand.errIsSet())
	yyerror("Ambiguous error redirect.\n");
    Command::currentCommand.setErrFile($2);
    Command::currentCommand.setOutFile($2);
}
| TOOGREATAND WORD {
    if (Command::currentCommand.outIsSet())
	yyerror("Ambiguous output redirect.\n");
    else if (Command::currentCommand.errIsSet())
	yyerror("Ambiguous error redirect.\n");
    Command::currentCommand.setOutFile($2);
    Command::currentCommand.setErrFile($2);
    Command::currentCommand.setAppend(true);
}
| LESS WORD {
    if (Command::currentCommand.inIsSet())
	yyerror("Ambiguous input redirect.\n");
    Command::currentCommand.setInFile($2);
}
;

background_optional: AMPERSAND {
    Command::currentCommand.setBackground(true);
}
| /** Accept empty **/
;
%%

void yyerror(const char * s)
{ fprintf(stderr,"%s", s); }

