#include "shell_bison.hh"
#include "command.hpp"

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
  Command::currentCommand.readShellRC();
  Command::currentCommand.prompt();
  yyparse();

  return 0;
}
