%{
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

#include <string.h>
#include "shell_bison.hh"
#include "shell-readline.hpp"
/******************* Input From read-shell ************************/
#include <unistd.h>
	static char * lastLine = nullptr;
	read_line reader;

int mygetc (FILE * f) {
	static char * p = nullptr;
	static int get_file = 0;
	char ch;
	if (!Command::currentCommand.is_interactive()) return getc(f);
	if (p == nullptr || *p == 0) {
		if (lastLine != nullptr) free(lastLine);
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

/* Utils copied from esh for handling quoted strings in fancy flex mode stuff */
char* strbuf = nullptr;
int stringIndex = 0;
int strbufSize = 0;
#define DEFAULT_STRBUF_SIZE 256
void strbufWrite(char c) {
	//printf("strbufWrite(%c)\n",c);
	if( stringIndex >= strbufSize ) {
		strbuf = (char*)realloc(strbuf,sizeof(char)*(strbufSize*=2));
	}
	strbuf[stringIndex++] = c;
}
/* end fancy string utils */

%}

%option noyywrap

%x QUOTED_STRING
%x LINE_COMMENT
%%

\n { return NEWLINE; }

[ ] 	{ /* Discard spaces */ }

\t { /* tabs */ return TAB; }

<INITIAL>"#" BEGIN(LINE_COMMENT);
<LINE_COMMENT>[\n] BEGIN(INITIAL);
<LINE_COMMENT>[.]+ { /* Ignore anything in a line comment */ }

"&&" return ANDAND;
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
	  args[1] = nullptr;

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
  } waitpid(ret,nullptr,0);

  free(string);
}

\"	{
	BEGIN QUOTED_STRING;
	stringIndex = 0;
	Command::currentCommand.set_expand(false);
	strbuf = new char[(strbufSize = DEFAULT_STRBUF_SIZE)];
}

<QUOTED_STRING>\\n {
	strbufWrite('\n');
}
<QUOTED_STRING>\\t {
	strbufWrite('\t');
}
<QUOTED_STRING>\\\" {
	strbufWrite('\"');
}
<QUOTED_STRING>\" {
	strbufWrite('\0');
	yylval.string_val = strbuf;
	BEGIN 0;
	return WORD;
}
<QUOTED_STRING>\n {
	std::cout<<"> "<<std::flush;
	strbufWrite('\n');
}
<QUOTED_STRING>. {
	strbufWrite(*yytext);
}

(([^ \"\t\n\|\>\<\&\[\]])|(\\.))+ {
	yylval.string_val = new char[strlen(yytext) + 1];
	strcpy(yylval.string_val, yytext);
	size_t strs_len = strlen(yylval.string_val);
	for( int i = 0; i < strs_len; i++ ) {
		if( yylval.string_val[i] == '\\' ) {
			for( int j = i; j < strs_len; j++ ) {
				yylval.string_val[j] = yylval.string_val[j+1];
			}
		}
	}
	return WORD;
}
