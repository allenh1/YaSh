%{
#include <string.h>
#include "shell_bison.hh"

/******************* Input From read-shell ************************/
#include "shell-readline.hpp"
#include <unistd.h>
static char * lastLine = NULL;
// extern FILE * yyin;

int mygetc (FILE * f) {
static readLine reader;
static char * p = NULL;
char ch;

if (!isatty(0)) return getc(f);

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
%%

\n { return NEWLINE; }

[ ] 	{ /* Discard spaces */ }	

\t { /* tabs */ return TAB; }

">" return GREAT;

">&" return GREATAND;
                
"<" return LESS;

">>" return TOOGREAT;

">>&" return TOOGREATAND;

"|" return PIPE;

"&" return AMPERSAND;

"`" return BACKTIK;

"exit" {std::cout<<"Bye!"<<std::endl; exit(0); }

^"source" { return SRC; }

`(\\.|[^`])*` {
    yylval.string_val = (char*) calloc(strlen(yytext) - 1, sizeof(char));
    strncpy(yylval.string_val, yytext + 1, strlen(yytext + 1) - 1);
    return BACKTIK;
}

\"(\\.|[^"])*\" {
    yylval.string_val = (char*) calloc(strlen(yytext) - 1, sizeof(char));
    strncpy(yylval.string_val, yytext + 1, strlen(yytext + 1) - 1); 
    return WORD;
}

[^ ^|\t\n>]*[^ ^|\t\n>]*  {
    yylval.string_val = (char*) calloc(strlen(yytext) + 1, sizeof(char));
    strcpy(yylval.string_val, yytext);
    return WORD;
}

.  {
    return NOTOKEN;
}

