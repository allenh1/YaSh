%{#include <memory>%}
%{#include <memory>%}

%token	<string_val> WORD
%token  <string_val> BACKTIK

%token 	NOTOKEN GREAT NEWLINE GTFO LESS TOOGREAT TOOGREATAND PIPE AMPERSAND
%token GREATAND TAB SRC ANDAND
%union	{
    char * string_val;
}

%{
#include <stdio.h>
#include <dirent.h>
#include <fcntl.h>
#include <regex.h>
#include <cstring>
#include <iostream>
#include <algorithm>
#include "shell-readline.hpp"
    void yyerror(const char * s);
    int yylex();

    void wildcard_expand(char * prefix, char * suffix) {
	if (!*suffix && prefix && *prefix)
	    { Command::currentCommand.wc_collector.push_back(std::string(strdup(prefix))); return; }
	else if (!*suffix) return;
	// Get next slash (skipping first, if necessary)
	char * slash = strchr((*suffix == '/') ? suffix + 1: suffix, '/');
	char * next  = (char*) calloc(MAXLEN + 1, sizeof(char));
	// This line is magic.
	for (char * temp = next; *suffix && suffix != slash; *(temp++) = *(suffix++));

	// Expand the wildcard
	char * nextPrefix = (char*) calloc(MAXLEN + 1, sizeof(char));
	if (!(strchr(next, '*') || strchr(next, '?'))) {
	    // No more wildcards.
	    if (!(prefix && *prefix)) sprintf(nextPrefix, "%s", next);
	    else sprintf(nextPrefix, "%s%s", prefix, next);
	    wildcard_expand(nextPrefix, suffix);
	    free(nextPrefix); free(next); return;
	}

	// Wildcards were found!
	// Convert to regex.
	char * rgx = (char*) calloc(2 * strlen(next) + 3, sizeof(char));
	char * trx = rgx;

	*(trx++) = '^';
	for (char * tmp = next; *tmp; ++tmp) {
	    switch (*tmp) {
	    case '*':
		*(trx++) = '.';
		*(trx++) = '*';
		break;
	    case '?':
		*(trx++) = '.';
		break;
	    case '.':
		*(trx++) = '\\';
		*(trx++) = '.';
	    case '/':
		break;
	    default:
		*(trx++) = *tmp;
	    }
	} *(trx++) = '$'; *(trx++) = 0;
	// Compile regex.
	regex_t * p_rgx = new regex_t();
	if (regcomp(p_rgx, rgx, REG_EXTENDED|REG_NOSUB)) {
	    // Exit with error if regex failed to compile.
	    perror("regex (compile)");
	    exit(1);
	}

	char * _dir = (prefix) ? strdup(prefix) : strdup(".");

	DIR * dir = opendir(_dir); free(_dir);
	if (!dir) {
	    free(dir); free(rgx); free(next); free(nextPrefix); delete p_rgx;
	    return; // Return if opendir returned null.
	}

	dirent * _entry;
	for (; _entry = readdir(dir);) {
	    // Check for a match!
	    if (!regexec(p_rgx, _entry->d_name, 0, 0, 0)) {
		if (!(prefix && *prefix)) sprintf(nextPrefix, "%s", _entry->d_name);
		else sprintf(nextPrefix, "%s/%s", prefix, _entry->d_name);

		if (_entry->d_name[0] == '.' && *rgx == '.') wildcard_expand(nextPrefix, suffix);
		else if (_entry->d_name[0] == '.') continue;
		else wildcard_expand(nextPrefix, suffix);
	    }
	}

	// Be kind to malloc. Malloc is bae.
	free(next); free(nextPrefix); free(rgx); free(dir);
	// This one was allocated with new.
	delete p_rgx;
    }

  

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
| SRC WORD { reader.setMode(SOURCE); reader.setFile(std::string($2)); }
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
    wildcard_expand(0, $1);
    std::string * array = Command::currentCommand.wc_collector.data();
    std::sort(array, array + Command::currentCommand.wc_collector.size(), Comparator());

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

background_optional:
AMPERSAND {
    Command::currentCommand.setBackground(true);
}
| /** Accept empty **/
;
%%

void yyerror(const char * s)
{
    fprintf(stderr,"%s", s);
}

