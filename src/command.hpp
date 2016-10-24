#ifndef __COMMAND_HPP__
#define __COMMAND_HPP__
/* File Includes */
#include "simple_command.hpp"
struct SimpleCommand;

class Command
{
public:
	Command();

	void prompt();
	void print();
	void execute();
	void old_execute();
	void clear();

	void pushDir(const char * new_dir);
	void popDir();
   
	void ctrlc_handler(int);
	void sigchld_handler(int);
	       
	void insertSimpleCommand(std::shared_ptr<SimpleCommand> simpleCommand);

	void set_in_file(char * _fd);
	void set_out_file(char * _fd);
	void set_err_file(char * _fd);

	void setAlias(const char * _from, const char * _to);

	void subShell(char * arg);

	const bool & inIsSet()  { return inSet; }
	const bool & outIsSet() { return outSet; }
	const bool & errIsSet() { return errSet; }
	const bool & is_interactive() { return m_interactive; }
	
	void setAppend(const bool & ap) { append = ap; }
	void setBackground(const bool & bg) { background = bg; }

	static Command currentCommand;
	static std::shared_ptr<SimpleCommand> currentSimpleCommand;

	const int & get_stdin()  { return m_stdin; }
	const int & get_stdout() { return m_stdout; }
	const int & get_stderr() { return m_stderr; }

	void set_interactive(const bool & _interactive) {
		m_interactive = _interactive;
	}

	pid_t m_pgid = 0;
	
	std::map<std::string, std::vector<std::string> > m_aliases;
   
	std::vector<std::string> wc_collector;// Wild card collection tool

	bool printPrompt = true;
private:
	std::vector<std::string> string_split(std::string s, char delim) {
		std::vector<std::string> elems; std::stringstream ss(s);
		for (std::string item;std::getline(ss, item, delim); elems.push_back(item));
		return elems;
	}

	int get_output_flags();
	
	std::unique_ptr<char> outFile;
	std::unique_ptr<char> inFile;
	std::unique_ptr<char> errFile;

	bool append = false;
	bool background = false;
	bool m_interactive = false;
	int numOfSimpleCommands = 0;
	
	bool inSet  = false; int m_stdin  = 0;
	bool outSet = false; int m_stdout = 1;
	bool errSet = false; int m_stderr = 2;

	std::vector<std::string> m_dir_stack;
   
	std::vector<std::shared_ptr<SimpleCommand> > simpleCommands;
	std::vector<std::string> m_history;
};

/* Signal Handlers */
void sigchld_handler(int signum);
void ctrlc_handler(int signum);
#endif
