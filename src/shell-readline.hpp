#ifndef __SHELL_READLINE_HPP__
#define __SHELL_READLINE_HPP__
/* Linux Includes */
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <functional>
#include <termios.h>
#include <algorithm>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <alloca.h>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <regex.h>
#include <fcntl.h>
#include <thread>
#include <vector>
#include <cctype>
#include <stack>
/** Include wildcards for tab completion **/
#include "wildcard.hpp"
#include "command.hpp"

extern FILE * yyin;
extern void yyrestart(FILE*);
extern int yyparse();

class read_line
{
public:
	read_line();

	bool write_with_error(int _fd, char & c);
	bool write_with_error(int _fd, const char * s);
	bool write_with_error(int _fd, const char * s, const size_t & len);

	bool read_with_error(int _fd, char & c);
	size_t get_term_width();

	bool handle_enter(std::string & _line, char & input);
	bool handle_backspace(std::string & _line);
	bool handle_bang(std::string & _line);
	bool handle_delete(std::string & _line);
	bool handle_tab(std::string & _line);
   
	bool handle_ctrl_a(std::string & _line);
	bool handle_ctrl_d(std::string & _line);
	bool handle_ctrl_e(std::string & _line);
	bool handle_ctrl_k(std::string & _line);
   
	bool handle_ctrl_arrow(std::string & _line);

	bool handle_up_arrow(std::string & _line);
	bool handle_down_arrow(std::string & _line);
	bool handle_left_arrow(std::string & _line);
	bool handle_right_arrow(std::string & _line);
  
	void operator() () {
		// Raw mode
		char input;
		std::string _line = "";

		// Read in the next character
		for (; true ;) {
			/* If you can't read from 0, don't continue! */
			if (!read_with_error(0, input)) break;

			// Echo the character to the screen
			if (input >= 32 && input != 127) {
				// Character is printable
				if (input == '!' && !handle_bang(_line)) continue;
	
				if (m_buff.size()) {
					/**
					 * If the buffer has contents, they are
					 * in the middle of a line.
					 */
					_line += input;
	  
					/* Write current character */
					if (!write_with_error(1, input)) continue;

					/* Copy buffer and print */
					std::stack<char> temp = m_buff;
					for (char d = 0; temp.size(); ) {
						d = temp.top(); temp.pop();
						if (!write_with_error(1, d)) continue;
					}

					// Move cursor to current position.
					for (size_t x = 0; x < m_buff.size(); ++x) {
						if (!write_with_error(1, "\b", 1)) continue;
					}
				} else {
					_line += input;
					if ((size_t)history_index == m_history.size())
						m_current_line_copy += input;
					/* Write to screen */
					if (!write_with_error(1, input)) continue;
				}
			} else if (input == 10) {
				if (handle_enter(_line, input)) break;
				else continue;
			} else if (input == 1) {
				if (handle_ctrl_a(_line)) break;
				else continue;
			}
			else if (input == 5 && !handle_ctrl_e(_line)) continue;
			else if (input == 4 && !handle_ctrl_d(_line)) continue;
			else if (input == 11) {
				if (!handle_ctrl_k(_line)) continue;
				else break;
			}
			else if ((input == 8 || input == 127) && !handle_backspace(_line)) continue;
			else if (input == 9 && !handle_tab(_line)) continue;
			else if (input == 27) {	
				char ch1, ch2;
				// Read the next two chars
				if (!read_with_error(0, ch1)) continue;
				else if (!read_with_error(0, ch2)) continue;
		
				/* handle ctrl + arrow key */
				if ((ch1 == 91 && ch2 == 49) && !handle_ctrl_arrow(_line)) continue;
			
				else if (ch1 == 91 && ch2 == 51 && !handle_delete(_line)) continue;
				if (ch1 == 91 && ch2 == 65 && !handle_up_arrow(_line)) continue;
				if (ch1 == 91 && ch2 == 66 && !handle_down_arrow(_line)) continue;
				if (ch1 == 91 && ch2 == 67 && !handle_right_arrow(_line)) continue;
				if (ch1 == 91 && ch2 == 68 && !handle_left_arrow(_line)) continue;
			}
		}
		
		if (_line.size()) {
			if (!write_with_error(m_history_fd, _line.c_str(), _line.size())) return;
			else if (!write_with_error(m_history_fd, "\n", 1)) return;
		} _line += (char) 10 + '\0';
		m_current_line_copy.clear();
		m_history.push_back(_line);
	}

	char * getStashed() {
		char * ret = (char*) calloc(m_stashed.size() + 1, sizeof(char));
		strncpy(ret, m_stashed.c_str(), m_stashed.size());
		ret[m_stashed.size()] = '\0';
		m_get_mode = 1;
		std::cerr<<"get returning: "<<ret<<std::endl;
		return ret;
	}
  
	char * get() {
		if (m_get_mode == 2) {
			char * ret = (char*) calloc(m_stashed.size() + 1, sizeof(char));
			strncpy(ret, m_stashed.c_str(), m_stashed.size());
			ret[m_stashed.size()] = '\0';
			m_get_mode = 1;
			std::cerr<<"get returning: "<<ret<<std::endl;
			return ret;
		}
      
		std::string returning;
		returning = m_history[m_history.size() - 1];
		/* returning.pop_back(); */
		char * ret = (char*) calloc(returning.size() + 1, sizeof(char));
		strncpy(ret, returning.c_str(), returning.size());
		ret[returning.size()] = '\0';
		return ret;
	}

   
	void tty_raw_mode() {
		pid_t pid = Command::currentCommand.m_shell_pgid;
		/* become a strong independent black woman */
		tcsetpgrp(0, pid);
		/* save defaults for later */
		tcgetattr(0,&oldtermios);		
		termios tty_attr = oldtermios;
		/* set raw mode */
		tty_attr.c_lflag &= (~(ICANON|ECHO));
		tty_attr.c_cc[VTIME] = 0;
		tty_attr.c_cc[VMIN] = 1;
			
		tcsetattr(0,TCSANOW,&tty_attr);
	}
	
	void unset_tty_raw_mode() {
		tcsetattr(0, TCSAFLUSH, &oldtermios);
	}

	void setFile(const std::string & _filename) {
		std::string expanded_home = tilde_expand(_filename.c_str());

		char * file = strndup(expanded_home.c_str(), expanded_home.size());

		yyin = fopen(file, "r"); free(file);

		if (yyin != NULL) {
			Command::currentCommand.printPrompt = false;
			yyrestart(yyin); yyparse();
			fclose(yyin);

			yyin = stdin;
			yyrestart(yyin);
			Command::currentCommand.printPrompt = true;
		} Command::currentCommand.prompt();
		yyparse();
	}

	void save_history();
	void load_history();
	const std::vector<std::string> & get_history();
	termios oldtermios;
private:
	std::vector<std::string> string_split(std::string s, char delim) {
		std::vector<std::string> elems; std::stringstream ss(s);
		std::string item;
		for (;std::getline(ss, item, delim); elems.push_back(std::move(item)));
		return elems;
	}
	std::string m_current_path;
	std::string m_current_line_copy;
	std::string m_stashed;
	std::stack<char> m_buff;
	std::vector<std::string> m_path;
	ssize_t history_index = 0;
	int fdin = 0;
	int m_history_fd = 0;
	std::string m_filename;
	std::ifstream * m_ifstream;
	int m_get_mode;
	std::vector<std::string> m_history;
	bool m_show_line = false;
};
#endif
