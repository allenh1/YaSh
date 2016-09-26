#include "shell_bison.hh"
#include "command.hpp"

extern "C" FILE * yyin;
extern "C" FILE * yyout;
extern void yyrestart (FILE * in);

int main()
{
  struct sigaction ctrl_action;
  ctrl_action.sa_handler = ctrlc_handler;
  sigemptyset(&ctrl_action.sa_mask);
  ctrl_action.sa_flags = SA_RESTART;

  if (sigaction(SIGINT, &ctrl_action, NULL)) {
	perror("sigint");
	_exit(6);
  }

  struct sigaction chld;
  chld.sa_handler = sigchld_handler;
  sigemptyset(&chld.sa_mask);
  chld.sa_flags   = SA_RESTART | SA_NOCLDSTOP;

  if (sigaction(SIGCHLD, &chld, NULL) == -1) {
	perror("sigchild");
	_exit(7);
  }

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
  
  Command::currentCommand.prompt();
  yyparse();

  return 0;
}
