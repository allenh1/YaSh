#include "command.hpp"

std::vector<int> m_background;

SimpleCommand::SimpleCommand() { /** Used to call malloc for five stuffs here **/ }

void SimpleCommand::insertArgument(char * argument)
{
	std::string as_string(argument);

	auto exists = Command::currentCommand.m_aliases.find(as_string);

	/* exists == m_aliases.end() => we are inserting an alias */
	if (exists != Command::currentCommand.m_aliases.end()) {
		auto to_insert = exists->second;
		for (auto && str : to_insert) {
			char * toPush = strndup(str.c_str(), str.size());
			arguments.push_back(toPush); ++numOfArguments;
		}
	} else {
		std::string arga = tilde_expand(as_string);
		std::string arg  = env_expand(arga);

		char * str = strndup(arg.c_str(), arg.size());
		int index; char * t_str = str;
		char * temp = (char*) calloc(arg.size() + 1, sizeof(char));

		for (index = 0; *str; ++str) {
			if (*str == '\\' && *(str + 1) == '\"') {
				temp[index++] = '\"'; ++str;
			} else if (*str == '\\' && *(str + 1) == '&') {
				temp[index++] = '&'; ++str;
			} else if (*str == '\\' && *(str + 1) == '#') {
				temp[index++] = '#'; ++str;
			} else if (*str == '\\' && *(str + 1) == '<') {
				temp[index++] = '<'; ++str;
			} else if (*str == '\\' && *(str + 1) == '\\' && *(str+2) == '\\') {
				temp[index++] = '\\'; ++str; ++str;
			} else if (*str == '\\' && *(str + 1) == '\\'){
				temp[index++] = '\\'; ++str;
			} else if (*str == '\\' && *(str + 1) == ' ') {
				temp[index++] = ' '; ++str;
			} else {
				temp[index++] = *str;
			}
		} free(t_str);
		char * toPush = new char[index + 1]; memset(toPush, 0, index + 1);
		strcpy(toPush, temp);
		arguments.push_back(toPush), ++numOfArguments;
		free(temp);
	}
}

inline int eval_to_buffer(char * const* cmd, char * outBuff, size_t buffSize)
{
	int fdpipe[2]; int pid = -1; size_t x = 0;

	if (pipe(fdpipe) < 0) return -1;
	else if ((pid = fork()) < 0) return -1;
	else if (pid == 0) {
		/** Child Process: write into the pipe **/
		close(fdpipe[0]);   // close unused read end
		dup2(fdpipe[1], 1); // stdout to pipe
		dup2(fdpipe[1], 2); // stderr to pipe
		close(fdpipe[1]);

		if (execvp(cmd[0], cmd)) return -1;
		_exit(0);
	} else {
		/** Parent Process: read from the pipe **/
		close(fdpipe[1]);   // close unused write end
		for (memset(outBuff, 0, buffSize);x = read(fdpipe[0], outBuff, buffSize););
		if (x == buffSize) return -1;
		waitpid(pid, NULL, 0);
	} return 0;
}

int Command::get_output_flags()
{
	return O_CREAT | O_WRONLY | ((append) ? O_APPEND : O_TRUNC);
}

void Command::set_in_file(char * _fd) {
	inFile = std::unique_ptr<char>(_fd);
	inSet = true;

	m_stdin = open(_fd, O_RDONLY, 0600);

	if (m_stdin < 0) {
		perror("open");
		inSet = false;
		inFile = NULL;
		m_stdin = 0;
	}
}

void Command::set_out_file(char * _fd) {
	outFile = std::unique_ptr<char>(_fd);
	outSet = true;

	m_stdout = open(_fd, get_output_flags(), 0600);

	if (m_stdout < 0) {
		perror("open");
		outSet = false;
		outFile = NULL;
		m_stdout = 1;
	}
}

void Command::set_err_file(char * _fd) {
	errFile = std::unique_ptr<char>(_fd);
	errSet = true;

	m_stderr = open(_fd, get_output_flags(), 0600);

	if (m_stderr < 0) {
		perror("open");
		errSet = false;
		errFile = NULL;
		m_stderr = 2;
	}
}

#define SUBSH_MAX_LEN 4096
void Command::subShell(char * arg)
{
	std::cerr<<"Running subshell cmd: \""<<arg<<"\""<<std::endl;
	int cmd_pipe[2]; int out_pipe[2]; pid_t pid;
	int tmpin = dup(0); int tmpout = dup(1); int tmperr = dup(2);
  
	if (pipe(cmd_pipe) == -1) {
		perror("cmd_pipe");
		return;
	} else if (pipe(out_pipe) == -1) {
		perror("out_pipe");
		return;
	}

	dup2(cmd_pipe[1], 1); close(cmd_pipe[1]); /* cmd to stdout */
	dup2(out_pipe[0], 0); close(out_pipe[0]); /* out to stdin  */
  
	if ((pid = fork()) == -1) {
		perror("subshell fork");
		return;
	} else if (pid == 0) {
		/* Child Process */
		close(out_pipe[0]); /* close the read end of the out pipe */
		close(cmd_pipe[1]); /* close the write end of the cmd pipe */

		dup2(out_pipe[1], 1); close(out_pipe[1]); /* out_pipe[1] -> stdout */
		dup2(cmd_pipe[0], 0); close(cmd_pipe[0]); /* cmd_pipe[0] -> stdin  */

		execlp("yash", "yash", NULL);
		perror("subshell exec");
		_exit(1);
	} else if (pid != 0) {
		/* Parent Process */
		char * buff = (char*) calloc(SUBSH_MAX_LEN, sizeof(char));
		char * c = NULL;
	
		close(out_pipe[1]); /* close the write end of the out pipe */
		close(cmd_pipe[0]); /* close the read end of the cmd pipe */

		/* write the command to the write end of the cmd pipe */
		for (c = arg; *c && write(cmd_pipe[1], c++, 1););

		/* Close pipe so subprocess isn't waiting */
		dup2(tmpout, 1); close(tmpout); close(cmd_pipe[1]);
	
		waitpid(pid, NULL, WNOHANG); /* Don't hang if child is already dead */

		//std::cerr<<"Child rage quit"<<std::endl;
	
		/* read from the out pipe and store in a buffer */
		for (c = buff; read(out_pipe[0], c++, 1););

		std::cerr<<"Read from buffer"<<std::endl;
		size_t buff_len = c - buff; /* this is the number of characters read */

		/* Push the buffer onto stdin */
		for (int b = 0; (ungetc(buff[b++], stdin)) && buff_len--;);
		free(buff); /* release the buffer */
	}

	/* restore default IO */
	dup2(tmpin, 0); dup2(tmpout, 1); dup2(tmperr, 2);
}

Command::Command()
{ /** Constructor **/ }

void Command::insertSimpleCommand(std::shared_ptr<SimpleCommand> simpleCommand)
{ simpleCommands.push_back(simpleCommand), ++numOfSimpleCommands; }

void Command::clear()
{
   if (m_stdin  != 0) close(m_stdin);
   if (m_stdout != 1) close(m_stdout);
   if (m_stderr != 2) close(m_stderr);

   m_stdin = 0, m_stdout = 1, m_stderr = 2;
   
	simpleCommands.clear(),
		background = append = false,
		numOfSimpleCommands = 0,
		outFile.release(), inFile.release(),
		errFile.release(), simpleCommands.shrink_to_fit();
	outSet = inSet = errSet = false;
}

void Command::print()
{
	std::cout<<std::endl<<std::endl;
	std::cout<<"              COMMAND TABLE                "<<std::endl;  
	std::cout<<std::endl; 
	std::cout<<"  #   Simple Commands"<<std::endl;
	std::cout<<"  --- ----------------------------------------------------------"<<std::endl;

	for (int i = 0; i < numOfSimpleCommands; i++) {
		printf("  %-3d ", i);
		for (int j = 0; j < simpleCommands[i]->numOfArguments; j++) {
			std::cout<<"\""<< simpleCommands[i]->arguments[j] <<"\" \t";
		}
	}

	std::cout<<std::endl<<std::endl;
	std::cout<<"  Output       Input        Error        Background"<<std::endl;
	std::cout<<"  ------------ ------------ ------------ ------------"<<std::endl;
	printf("  %-12s %-12s %-12s %-12s\n", outFile.get()?outFile.get():"default",
		   inFile.get()?inFile.get():"default", errFile.get()?errFile.get():"default",
		   background?"YES":"NO");
	std::cout<<std::endl<<std::endl;
}

void Command::execute()
{
	int fdpipe[2], fdin, fdout, fderr;
	pid_t pid = 0;

	/* check for dank memes */
	char * dbg = getenv("SHELL_DBG");
	if (dbg && !strcmp(dbg, "YES")) print();

	char * lolz = getenv("LOLZ");
	if (lolz && !strcmp(lolz, "YES")) {
		/// Because why not?
		std::shared_ptr<SimpleCommand> lul(new SimpleCommand());
		char * _ptr = strdup("lolcat");
		lul->insertArgument(_ptr);
		free(_ptr);
		if (strcmp(simpleCommands.back().get()->arguments[0], "cd") &&
			strcmp(simpleCommands.back().get()->arguments[0], "clear") &&
			strcmp(simpleCommands.back().get()->arguments[0], "ssh") &&
			strcmp(simpleCommands.back().get()->arguments[0], "setenv") &&
			strcmp(simpleCommands.back().get()->arguments[0], "unsetenv")) {
			this->insertSimpleCommand(lul);
		}
	}

	/* point fdin (fderr) to this pipeline's input (error) */
	fdin = m_stdin; fderr = m_stderr;
	for (int x = 0; x < numOfSimpleCommands; ++x) {
		/* manage commands */
		std::vector<char *> curr = simpleCommands.at(x).get()->arguments;		
		char ** d_args;
		curr.push_back((char *) NULL);
		d_args = curr.data();

		/* add NULL to the end of the simple command (for exec) */
		simpleCommands.at(x).get()->arguments.push_back((char*) NULL);

		if (x != numOfSimpleCommands - 1) {
			/* thank you Gustavo for the outer if statement. */
			if (pipe(fdpipe)) {
				/* pipe failed */
				perror("pipe");
				clear(); prompt();
				return;
			}
		}

		/* redirect output */
		if (x != numOfSimpleCommands - 1) fdout = fdpipe[1];
		else fdout = m_stdout;

		if (simpleCommands.at(x).get()->handle_builtins(fdin,
														fdout,
														fderr)) {
		   goto cleanup;
		} else if ((pid = fork()) < 0) {
			/* fork failed */
			perror("fork"); clear();
			prompt(); return;
		} else if (pid == 0) {
			/* child process: exec into the process group */
			simpleCommands.at(x).get()->launch(fdin, fdout, fderr,
											   m_pgid, background,
											   m_interactive);			
		} else {
			/* parent process */
			simpleCommands.at(x).get()->pid = pid;
			if (m_interactive) {
				if (!m_pgid) m_pgid = pid;
				/* add pid to the pipeline's process group */
				setpgid(pid, m_pgid);
			}
		}

	cleanup:
		/* cleanup pipeline */
		if (fdin  != m_stdin) close(fdin);
		if (fdout != m_stdout) close(fdout);
		/* set up the input for the next command */
		fdin = fdpipe[0];
	}

	/* prep to save */
	job current; int status;
	current.pgid   = pid;
	current.stdin  = m_stdin;
	current.stdout = m_stdout;
	current.stderr = m_stderr;
	current.status = job_status::RUNNING;
	/* waitpid:
	 *   pid <  -1 => wait for absolute value of pid
	 *   pid == -1 => wait for any child process
	 *   pid ==  0 => wait for any child whose pgid is ours
	 *   pid >   0 => wait for the specified pid
	 */

	if (!background) waitpid(pid, &status, WUNTRACED);
	else m_jobs.push_back(current), m_job_map[m_pgid] = m_jobs.size() - 1;
	
	if (WIFSTOPPED(status)) {
		current.status = job_status::STOPPED;
		m_jobs.push_back(current);
		std::cout<<"["<<(m_job_map[pid] = m_jobs.size() - 1)
				 <<"]+\tstopped\t"<<pid<<std::endl;
	} for (pid_t _pid = 0; (_pid = waitpid(-1, &status,
										   WUNTRACED|WNOHANG)) > 0;) {
	   const auto & x = m_job_map.find(_pid);
	   if (x != m_job_map.end()) {
		  /* x->second is the value */
		  std::cout<<"["<<(m_job_map[_pid] = m_jobs.size() - 1)
				   <<"]-\texited"<<std::endl;
	   }
	}
	

	/* Clear to prepare for next command */
	clear();

	/* Print new prompt if we are in a terminal. */
	if (m_interactive) prompt();

	/**
	 * @todo put in foreground or background
	 */
}

void Command::prompt()
{
	if (!printPrompt) return;
	std::string PROMPT; char * pmt = getenv("PROMPT");
	if (pmt) PROMPT = std::string(pmt);
	else PROMPT = std::string("default");

	if (isatty(0) && PROMPT == std::string("default")) {
		std::string _user = std::string(getenv("USER"));
		char buffer[100]; std::string _host;
		if (!gethostname(buffer, 100)) _host = std::string(buffer);
		else _host = std::string("localhost");
		char * _wd = NULL, * _hme = NULL;
		char cdirbuff[2048]; char * const _pwd[2] = { (char*) "pwd", (char*) NULL };
		eval_to_buffer(_pwd, cdirbuff, 2048);
		std::string _cdir = std::string(cdirbuff);
		char * _curr_dur = strndup(_cdir.c_str(), _cdir.size() - 1);
		if (setenv("PWD", _curr_dur, 1)) perror("setenv");
		std::string _home = std::string(_hme = getenv("HOME"));

		// Replace the user home with ~
		if (strstr(_curr_dur, _hme)) _cdir.replace(0, _home.size(), std::string("~"));

		if (_user != std::string("root")) {
			std::cout<<"\x1b[36;1m"<<_user<<"@"<<_host<<" ";
			std::cout<<"\x1b[33;1m"<<_cdir<<"$ "<<"\x1b[0m";
			fflush(stdout);
		} else {
			// Root prompt is cooler than you
			std::cout<<"\x1b[31;1m"<<_user<<"@"<<_host<<" ";
			std::cout<<"\x1b[35;1m"<<_cdir<<"# "<<"\x1b[0m";
			fflush(stdout);
		} free(_curr_dur);
	} else fflush(stdout);
}

Command Command::currentCommand;
std::shared_ptr<SimpleCommand> Command::currentSimpleCommand;

int yyparse(void);

std::vector<std::string> splitta(std::string s, char delim) {
	std::vector<std::string> elems; std::stringstream ss(s);
	std::string item;
	for (;std::getline(ss, item, delim); elems.push_back(std::move(item)));
	return elems;
}

void Command::setAlias(const char * _from, const char * _to)
{
	std::string from(_from); std::string to(_to);
	std::vector<std::string> split = splitta(to, ' ');
	
	/**
	 * We really don't care if the alias has been
	 * set. We should just overwrite the current
	 * alias. So, we just use the [] operator
	 * in map.
	 */

	m_aliases[from] = split;
}

void Command::pushDir(const char * new_dir) {
	char * _pwd = getenv("PWD");
   
	if (_pwd == NULL) {
		perror("pwd");
		return;
	} else if (new_dir == NULL || *new_dir == '\0') {
		std::cerr<<"Invalid new directory!"<<std::endl;
		return;
	}
   
	std::string curr_dir = std::string(getenv("PWD"));
	std::string news(new_dir);

	news = tilde_expand(news);
	if(news.find_first_of("*") != std::string::npos) news = curr_dir + "/" + news;

	wildcard_expand((char*)news.c_str());
   
	if(!wc_collector.size() && changedir(news)) {
		m_dir_stack.insert(m_dir_stack.begin(), curr_dir);
	} else if(wc_collector.size()) {
	  
		for (int y = wc_collector.size() - 1; y--; ) {
			auto x = wc_collector[y];
			if (is_directory(x)) m_dir_stack.insert(m_dir_stack.begin(), x);
		} if (m_dir_stack.size()) {
			changedir(m_dir_stack[0]);
			m_dir_stack.erase(m_dir_stack.begin(), m_dir_stack.begin() + 1);
			m_dir_stack.push_back(curr_dir);
		} else goto clear_and_exit;
	} else goto clear_and_exit;
   
	for(auto && a: m_dir_stack) std::cout<<a<<" ";
	if(!m_dir_stack.empty()) std::cout<<std::endl;
clear_and_exit:
	wc_collector.clear();
	wc_collector.shrink_to_fit();
}

void Command::popDir() {
	if (!m_dir_stack.size()) {
		std::cerr<<"No directories left to pop!"<<std::endl;
		return;
	}
   
	std::string dir = tilde_expand(m_dir_stack.front());
	if(changedir(dir)) {
		m_dir_stack.erase(m_dir_stack.begin(), m_dir_stack.begin()+1);
	}
	for(auto && a: m_dir_stack) std::cout<<a<<" ";
	std::cout<<std::endl;
}
