#ifndef KYBD_READ_THREAD
#define KYBD_READ_THREAD
/**
 * This is the keyboard callable. 
 * This example implements a more
 * C++-y version of read-line.c.
 */
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
#include <vector>
#include <cctype>
#include <mutex>
#include <stack>

/** Include wildcards for tab completion **/
#include "command.hpp"

#define SOURCE 5053
#define STANDARD 572
#define MAXLEN 1024

static int get_file = 0;
// static readLine reader;

struct Lensort {
  bool operator () (char*& ch1, char*& ch2) { return strlen(ch1) < strlen(ch2); }
};

inline char * longest_substring(const std::vector<std::string> & _vct) {
  char * _substr = NULL; std::vector<char*> vct;
  for (auto && x : _vct) vct.push_back(strndup(x.c_str(), x.size()));
  std::sort(vct.begin(), vct.end(), Lensort());
  size_t minlen = strlen(vct[0]); char * last = NULL; int y = 1;
  for (char * s = strndup(vct[0],1); y < minlen; s = strndup(vct[0], y++)) {
    register volatile unsigned short count = 0;
    for (auto && x : vct) if (!strncmp(x, s, minlen)) ++count;
    if (count == vct.size()) free(last), last = s;
    else free(s);
  } return last;
}

static class readLine
{
public:
  readLine() { m_get_mode = 1; }

  void operator() () {
    // Raw mode
    char input; int result;
    std::string _line = "";

    // Read in the next character
    for (; true ;) {
      result = read(0, &input,  1);
      // Echo the character to the screen
      if (input >= 32 && input != 127) {
	// Character is printable
	if (input == '!') {
	  result = write(0, "!", 1);
	  //Check for "!!" and "!-<n>"
	  if (!m_history.size()) continue;

	  char ch1, ch2;
	  result = read(0, &ch1, 1);
	  if (ch1 == '!') {
	    // "!!" = run prior command
	    result =  write(1, "!", 1);
	    _line += m_history[m_history.size() - 1];
	    _line.pop_back();
	    m_show_line = true;
	    continue;
	  } else if (ch1 == '-') {
	    result = write(1, "-", 1);
	    auto && is_digit = [](char b) { return '0' <= b && b <= '9'; };
	    // "!-<n>" = run what I did n commands ago.
	    char * buff = (char*) alloca(20); char * b;
	    for (b=buff;read(0,b,1)&&write(1,b,1)&&is_digit(*b);*(++b+1)=0);
	    int n = atoi(buff); bool run_cmd = false;
	    if (*b=='\n') run_cmd = true;
	    if (n > 0) {
	      _line += m_history[m_history.size() - n];
	      _line.pop_back();
	      m_show_line = true;
	      if (run_cmd) {
		if (m_buff.size()) {
		  char ch;
		  for (; m_buff.size();) {
		    ch = m_buff.top(); m_buff.pop();
		    _line += ch;
		    result = write(1, &ch, 1);
		  }
		}
		result = write(1, &input, 1);
		history_index = m_history.size();
		break;
	      }
	    }
	    continue;
	  }
	}
	
	if (m_buff.size()) {
	  /**
	   * If the buffer has contents, they are
	   * in the middle of a line.
	   */
	  _line += input;
	  
	  // Write current character
	  result = write(1, &input, 1);

	  // Copy buffer and print
	  std::stack<char> temp = m_buff;
	  for (char d = 0; temp.size(); ) {
	    d = temp.top(); temp.pop();
	    result = write(1, &d, 1);
	  }

	  // Move cursor to current position.
	  for (int x = 0, b = '\b'; x < m_buff.size(); ++x)
	    result = write(1, &b, 1);
	} else {
	  _line += input;
	  if (history_index == m_history.size())
	    m_current_line_copy += input;
	  // Write to screen
	  result = write(1, &input, 1);
	}
      } else if (input == 10) {
	// Enter was typed
      enter:
	if (m_buff.size()) {
	  char ch;
	  for (; m_buff.size();) {
	    ch = m_buff.top(); m_buff.pop();
	    _line += ch;
	    result = write(1, &ch, 1);
	  }
	}
	result = write(1, &input, 1);
	history_index = m_history.size();
        break;
      } else if (input == 1) {
	// Control A
	if (!_line.size()) continue;

	for (char d = '\b'; _line.size();) {
	  m_buff.push(_line.back()); _line.pop_back();
	  result = write(1, &d, 1);
	}
      } else if (input == 5) {
	// Control E

	for (char d = 0; m_buff.size();) {
	  d = m_buff.top(); m_buff.pop();
	  result = write(1, &d, 1);
	  _line += d;
	} 
      } else if (input == 4) {
	// Control D
	if (!_line.size()) continue;

	if (m_buff.size()) {
	  // Buffer!
	  std::stack<char> temp = m_buff;
	  temp.pop();
	  for (char d = 0; temp.size(); ) {
	    d = temp.top(); temp.pop();
	    result = write(1, &d, 1);
	  }
	  char b = ' ';
	  result = write(1, &b, 1); b = '\b';
	  result = write(1, &b, 1); m_buff.pop();
	  // Move cursor to current position.
	  for (int x = 0; x < m_buff.size(); ++x)
	    result = write(1, &b, 1);
	} else continue;
	if (history_index == m_history.size()) m_current_line_copy.pop_back();
      } else if (input == 8 || input == 127) {
	// backspace
        if (!_line.size()) continue;

	if (m_buff.size()) {
	  // Buffer!
	  char b = '\b';
	  result = write(1, &b, 1);
	  _line.pop_back();
	  std::stack<char> temp = m_buff;
	  for (char d = 0; temp.size(); ) {
	    d = temp.top(); temp.pop();
	    result = write(1, &d, 1);
	  }
	  b = ' ';
	  result = write(1, &b, 1); b = '\b';
	  result = write(1, &b, 1);
	  // Move cursor to current position.
	  for (int x = 0; x < m_buff.size(); ++x)
	    result = write(1, &b, 1);
	} else {
	  char ch = '\b';
	  result = write(1, &ch, 1);
	  ch = ' ';
	  result = write(1, &ch, 1);
	  ch = '\b';
	  result = write(1, &ch, 1);
	  _line.pop_back();
	}
	if (history_index == m_history.size()) m_current_line_copy.pop_back();
      } else if (input == 9) {
	Command::currentSimpleCommand = std::unique_ptr<SimpleCommand>(new SimpleCommand());
	
	// Part 1: add a '*' to the end of the stream.
	std::string _temp;
	// std::cerr<<std::endl;
	std::vector<std::string> _split;
	if (_line.size()) {
	  _split = string_split(_line, ' ');
	  _temp = _split.back() + "*";
	} else _temp = "*";

	char * _complete_me = strndup(_temp.c_str(), _temp.size());
	
	// Part 2: Invoke wildcard expand
	wildcard_expand(NULL, _complete_me);

	// Part 3: If wc_collector.size() <= 1, then complete the tab.
	//         otherwise, run "echo <text>*"
	std::string * array = Command::currentCommand.wc_collector.data();
	std::sort(array, array + Command::currentCommand.wc_collector.size(), Comparator());

	if (Command::currentCommand.wc_collector.size() == 1) {
	  char ch = '\b'; char sp =  ' ';
	  for (int x = 0, y = 0; x < _line.size(); y = write(1, &ch, 1), ++x);
	  for (int x = 0, y = 0; x < _line.size(); y = write(1, &sp, 1), ++x);
	  for (int x = 0, y = 0; x < _line.size(); y = write(1, &ch, 1), ++x);
	  _line = "";
	  
	  for (int x = 0; x < _split.size() - 1; _line += _split[x++] + " ");
	  _line += Command::currentCommand.wc_collector[0];
	  m_current_line_copy = _line;
	  Command::currentCommand.wc_collector.clear();
	  Command::currentCommand.wc_collector.shrink_to_fit();
	} else {
	  unsigned short x = 0; std::cerr<<std::endl;
	  for (auto && arg : Command::currentCommand.wc_collector) {
	    char * temp = strdup(arg.c_str());
	    std::cerr<<temp<<std::endl;
	    Command::currentSimpleCommand->insertArgument(temp);
	    free(temp); x = x & 1;
	  }
	
	  Command::currentSimpleCommand->insertArgument(strdup("echo"));
	  Command::currentCommand.wc_collector.clear();
	  Command::currentCommand.wc_collector.shrink_to_fit();
	  Command::currentCommand.execute();
	} free(_complete_me);

	if (write(1, _line.c_str(), _line.size()) != _line.size()) {
	  perror("write");
	  std::cerr<<"I.E. STAHP!\n"<<std::endl;
	}
      } else if (input == 27) {
	/**
	 * @todo escape sequences
	 */
	
	char ch1, ch2;
	// Read the next two chars
	result = read(0, &ch1, 1);
	result = read(0, &ch2, 1);
	if (ch1 == 91 && ch2 == 51) {
	  // Maybe a delete key?
	  char ch3; result = read(0, &ch3, 1);
	  if (ch3 == 126) {
	    if (!_line.size()) continue;

	    if (m_buff.size()) {
	      // Buffer!
	      std::stack<char> temp = m_buff;
	      temp.pop();
	      for (char d = 0; temp.size(); ) {
		d = temp.top(); temp.pop();
		result = write(1, &d, 1);
	      }
	      char b = ' ';
	      result = write(1, &b, 1); b = '\b';
	      result = write(1, &b, 1); m_buff.pop();
	      // Move cursor to current position.
	      for (int x = 0; x < m_buff.size(); ++x)
		result = write(1, &b, 1);
	    } else continue;
	    if (history_index == m_history.size()) m_current_line_copy.pop_back();
	  }
	} if (ch1 == 91 && ch2 == 65) {
	  // This was an up arrow.
	  // We will print the line prior from history.
          if (!m_history.size()) continue;
	  // if (history_index == -1) continue;
	  // Clear input so far
	  for (int x = 0, bsp ='\b'; x < _line.size(); ++x)
	    result = write(1, &bsp, 1);
          for (int x = 0, sp = ' '; x < _line.size(); ++x)
	    result = write(1, &sp, 1);
          for (int x = 0, bsp ='\b'; x < _line.size(); ++x)
	    result = write(1, &bsp, 1);

	  if (history_index == m_history.size()) --history_index;
	  // Only decrement if we are going beyond the first command (duh).
	  _line = m_history[history_index];
	  history_index = (!history_index) ? history_index : history_index - 1;
	  // Print the line
	  if (_line.size()) _line.pop_back();
	  result = write(1, _line.c_str(), _line.size());
	} if (ch1 == 91 && ch2 == 66) {
	  // This was a down arrow.
	  // We will print the line prior from history.
	  
          if (!m_history.size()) continue;
	  if (history_index == m_history.size()) continue;
	  // Clear input so far
	  for (int x = 0, bsp ='\b'; x < _line.size(); ++x)
	    result = write(1, &bsp, 1);
          for (int x = 0, sp = ' '; x < _line.size(); ++x)
	    result = write(1, &sp, 1);
          for (int x = 0, bsp ='\b'; x < _line.size(); ++x)
	    result = write(1, &bsp, 1);
	  
	  history_index = (history_index == m_history.size()) ? m_history.size()
	    : history_index + 1;
	  if (history_index == m_history.size()) _line = m_current_line_copy;
	  else _line = m_history[history_index];
	  if (_line.size() && history_index != m_history.size()) _line.pop_back();
	  // Print the line
	  result = write(1, _line.c_str(), _line.size());
	} if (ch1 == 91 && ch2 == 67) {
	  // Right Arrow
	  if (!m_buff.size()) continue;
	  char wrt = m_buff.top();
	  result = write(1, &wrt, 1);
	  _line += wrt; m_buff.pop();
	} if (ch1 == 91 && ch2 == 68) {
	  if (!_line.size()) continue;
	  // Left Arrow
	  m_buff.push(_line.back());
	  char bsp = '\b';
	  result = write(1, &bsp, 1);
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
    if (m_mode == SOURCE) m_history.pop_back();
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

  void setMode(const int & _mode) { m_mode = _mode; }

  void setFile(const std::string & _filename) {
    m_filename = _filename;
    m_ifstream = new std::ifstream(m_filename);
    std::string line; std::string _line;
    for (; !m_ifstream->eof();) {
      std::getline(*m_ifstream, _line);
      line += _line + (char) 10;
    }
    m_get_mode = 2;
    m_stashed = line;
    get_file = 1; delete m_ifstream;
  }

  static void wildcard_expand(char * prefix, char * suffix) {
    if (!*suffix && prefix && *prefix)
      { Command::currentCommand.wc_collector.push_back(std::string(strdup(prefix))); return; }
    else if (!*suffix) return;
    // Get next slash (skipping first, if necessary)
    char * slash = strchr((*suffix == '/') ? suffix + 1: suffix, '/');
    char * next  = (char*) calloc(MAXLEN + 1, sizeof(char));
    // This line is magic.
    for (char * temp = next; *suffix && suffix != slash; *(temp++) = *(suffix++));

    // Expand the wildcard
    char * nextPrefix = (char*) calloc(MAXLEN + 1, sizeof(char));
    if (!(strchr(next, '*') || strchr(next, '?'))) {
      // No more wildcards.
      if (!(prefix && *prefix)) sprintf(nextPrefix, "%s", next);
      else sprintf(nextPrefix, "%s%s", prefix, next);
      wildcard_expand(nextPrefix, suffix);
      free(nextPrefix); free(next); return;
    }

    // Wildcards were found!
    // Convert to regex.
    char * rgx = (char*) calloc(2 * strlen(next) + 3, sizeof(char));
    char * trx = rgx;

    *(trx++) = '^';
    for (char * tmp = next; *tmp; ++tmp) {
      switch (*tmp) {
      case '*':
	*(trx++) = '.';
	*(trx++) = '*';
	break;
      case '?':
	*(trx++) = '.';
	break;
      case '.':
	*(trx++) = '\\';
	*(trx++) = '.';
      case '/':
	break;
      default:
	*(trx++) = *tmp;
      }
    } *(trx++) = '$'; *(trx++) = 0;
    // Compile regex.
    regex_t * p_rgx = new regex_t();
    if (regcomp(p_rgx, rgx, REG_EXTENDED|REG_NOSUB)) {
      // Exit with error if regex failed to compile.
      perror("regex (compile)");
      exit(1);
    }

    char * _dir = (prefix) ? strdup(prefix) : strdup(".");

    DIR * dir = opendir(_dir); free(_dir);
    if (!dir) {
      free(dir); free(rgx); free(next); free(nextPrefix); delete p_rgx;
      return; // Return if opendir returned null.
    }

    dirent * _entry;
    for (; _entry = readdir(dir);) {
      // Check for a match!
      if (!regexec(p_rgx, _entry->d_name, 0, 0, 0)) {
	if (!(prefix && *prefix)) sprintf(nextPrefix, "%s", _entry->d_name);
	else sprintf(nextPrefix, "%s/%s", prefix, _entry->d_name);

	if (_entry->d_name[0] == '.' && *rgx == '.') wildcard_expand(nextPrefix, suffix);
	else if (_entry->d_name[0] == '.') continue;
	else wildcard_expand(nextPrefix, suffix);
      }
    }

    // Be kind to malloc. Malloc is bae.
    free(next); free(nextPrefix); free(rgx); free(dir);
    // This one was allocated with new.
    delete p_rgx;
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
  std::vector<std::string> m_history;
  ssize_t history_index = 0;
  std::string m_filename;
  std::ifstream * m_ifstream;
  volatile int m_mode = STANDARD;
  int m_get_mode;
  bool m_show_line = false;
  termios oldtermios;
} reader;
#endif
