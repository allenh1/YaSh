#include "simple_command.hpp"

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

bool SimpleCommand::handle_builtins()
{
	if (handle_cd()) return true;
	return false;
}

bool SimpleCommand::handle_cd()
{
	if (arguments[0] == std::string("cd")) {
		std::string curr_dir = std::string(getenv("PWD"));
		int cd; std::string new_dir;

		if (arguments.size() == 4) {
			char * to_replace = strdup(arguments[1]);
			char * replace_to = strdup(arguments[2]);		
			char * replace_in = strndup(curr_dir.c_str(), curr_dir.size());
		
			char * sub = strstr(replace_in, to_replace);

			/* Desired replacement wasn't found, so error and exit */
			if (sub == NULL) {
				perror("cd");

				free(to_replace);
				free(replace_to);
				free(replace_in);
				return false;
			}

			register size_t replace_len     = strlen(to_replace);
			register size_t replacement_len = strlen(replace_to);

			if (!(replace_len && replacement_len)) {
				std::cerr<<"Error: replacement cannot be empty!"<<std::endl;

				free(to_replace); free(replace_in); free(replace_to);
				return false;
			}
			/* garauntee we have enough space */
			char * replacement = (char*) calloc(curr_dir.size() -
												replace_len +
												replacement_len + 1,
												sizeof(char));
			char * d = replacement;
			/* copy up to the beginning of the substring */
			for (char * c = replace_in; c != sub; *(d++) = *(c++));
			/* copy the replacement */
			for (char * c = replace_to; *c; *(d++) = *(c++));
			/* advance past the substring we wanted to replace */
			char * residual = sub + replace_len;
			/* copy the residual chars over */
			for (; *residual; *(d++) = *(residual++));
		
			std::cout<<replacement<<std::endl;
		
			free(to_replace); free(replace_to); free(replace_in);

			std::string new_dir(replacement); free(replacement);
			if (!changedir(new_dir)) {
				perror("cd");		 
				return false;
			}
		} else if (arguments.size() == 2) {
			std::string _empty = "";
			if (!changedir(_empty)) {
				perror("cd");
				return false;
			}		
			setenv("PWD", getenv("HOME"), 1);
		} else {
			if (arguments[1] == std::string("pwd") ||
				arguments[1] == std::string("/bin/pwd")) {
			} else if (*arguments[1] != '/') { 
				new_dir = std::string(getenv("PWD"));
				for (;new_dir.back() == '/'; new_dir.pop_back());
				new_dir += "/" + std::string(arguments[1]);
			} else new_dir = std::string(arguments[1]);

			for (; *new_dir.c_str() != '/' && new_dir.back() == '/'; new_dir.pop_back());
			if (changedir(new_dir)) setenv("PWD", new_dir.c_str(), 1);
		}
		setenv("PWD", curr_dir.c_str(), 1);
		// Regardless of errors, cd has finished.
		return true;
	}
	return false;
}
