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
   read_line() { m_get_mode = 1; }

   bool write_with_error(int _fd, char & c);
   bool write_with_error(int _fd, const char * s);
   bool write_with_error(int _fd, const char * s, const size_t & len);

   bool read_with_error(int _fd, char & c);
   size_t get_term_width();

   bool handle_enter(std::string & _line, char & input);
   bool handle_tab(std::string & _line);
   
   bool handle_ctrl_a(std::string & _line);
   bool handle_ctrl_d(std::string & _line);
   bool handle_ctrl_e(std::string & _line);
   bool handle_ctrl_k(std::string & _line);
   
   bool handle_ctrl_arrow(std::string & _line);
   
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
			if (input == '!') {
			   if (!write_with_error(0, "!")) continue;

			   /* Check for "!!" and "!-<n>" */
			   if (!m_history.size()) {
				  _line += input;
				  continue;
			   }
	  
			   char ch1;
			   /* read next char from stdin */
			   if (!read_with_error(0, ch1)) continue;

			   /* print newline and stop looping */
			   if ((ch1 == '\n') && !write_with_error(1, "\n", 1)) break;
			   
			   else if (ch1 == '!') {
				  // "!!" = run prior command
				  if (!write_with_error(1, "!", 1)) continue;

				  _line += m_history[m_history.size() - 1];
				  _line.pop_back();
				  m_show_line = true;
				  continue;
			   } else if (ch1 == '-') {
				  if (!write_with_error(1, "-", 1)) continue;

				  auto && is_digit = [](char b) { return '0' <= b && b <= '9'; };
				  
                  /* "!-<n>" = run what I did n commands ago. */
				  char * buff = (char*) alloca(20); char * b;
				  for (b=buff;read(0,b,1)&&write(1,b,1)&&is_digit(*b);*(++b+1)=0);
				  int n = atoi(buff); bool run_cmd = false;
				  if (*b=='\n') run_cmd = true;
				  if (n > 0) {
					 int _idx = m_history.size() - n;
					 _line += m_history[(_idx >= 0) ? _idx : 0];
					 _line.pop_back();
					 m_show_line = true;
					 if (run_cmd) {
						if (m_buff.size()) {
						   char ch;
						   for (; m_buff.size();) {
							  ch = m_buff.top(); m_buff.pop();
							  _line += ch;
							  
							  if (!write_with_error(1, ch)) continue;
						   }
						}
						history_index = m_history.size();
						break;
					 }
				  }
			   } else {
				  if (!write_with_error(1, ch1)) continue;
				  _line += "!"; _line += ch1;
				  continue;
			   }
			}
	
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
		 else if (input == 11 && !handle_ctrl_k(_line)) continue;
		 else if (input == 8 || input == 127) {
			/* backspace */
			if (!_line.size()) continue;

			if (m_buff.size()) {
			   // Buffer!
			   char b = '\b';
			   if (!write(1, &b, 1)) {
				  perror("write");
				  continue;
			   }
			   _line.pop_back();
			   std::stack<char> temp = m_buff;
			   for (char d = 0; temp.size(); ) {
				  d = temp.top(); temp.pop();
				  if (!write(1, &d, 1)) {
					 perror("write");
					 continue;
				  }
			   }
			   b = ' ';
			   if (!write(1, &b, 1)) {
				  perror("write");
				  continue;
			   } b = '\b';
		  
			   if (!write(1, &b, 1)) {
				  perror("write");
				  continue;
			   }

			   /* get terminal width */
			   register size_t term_width = get_term_width();
			   register size_t line_size = _line.size() + m_buff.size() + 2;
		  
			   for (size_t x = 0; x < m_buff.size(); ++x, --line_size) {
				  /* if the cursor is at the end of a line, print line up */
				  if (line_size && (line_size % term_width == 0)) {
					 /* need to go up a line */			  
					 const size_t p_len = strlen("\033[1A\x1b[33;1m$ \x1b[0m");

					 /* now we print the string */
					 if (line_size == term_width) {	
						if (!write(1, "\033[1A\x1b[33;1m$ \x1b[0m", p_len)) {
						   /**
							* @todo Make sure you print the correct prompt!
							* this currently is not going to print that prompt.
							*/
						   perror("write");
						   continue;
						}  else if (!write(1, _line.c_str(), _line.size())) {
						   perror("write");
						   continue;
						} else break;
					 } else {
						if (!write(1, "\033[1A \b", strlen("\033[1A \b"))) {
						   perror("write");
						   continue;
						} else if (!write(1, _line.c_str() + (_line.size() - line_size),
										  _line.size() - line_size)) {
						   perror("write");
						   continue;
						}
					 }
				  } else if (!write(1, "\b", 1)) {
					 perror("write");
					 continue;
				  }
			   }
			} else {
			   char ch = '\b';
			   if (!write(1, &ch, 1)) {
				  perror("write");
				  continue;
			   }
			   ch = ' ';
			   if (!write(1, &ch, 1)) {
				  perror("write");
				  continue;
			   }
			   ch = '\b';
			   if (!write(1, &ch, 1)) {
				  perror("write");
				  continue;
			   }
			   _line.pop_back();
			} if (((size_t) history_index == m_history.size()) && m_current_line_copy.size()) m_current_line_copy.pop_back();
		 }
		 else if (input == 9 && !handle_tab(_line)) continue;
		 else if (input == 27) {	
			char ch1, ch2;
			// Read the next two chars
			if (!read_with_error(0, ch1)) continue;
			else if (!read_with_error(0, ch2)) continue;
		
			/* handle ctrl + arrow key */
			if ((ch1 == 91 && ch2 == 49) && !handle_ctrl_arrow(_line)) continue;
			
			else if (ch1 == 91 && ch2 == 51) {
			   /* delete key */			   
			   char ch3;

			   if (!read(0, &ch3, 1)) {
				  perror("read");
				  continue;
			   }
			   if (ch3 == 126) {
				  if (!m_buff.size()) continue;
				  if (!_line.size()) {
					 m_buff.pop();
					 std::stack<char> temp = m_buff;
					 for (char d = 0; temp.size();)
						if (write(1, &(d = (temp.top())), 1)) temp.pop();

					 if (!write(1, " ", 1)) std::cerr<<"WAT.\n";
					 for (int x = m_buff.size() + 1; x -= write(1, "\b", 1););
					 continue;
				  }

				  if (m_buff.size()) {
					 // Buffer!
					 std::stack<char> temp = m_buff;
					 temp.pop();
					 for (char d = 0; temp.size(); ) {
						d = temp.top(); temp.pop();
						if (!write(1, &d, 1)) {
						   perror("write");
						   continue;
						}
					 }
					 char b = ' ';
					 if (!write(1, &b, 1)) {
						perror("write");
						continue;
					 } b = '\b';
					 if (!write(1, &b, 1)) {
						perror("write");
						continue;
					 } m_buff.pop();
					 // Move cursor to current position.
					 for (size_t x = 0; x < m_buff.size(); ++x) {
						if (!write(1, &b, 1)) {
						   perror("write");
						   continue;
						}
					 }
				  } else continue;
				  if ((size_t) history_index == m_history.size()) m_current_line_copy.pop_back();
			   }
			}
			if (ch1 == 91 && ch2 == 65) {
			   // This was an up arrow.
			   // We will print the line prior from history.
			   if (!m_history.size()) continue;
			   // if (history_index == -1) continue;
			   // Clear input so far
			   char ch[_line.size() + 1]; char sp[_line.size() + 1];
			   memset(ch, '\b',_line.size()); memset(sp, ' ', _line.size());
			   if (write(1, ch, _line.size()) != (int) _line.size()) {
				  perror("write");
				  continue;
			   } else if (write(1, sp, _line.size()) != (int) _line.size()) {
				  perror("write");
				  continue;
			   } else if (write(1, ch, _line.size()) != (int) _line.size()) {
				  perror("write");
				  continue;
			   }

			   if ((size_t) history_index == m_history.size()) --history_index;
			   // Only decrement if we are going beyond the first command (duh).
			   _line = m_history[history_index];
			   history_index = (!history_index) ? history_index : history_index - 1;
			   // Print the line
			   if (_line.size()) _line.pop_back();
			   if (write(1, _line.c_str(), _line.size()) != (int) _line.size()) {
				  perror("write");
				  continue;
			   }
			} if (ch1 == 91 && ch2 == 66) {
			   // This was a down arrow.
			   // We will print the line prior from history.
	  
			   if (!m_history.size()) continue;
			   if ((size_t) history_index == m_history.size()) continue;
			   // Clear input so far
			   for (size_t x = 0, bsp ='\b'; x < _line.size(); ++x) {
				  if (!write(1, &bsp, 1)) {
					 perror("write");
					 continue;
				  }
			   }
			   for (size_t x = 0, sp = ' '; x < _line.size(); ++x) {
				  if (!write(1, &sp, 1)) {
					 perror("write");
					 continue;
				  }
			   }
			   for (size_t x = 0, bsp ='\b'; x < _line.size(); ++x) {
				  if (!write(1, &bsp, 1)) {
					 perror("write");
					 continue;
				  }
			   }
	  
			   history_index = ((size_t) history_index == m_history.size()) ? m_history.size()
				  : history_index + 1;
			   if ((size_t) history_index == m_history.size()) _line = m_current_line_copy;
			   else _line = m_history[history_index];
			   if (_line.size() && (size_t) history_index != m_history.size()) _line.pop_back();
			   // Print the line
			   if (write(1, _line.c_str(), _line.size()) != (int) _line.size()) {
				  perror("write");
				  continue;
			   }
			}
			if (ch1 == 91 && ch2 == 67) {
			   /* Right Arrow Key */
			   if (!m_buff.size()) continue;

			   char wrt = m_buff.top();
			   if (!write(1, &wrt, 1)) {
				  perror("write");
				  continue;
			   }
			   _line += wrt; m_buff.pop();
			} if (ch1 == 91 && ch2 == 68) {
			   if (!_line.size()) continue;
			   /* Left Arrow Key */

			   /* grab width of terminal */
			   struct winsize w;
			   ioctl(1, TIOCGWINSZ, &w);
			   size_t term_width = w.ws_col;

			   /* check if we need to go up a line */
			   /*  The plus 2 comes from the "$ "  */
			   if ((_line.size() == (term_width - 2))) {
				  /* need to go up a line */		   
				  const size_t p_len = strlen("\033[1A\x1b[33;1m$ \x1b[0m");

				  /* now we print the string */
				  if (!write(1, "\033[1A\x1b[33;1m$ \x1b[0m", p_len)) {
					 /**
					  * @todo Make sure you print the correct prompt!
					  */
					 perror("write");
					 continue;
				  } else if (!write(1, _line.c_str(), term_width - 2)) {
					 perror("write");
					 continue;
				  }
			   } else if (!((_line.size() + 2) % (term_width))) {
				  /* This case is for more than one line of backtrack */
				  if (!write(1, "\033[1A \b", strlen("\033[1A \b"))) perror("write");
				  else if (!write(1, _line.c_str() - 2 + (term_width  * ((_line.size() - 2) / term_width)), term_width)) {
					 perror("write");
					 continue;
				  }
			   }
			   m_buff.push(_line.back());
			   char bsp = '\b';
			   if (!write(1, &bsp, 1)) {
				  perror("write");
				  continue;
			   }
			   _line.pop_back();
			}
		 }      
	  }

	  _line += (char) 10 + '\0';
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
	  tcgetattr(0,&oldtermios);
	  termios tty_attr = oldtermios;
	  /* Set raw mode. */
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
   std::string m_filename;
   std::ifstream * m_ifstream;
   int m_get_mode;
   std::vector<std::string> m_history;
   bool m_show_line = false;
   termios oldtermios;
};
#endif
