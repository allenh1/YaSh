#include "shell_bison.hh"
#include "command.hpp"

extern "C" FILE * yyin;
extern "C" FILE * yyout;

extern int yylex();  
extern int yyparse();

extern void yyrestart (FILE * in);
extern void yyerror(const char * s);

int main()
{
  bool is_interactive = false;
  
  Command::currentCommand.set_interactive((is_interactive = isatty(0)));

  // std::string expanded_home = tilde_expand("~/.yashrc");

  // char * rcfile = strndup(expanded_home.c_str(), expanded_home.size());

  // yyin = fopen(rcfile, "r"); free(rcfile);

  // /* From Brian P. Hays */
  // if (yyin != NULL) {
  // 	Command::currentCommand.printPrompt = false;
  // 	yyparse();
  // 	fclose(yyin);

  // 	yyin = stdin;
  // 	yyrestart(yyin);
  // 	Command::currentCommand.printPrompt = true;
  // }

  // std::cerr<<"source exited"<<std::endl;

  if (is_interactive) {
	/* loop until we are in the foreground */
	for (; tcgetpgrp(STDIN_FILENO) != (Command::currentCommand.m_pgid = getpgrp());) {
	  kill(- Command::currentCommand.m_pgid, SIGTTIN);
	}

	/* Ignore interactive and job-control signals */
	signal(SIGINT,  SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);

	/* go to our process group */
	Command::currentCommand.m_pgid = getpid();
	if (setpgid(Command::currentCommand.m_pgid,
				Command::currentCommand.m_pgid) < 0) {
	  perror("setpgid");
	  _exit(1);
	} /* read_line will grab the terminal */	
  }
  
  Command::currentCommand.prompt();  
  yyparse();

  return 0;
}
