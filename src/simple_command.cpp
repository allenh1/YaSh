#include "simple_command.hpp"

/**
 * Launch the process with the given
 * file descriptors
 */
void SimpleCommand::launch(const int & fdin, const int & fdout,
						   const int & fderr, const int & pgid,
						   const bool & background,
						   const bool & interactive)
{
	pid_t pid, _pgid;
	
	if (interactive) {
		/* move the pid into the group, and control the terminal */
		pid = getpid();

		_pgid = (pgid == 0) ? pid : pgid;
		setpgid(pid, _pgid);

		/* grab terminal control */
		if (!background) tcsetpgrp(0, _pgid);

		/* set signals to default handlers */
		signal(SIGINT,  SIG_DFL);
		signal(SIGQUIT, SIG_DFL);
		signal(SIGTSTP, SIG_DFL);
		signal(SIGTTIN, SIG_DFL);
		signal(SIGTTOU, SIG_DFL);
		signal(SIGCHLD, SIG_DFL);	  
	} setup_process_io(fdin, fdout, fderr); 

	/* handle special commands */
	handle_modified_commands();
	/* execute */
	execvp(arguments[0], arguments.data());
	perror("execvp");
	_exit(1);
}

void SimpleCommand::setup_process_io(const int & fdin,
									 const int & fdout,
									 const int & fderr)
{
	if (fdin != STDIN_FILENO) {
		dup2(fdin, STDIN_FILENO);
		close(fdin);
	} if (fdout != STDOUT_FILENO) {
		dup2(fdout, STDOUT_FILENO);
		close(fdout);
	} if (fderr != STDERR_FILENO) {
		dup2(fderr, STDERR_FILENO);
		close(fderr);	  
	}
}

bool SimpleCommand::handle_builtins(const int & fdin,
									const int & fdout,
									const int & fderr)
{
	if (handle_cd(fdin, fdout, fderr)) return true;
	else if (handle_setenv(fdin, fdout, fderr)) return true;
	else if (handle_unsetenv(fdin, fdout, fderr)) return true;
	else if (handle_cl(fdin, fdout, fderr)) return true;
	return false;
}

void SimpleCommand::handle_modified_commands()
{
	handle_ls();
	handle_grep();
	handle_history();
	handle_printenv();
}

bool SimpleCommand::handle_cd(const int & fdin,
							  const int & fdout,
							  const int & fderr)
{
	if (arguments[0] == std::string("cd")) {
		setup_process_io(fdin, fdout, fderr);
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
				return true;
			}

			register size_t replace_len     = strlen(to_replace);
			register size_t replacement_len = strlen(replace_to);

			if (!(replace_len && replacement_len)) {
				std::cerr<<"Error: replacement cannot be empty!"<<std::endl;

				free(to_replace); free(replace_in); free(replace_to);
				return true;
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
				return true;
			}
		} else if (arguments.size() == 2) {
			std::string _empty = "";
			if (!changedir(_empty)) {
				perror("cd");
				return true;
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

void SimpleCommand::handle_ls()
{
	if (arguments[0] == std::string("ls")) {
		char ** temp = new char*[arguments.size() + 2];
		for (int y = 2; y < arguments.size(); ++y) {
			temp[y] = strdup(arguments[y - 1]);
		} // ... still better than managing myself!
		temp[0] = strdup("ls");
		temp[1] = strdup("--color=auto");
		temp[arguments.size()] = NULL;

		execvp (temp[0], temp);
		perror("execvp");
		exit(2);
	}
}

void SimpleCommand::handle_grep()
{
	if (arguments[0] == std::string("grep")) {
		char ** temp = new char*[arguments.size() + 1];
		for (int y = 0; y < arguments.size() - 1; ++y) {
			temp[y] = strdup(arguments[y]);
		}
		temp[arguments.size() - 1] = strdup("--color");
		temp[arguments.size()] = NULL;

		execvp (temp[0], temp);
		perror("execvp");
		exit(2);
	}
}

void SimpleCommand::handle_printenv()
{
	if (arguments[0] == std::string("printenv")) {	
		char ** _env = environ;
		for (; *_env; ++_env) std::cout<<*_env<<std::endl;
		_exit (0);
	}
}

bool SimpleCommand::handle_setenv(const int & fdin,
								  const int & fdout,
								  const int & fderr)
{
	/* arguments.size() == 4 because of the extra NULL */
	if (arguments[0] == std::string("setenv") && arguments.size() != 4) {
		std::cerr<<"setenv: invalid arguments"<<std::endl;
		std::cerr<<"usage: setenv [variable] [value]"<<std::endl;
		return true;
	} else if (arguments[0] == std::string("setenv")) {
		setup_process_io(fdin, fdout, fderr);
		char * temp = (char*) calloc(strlen(arguments[1]) + 1, sizeof(char));
		char * pemt = (char*) calloc(strlen(arguments[2]) + 2, sizeof(char));
		strcpy(temp, arguments[1]); strcpy(pemt, arguments[2]);
		int result = setenv(temp, pemt, 1);
		if (result) perror("setenv");
		free(temp); free(pemt);
		return true;
	} return false;
}

bool SimpleCommand::handle_unsetenv(const int & fdin,
									const int & fdout,
									const int & fderr)
{
	if (arguments[0] == std::string("unsetenv") && arguments.size() != 3) {
		std::cerr<<"unsetenv: invalid arguments"<<std::endl;
		std::cerr<<"usage: unsetenv [variable]"<<std::endl;
		return true;
	} else if (arguments[0] == std::string("unsetenv")) {
		setup_process_io(fdin, fdout, fderr);
		char * temp = (char*) calloc(strlen(arguments[1]) + 1, sizeof(char));
		strcpy(temp, arguments[1]);
		if (unsetenv(temp) == -1) perror("unsetenv");		
		free(temp); return true;
	} return false;
}

bool SimpleCommand::handle_cl(const int & fdin,
							  const int & fdout,
							  const int & fderr)
{
	if (arguments[0] == std::string("cl")) {
		setup_process_io(fdin, fdout, fderr);
		if (arguments.size() > 2) {
			char ** temp = new char*[arguments.size()+2];
			temp[0] = strdup("ls");
			temp[1] = strdup("--color=auto");
			temp[2] = 0;
			std::string dir = std::string(arguments[1]);
			changedir(dir);

			pid_t pid = fork();
			if (pid == 0) {
				execvp(temp[0],temp);
				perror("execlp");
				_exit(1);
			}
			waitpid(pid,0,0);
			for(int y = 0; y < 3; y++){
				free(temp[y]);
				temp[y] = NULL;
			} delete[] temp;

			return true;
		} else std::cerr<< "Usage: cl, no args given" << std::endl;	
	} return false;
}

bool SimpleCommand::handle_history()
{
	if (arguments[0] == std::string("history")) {
		if (arguments.size() > 2) {
			/* check for the --no-lineno option */
			if (arguments[1] == std::string("--no-lineno")) {
				for (const auto & x : *history) std::cout<<x;
			} else {
				std::cerr<<"history: invalid arguments"<<std::endl;
				std::cerr<<"usage: history [--no-lineno]"<<std::endl;
			}
		} else {
			/* get the length of the number, for formatting. */
			std::string line = std::to_string(history->size());
			unsigned short width = line.size() + 3; size_t x = 0;
			std::string num;
			for (const auto & _line : *history) {
				num = std::to_string(x++) + ": ";
				std::cout<<std::setw(width)<<num<<_line;
			}
		} exit(0);
	}
}
