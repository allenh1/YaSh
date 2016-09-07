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
	char * str = strdup(arg.c_str()); int index; char * t_str = str;
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

#define SUBSH_MAX_LEN 4096
void Command::subShell(char * arg)
{
  int cmd_pipe[2]; int out_pipe[2]; pid_t pid;
  int tmpin = dup(0); int tmpout = dup(1); int tmperr = dup(2);
  
  if (pipe(cmd_pipe) == -1) {
	perror("cmd_pipe");
	return;
  } else if (pipe(out_pipe) == -1) {
	perror("out_pipe");
	return;
  } else if ((pid = fork()) == -1) {
	perror("subshell fork");
	return;
  } else if (pid == 0) {
	/* Child Process */
	close(out_pipe[0]); /* close the read end of the out pipe */
	close(cmd_pipe[1]); /* close the write end of the cmd pipe */

	dup2(out_pipe[1], 1); close(out_pipe[1]); /* out_pipe[1] -> stdout */
	dup2(cmd_pipe[0], 0); close(cmd_pipe[0]); /* cmd_pipe[0] -> stdin  */

	execlp("/proc/self/exe", "/proc/self/exe", NULL);
	perror("subshell exec");
	_exit(1);
  } else if (pid != 0) {
	char * buff = (char*) calloc(SUBSH_MAX_LEN, sizeof(char));
	register char * c = NULL;
	
	/* Parent Process */
	close(out_pipe[1]); /* close the write end of the out pipe */
	close(cmd_pipe[0]); /* close the read end of the cmd pipe */

	/* write the command to the write end of the cmd pipe */
	for (c = arg; *c && write(cmd_pipe[1], c++, 1););

	waitpid(pid, NULL, 0);
	
	/* read from the out pipe and store in a buffer */
	for (c = buff; read(out_pipe[0], c++, 1););
	
	size_t buff_len = c - buff; /* this is the number of characters read */

	/* Push the buffer onto stdin */
	for (int b = 0; (ungetc(buff[b++], stdin) != EOF) && buff_len--;);
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
  // Don't do anything if there are no simple commands
  if (numOfSimpleCommands == 0) {
	prompt();
	return;
  }

  auto changedir = [] (std::string & s) {
	/* verify that the directory exists */
	char * cpy = strndup(s.c_str(), s.size()); 
	DIR * _dir;

	if (!s.empty() && (_dir = opendir(cpy))) {
	  /* directory is there */
	  closedir(_dir);
	} else if (!s.empty() && errno == ENOENT) {
	  /* directory doesn't exist! */
	  std::cerr<<u8"¯\\_(ツ)_/¯"<<std::endl;
	  return -1;
	} else if (!s.empty()) {
	  /* cd failed because... ¯\_(ツ)_/¯ */
	  std::cerr<<u8"¯\\_(ツ)_/¯"<<std::endl;
	  return -1;
	}
    
	if (*s.c_str() && *s.c_str() != '/') {
	  // we need to append the current directory.
	  for (;s.back() == '/'; s.pop_back());
	  s = std::string(getenv("PWD")) + "/" + s;
	} else if (!*s.c_str()) {
	  for (;s.back() == '/'; s.pop_back());
	  passwd * _passwd = getpwuid(getuid());
	  std::string user_home = _passwd->pw_dir;
	  return chdir(user_home.c_str());
	} for(; *s.c_str() != '/' && s.back() == '/'; s.pop_back());
	return chdir(s.c_str());
  };

  // Print contents of Command data structure
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
  
  // Add execution here
  // For every simple command fork a new process
  int pid = -1;
  int fdpipe [2]; //for de pipes
  int fderr, fdout, fdin; char * blak;
  int tmpin  = dup(0);
  int tmpout = dup(1);
  int tmperr = dup(2);

  // Redirect the input
  if (inFile.get()) {
	fdin  = open(inFile.get(),  O_WRONLY, 0700);
  } else fdin = dup(tmpin);

  if (outFile.get()) {
	if (this->append) fdout = open(outFile.get(), O_CREAT | O_WRONLY | O_APPEND, 0600);
	else fdout = open(outFile.get(), O_CREAT | O_WRONLY | O_TRUNC, 0600);
  } else fdout = dup(tmpout);

  if (errFile.get()) {
	if (this->append) fderr = open(errFile.get(), O_CREAT | O_WRONLY | O_APPEND, 0600);
	else fderr = open(errFile.get(), O_CREAT | O_WRONLY | O_TRUNC, 0600);
  } else fderr = dup(tmperr);

  if (fderr < 0) {
	perror("error file");
	exit(1);// could not open file
  } if (fdout < 0) {
	perror("out file");
	exit(1);
  } if (fdin < 0) {
	perror("in file");
	exit(1);
  }

  // Redirect stderr to file
  dup2(fderr, 2);
  close(fderr);
  for (int x = 0; x < numOfSimpleCommands; ++x) {
	std::vector<char *> curr = simpleCommands.at(x).get()->arguments;
	char ** d_args = new char*[curr.size() + 1];
	for (int y = 0; y < curr.size(); ++y) {
	  d_args[y] = strdup(curr[y]);
	} // ... still better than managing myself!
	d_args[curr.size()] = NULL;
	// output redirection
	dup2(fdin, 0);
	close(fdin);
	/** output direction **/
	if (x == numOfSimpleCommands - 1) {
	  if (outFile.get()) {
		if (this->append) fdout = open(outFile.get(), O_CREAT|O_WRONLY|O_APPEND, 600);
		else fdout = open(outFile.get(), O_CREAT|O_WRONLY|O_TRUNC, 0600);
	  } else fdout = dup(tmpout);

	  if (errFile.get()) {
		if (this->append) fderr = open(outFile.get(), O_CREAT|O_WRONLY|O_APPEND, 600);
		else fderr = open(errFile.get(), O_CREAT|O_WRONLY|O_TRUNC, 0600);
	  } else fderr = dup(tmperr);
	} // direct to requested upon last command

	else {
	  int fdpipe[2];
	  int pipe_res = pipe(fdpipe);
	  // perror("pipe");
	  fdout = fdpipe[1];
	  fdin  = fdpipe[0];
	} // direct to input of next command

	dup2(fdout, 1);
	close(fdout);

	if (d_args[0] == std::string("cd")) {
	  std::string curr_dir = std::string(getenv("PWD"));
	  int cd; std::string new_dir;
	  if (curr.size() < 2) {
		std::string _empty = "";
		cd = changedir(_empty);
		if (cd != 0) {
		  perror("cd");
		  clear();
		  prompt();
		}
		setenv("PWD", getenv("HOME"), 1);
	  } else {
		if (d_args[1] == std::string("pwd") ||
			d_args[1] == std::string("/bin/pwd")) {
		  // Nothing to be done.
		  clear();
		  prompt();
		  return;
		} else if (*d_args[1] != '/') { 
		  new_dir = std::string(getenv("PWD"));
		  for (;new_dir.back() == '/'; new_dir.pop_back());
		  new_dir += "/" + std::string(d_args[1]);
		} else new_dir = std::string(d_args[1]);

		for (; *new_dir.c_str() != '/' && new_dir.back() == '/'; new_dir.pop_back());
		cd = changedir(new_dir);
		if (cd == 0) setenv("PWD", new_dir.c_str(), 1);
	  }

	  if (cd != 0) {
		const char * msg = "cd failed: No such file or directory\n";
		if (!write(2, msg, strlen(msg))) {
		  perror("write");
		}
		setenv("PWD", curr_dir.c_str(), 1);
	  }
	  // Regardless of errors, cd has finished.
	  clear();
	  prompt();
	  return;
	} else if (d_args[0] == std::string("ls")) {
	  char ** temp = new char*[curr.size() + 2];
	  for (int y = 2; y <= curr.size(); ++y) {
		temp[y] = strdup(curr[y - 1]);
	  } // ... still better than managing myself!
	  temp[0] = strdup("ls");
	  temp[1] = strdup("--color=auto");
	  temp[curr.size() + 1] = NULL;

	  pid = fork();

	  if (pid == 0) {
		execvp (temp[0], temp);
		perror("execvp");
		exit(2);
	  } 
	  for (int x = 0; x < curr.size() + 2; ++x) {
		free(temp[x]);
		temp[x] = NULL;
	  } delete[] temp;
	} else if (d_args[0] == std::string("setenv")) {
	  char * temp = (char*) calloc(strlen(d_args[1]) + 1, sizeof(char));
	  char * pemt = (char*) calloc(strlen(d_args[2]) + 2, sizeof(char));
	  strcpy(temp, d_args[1]); strcpy(pemt, d_args[2]);
	  int result = setenv(temp, pemt, 1);
	  if (result) perror("setenv");
	  clear(); prompt(); free(temp); free(pemt);
	  return;
	} else if (d_args[0] == std::string("unsetenv")) {
	  char * temp = (char*) calloc(strlen(d_args[1]) + 1, sizeof(char));
	  strcpy(temp, d_args[1]);
	  if (unsetenv(temp) == -1) perror("unsetenv");
	  clear(); prompt(); free(temp);
	  return;
	} else if (d_args[0] == std::string("grep")) {
	  char ** temp = new char*[curr.size() + 2];
	  for (int y = 1; y < curr.size(); ++y) {
		temp[y] = strdup(curr[y]);
	  } // ... still better than managing myself!
	  temp[0] = strdup("grep");
	  temp[curr.size()] = strdup("--color");
	  temp[curr.size() + 1] = NULL;

	  pid = fork();

	  if (pid == 0) {
		execvp (temp[0], temp);
		perror("execvp");
		exit(2);
	  } for (int x = 0; x < curr.size() + 2; ++x) {
		free(temp[x]); temp[x] = NULL;
	  } delete[] temp;
	} else {
	  // Time for a good hot fork
	  pid = fork();

	  if (pid == 0) {
		if (d_args[0] == std::string("printenv")) {
		  char ** _env = environ;
		  for (; *_env; ++_env) std::cout<<*_env<<std::endl;
		  _exit (0);
		}

		execvp (d_args[0], d_args);
		perror("execvp");
		_exit(1);
	  }
	}

	for (int x = 0; x < curr.size(); ++x) {
	  free(d_args[x]); d_args[x] = NULL;
	} delete[] d_args;
  }

  // Restore I/O defaults

  dup2(tmpin, 0);
  dup2(tmpout, 1);
  dup2(tmperr, 2);
  close(tmpin);
  close(tmpout);
  close(tmperr);

  if (!background) {
	// We are not running in the backround,
	// so we should wait for the final command
	// to execute.

	waitpid(pid, NULL, 0);
  } else m_background.push_back(pid);

  // Clear to prepare for next command
  clear();

  // Print new prompt if we are in a terminal.
  if (isatty(0)) {
	prompt();
  }
}

// Shell implementation

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
  } else {
	std::cout<<PROMPT;
	fflush(stdout);
  }
}

Command Command::currentCommand;
std::shared_ptr<SimpleCommand> Command::currentSimpleCommand;

int yyparse(void);

void ctrlc_handler(int signum)
{
  /** kill my kids **/
  kill(signum, SIGINT);
}

void sigchld_handler(int signum)
{
  int saved_errno = errno; int pid = -1;
  for(; pid = waitpid(-1, NULL, WNOHANG) > 0;) {
	bool found = false;
	for (auto && a : m_background) {
	  if (a == pid) {
		found = true;
		break;
	  }
	} if (found) {
	  std::cout<<"["<<signum<<"] has exited."<<std::endl;
	  Command::currentCommand.prompt();
	}
  } errno = saved_errno;
}

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

void Command::readShellRC()
{

}



