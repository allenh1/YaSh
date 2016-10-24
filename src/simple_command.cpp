#include "command.hpp"

/**
 * Launch the process with the given
 * file descriptors
 */
void SimpleCommand::launch(int fdin, int fdout, int fderr,
						   int pgid, bool background,
						   bool interactive)
{
   pid_t pid;
   
   if (interactive) {
	  /* move the pid into the group, and control the terminal */
	  pid = getpid();

	  if (pgid == 0) pgid = pid;
	  setpgid(pid, pgid);

	  /* grab terminal control */
	  if (!background) tcsetpgrp(0, pgid);

	  /* set signals to default handlers */
	  signal(SIGINT,  SIG_DFL);
	  signal(SIGQUIT, SIG_DFL);
	  signal(SIGTSTP, SIG_DFL);
	  signal(SIGTTIN, SIG_DFL);
	  signal(SIGTTOU, SIG_DFL);
	  signal(SIGCHLD, SIG_DFL);	  
   } if (fdin != STDIN_FILENO) {
	  dup2(fdin, STDIN_FILENO);
	  close(fdin);
   } if (fdout != STDOUT_FILENO) {
	  dup2(fdout, STDOUT_FILENO);
	  close(fdout);
   } if (fderr != STDERR_FILENO) {
	  dup2(fderr, STDERR_FILENO);
	  close(fderr);	  
   }

   /* execute */
   execvp(arguments[0], arguments.data());
   perror("execvp");
   _exit(1);
}
