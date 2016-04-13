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
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <fstream>
#include <stdio.h>
#include <vector>
#include <cctype>
#include <mutex>
#include <stack>

#define SOURCE 5053
#define STANDARD 572

static int get_file = 0;
// static readLine reader;

static class readLine
{
 public:
  readLine() { m_get_mode = 1; }

  void operator() () {
    // Raw mode
    char input; int result;
    std::string _line;

    // Read in the next character
    for (; true ;) {
      result = read(0, &input,  1);
      // Echo the character to the screen
      if (input >= 32 && input != 127) {
	// Character is printable

	if (m_buff.size()) {
	  /**
	   * If the buffer has contents, they are
	   * in the middle of a line.
	   */
	  if (input == '>') {
	    _line += ' ';
	    _line += input;
	    _line += ' ';
	  } else _line += input;
	  
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
	  if (input == '>') {
	    _line += ' ';
	    _line += input;
	    _line += ' ';
	  } else _line += input;
	  if (history_index == m_history.size())
	    m_current_line_copy += input;
	  // Write to screen
	  result = write(1, &input, 1);
	}
      } else if (input == 10) {
	// Enter was typed
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
      } else if (input == '\t') {
	/**
	 * @todo complete tab complete
	 */
      } else if (input == '!') {
	/**
	 * @todo complete the bang-bang
	 */
      } else if (input == 27) {
	/**
	 * @todo escape sequences
	 */
	
	char ch1, ch2;
	// Read the next two chars
	result = read(0, &ch1, 1);
	result = read(0, &ch2, 1);

	if (ch1 == 91 && ch2 == 65) {
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
    //std::cerr<<"Read mode set!"<<std::endl;
    //std::cerr<<"mode = "<<m_mode<<std::endl;
    m_filename = _filename;
    m_ifstream = new std::ifstream(m_filename);
    std::string line; std::string _line;
    for (; !m_ifstream->eof();) {
      std::getline(*m_ifstream, _line);
      //      std::cerr<<"line: \""<<line<<"\""<<std::endl;
      line += _line + (char) 10;
    } //std::cerr<<"End of loop"<<std::endl;
    m_get_mode = 2;
    m_stashed = line;
    get_file = 1; delete m_ifstream;
    //std::cerr<<"mode: "<<m_get_mode<<std::endl;
    //std::cerr<<"stashed: \""<<line<<"\""<<std::endl;
  }
  
 private:
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
  termios oldtermios;
} reader;
#endif
