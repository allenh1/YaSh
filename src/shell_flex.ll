%{
#include <string.h>
#include "shell_bison.hh"
#include "shell-readline.hpp"
/******************* Input From read-shell ************************/
#include <unistd.h>
	static char * lastLine = NULL;
	read_line reader;

int mygetc (FILE * f) {
	static char * p = NULL;
	static int get_file = 0;
	
	char ch;
	
	if (!Command::currentCommand.is_interactive()) return getc(f);
	if (p == NULL || *p == 0) {
		if (lastLine != NULL) free(lastLine);
		if (get_file) {
			p = reader.getStashed();
			get_file = 0;
			lastLine = p;
		} else {
			reader.tty_raw_mode();
			reader();
			reader.unset_tty_raw_mode();
			p = reader.get();
			lastLine = p;
		}
	}
	ch = *p;
	++p;
	
	return ch;
}

#undef getc
#define getc(f) mygetc(f)

/******************* Ending of shell input  *************************/

static  void yyunput (int c,char *buf_ptr);

void myunputc(int c) {
    unput(c);
}

%}

%option noyywrap

%x LINE_COMMENT
%%

\n { return NEWLINE; }

[ ] 	{ /* Discard spaces */ }	

\t { /* tabs */ return TAB; }

<INITIAL>"#" BEGIN(LINE_COMMENT);
<LINE_COMMENT>[\n] BEGIN(INITIAL);
<LINE_COMMENT>[.]+ { /* Ignore anything in a line comment */ }

">" return GREAT;
">&" return GREATAND;
"<" return LESS;
">>" return TOOGREAT;
">>&" return TOOGREATAND;
"|" return PIPE;
"&" return AMPERSAND;

^"exit" {std::cout<<"Bye!"<<std::endl; exit(0); }

^"time" { return TIME; }
^"source" { return SRC; }
^"alias" { return ALIAS; }
^"popd" { return POPD; }

`(\\.|[^`"])*` {
  /* Subshell, by Drew Barthel */

  /* Remove backticks */
  char* string = strdup(yytext+1);
  string[strlen(string)-1]='\n';

  /* Set up temp string and pipes */
  char buffer[1024];
  int pipeOne[2], pipeTwo[2]; //Double the pipes? double the fun!
  if (pipe(pipeOne) == -1) {
    perror("subshell pipe");
    free(string);
    return 1;
  } else if (pipe(pipeTwo) == -1) {
    perror("subshell pipe");
    free(string);
    return 1;
  }
		
  /* Housekeeping shit */
  int tempIn  = dup(0);
  int tempOut = dup(1);
  dup2(pipeOne[1], 1); close(pipeOne[1]);
  dup2(pipeTwo[0], 0); close(pipeTwo[0]);
		
  // Get ready world.
  pid_t ret = fork(); //Deekeeds
  switch(ret) {
  case(-1): {
	perror("fork-subshell");
	exit(1);
  } case (0): {

	  //Housekeeping
	  dup2(pipeOne[0], 0);
	  dup2(pipeTwo[1], 1);
	  close(pipeOne[0]);
	  close(pipeTwo[1]);
				
	  // Set up for self call
	  char* args[2];
	  args[0] = strdup("/proc/self/exe"); //Call self
	  args[1] = NULL;

	  // Call self
	  execvp(args[0], args);

	  // You shouldn't be around these parts boy
	  perror("execvp-subshell");
	  exit(1);

	} default: {

		/* Write to pipe. */
		size_t length = strlen(string), i = 0;
		for(;i < length && write(1, string + i,1); i++);
				
		//Housekeeping
		dup2(tempOut, 1);
		close(tempOut);
		close(pipeTwo[1]);
				
		// Read from pipe.
		char* temp = buffer;
		char c = 0;
		while(read(0,&c,1)) {

		  if(c=='\n') *temp = ' ';
		  else *temp = c;
		  temp++;

		} temp--;
				
		/* Clear uneeded things */
		while(temp>=buffer){unput(*temp); temp--;}
				
		// Final housecleaning
		dup2(tempIn, 0);
		close(tempIn);
		break;
	  } 

	/* Wait for all processes */
  } waitpid(ret,NULL,0);

  free(string);
}


\"(\\.|[^"])*\" {
    yylval.string_val = new char[strlen(yytext) - 1];
    memset(yylval.string_val, 0, strlen(yytext) - 1);
    strncpy(yylval.string_val, yytext + 1, strlen(yytext + 1) - 1);
    return WORD;
}

[^ ^|\t\n>"][^ ^|\t\n>"]*  {
    yylval.string_val = new char[strlen(yytext) + 1];
    memset(yylval.string_val, 0, strlen(yytext) + 1);
    strcpy(yylval.string_val, yytext);
    return WORD;
}
