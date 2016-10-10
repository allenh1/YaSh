#include "shell-readline.hpp"

/** 
 * Wrapper for the write function, given a character.
 * 
 * @param _fd File descriptor on which to write.
 * @param c Character to write.
 * 
 * @return True upon successful write.
 */
bool read_line::write_with_error(int _fd, char & c)
{
   if (!write(_fd, &c, 1)) {
	  perror("write");
	  return false;
   } return true;
}

/** 
 * Wrapper for the write function, given a C string.
 *
 * This function writes the contents of s to the
 * provided file descriptor. This function will
 * write to the first occurance of a null character,
 * as it calls strlen to determine the length.
 * For those strings whose length, is known, see
 * @see write_with_error(int _fd, const char *& s, size_t len).
 * 
 * @param _fd File descriptor on which to write.
 * @param s String to write.
 * 
 * @return True upon successful write.
 */
bool read_line::write_with_error(int _fd, const char * s)
{
   if (write(_fd, s, strlen(s)) != strlen(s)) {
	  perror("write");
	  return false;
   } return true;
}

/** 
 * Wrapper for the write function, given a C string
 * and a number of characters.
 *
 * This function writes len bytes of s to the
 * provided file descriptor.
 * 
 * @param _fd File descriptor on which to write.
 * @param s String to write.
 * @param len Number of bytes of s to write.
 * 
 * @return True upon successful write.
 */
bool read_line::write_with_error(int _fd, const char * s, const size_t & len)
{
   if (write(_fd, s, len) != len) {
	  perror("write");
	  return false;
   } return true;
}

/** 
 * Wrapper for the read function, given a char &
 * in which to store the char and a file descriptor
 * from which we read it.
 * 
 * @param _fd File descriptor from which to read.
 * @param c Character to store the read character.
 * 
 * @return True upon successful read.
 */
bool read_line::read_with_error(int _fd, char & c)
{
   char d; /* temp, for reading */
   if (!read(0, &d, 1)) {
	  perror("read");
	  return false;
   } else return (c = d), true;
}

/** 
 * Wrapper function for getting the width
 * of the terminal using ioctl.
 * 
 * @return Number of columns in the terminal.
 */
size_t read_line::get_term_width()
{
   struct winsize w;
   ioctl(1, TIOCGWINSZ, &w);
   return w.ws_col;
}

/** 
 * @brief Receive enter key
 * 
 * @param _line The current line of text
 * @param input The last input character
 * 
 * @return True if we should break, false to continue.
 */
bool read_line::handle_enter(std::string & _line, char & input)
{
   char ch;
   // Enter was typed
   if (m_buff.size()) {
	  char ch;
	  for (; m_buff.size();) {
		 ch = m_buff.top(); m_buff.pop();
		 _line += ch;
		 if (!write_with_error(1, ch)) return false;
	  }
   } if (!write_with_error(1, input)) return false;
   history_index = m_history.size();
   return true;
}

/** 
 * @brief Handle ctrl + a
 *
 * This is what we do when the user types ctrl+a.
 * Namely, we move the cursor to the beginning of
 * the text entry point.
 * 
 * @param _line Current line of text.
 * 
 * @return False to continue, true otherwise.
 */
bool read_line::handle_ctrl_a(std::string & _line)
{
   if (!_line.size()) return false;

   register size_t term_width = get_term_width();
			
   for (; _line.size();) {
	  m_buff.push(_line.back()); _line.pop_back();
	  /* Next check if we need to go up */
	  /* @todo this does not quite work -- but it's close */
	  if (_line.size() == term_width) {
		 if (!write_with_error(1, "\033[1A\x1b[33;1m$ \x1b[0m")) return false;
		 else if (!write_with_error(1, _line.c_str(), term_width - 3)) return false;

		 for (size_t k = 0; k < term_width - 2; ++k)
			if (!write_with_error(1, "\b", 1)) return true;
	  } else if (!write_with_error(1, "\b", 1)) return false;
   }
}

/** 
 * @brief Handle ctrl + e
 *
 * This is how the shell responds to ctrl + e.
 * In particular, the shell should go to the end
 * of the current text.
 * 
 * @param _line Current text.
 * 
 * @return False to continue, true otherwise.
 */
bool read_line::handle_ctrl_e(std::string & _line)
{
   // Control E
   char ctrle[m_buff.size() + 1];
   size_t len = m_buff.size();
   memset(ctrle, 0, m_buff.size() + 1);

   for (char * d = ctrle; m_buff.size();) {
	  *(d) = m_buff.top(); m_buff.pop();
	  _line += *(d++);
   }

   if (!write_with_error(1, ctrle, len)) return false;
   return true;
}

/** 
 * @brief Handle ctrl + d
 *
 * This is how the shell responds to the
 * delete character. It should remove the
 * highlighted cursor, should one exist.
 * 
 * @param _line Current line of text.
 * 
 * @return True to break, false otherwise.
 */
bool read_line::handle_ctrl_d(std::string & _line)
{
   if (!m_buff.size()) return false;
   if (!_line.size()) {
	  m_buff.pop();
	  std::stack<char> temp = m_buff;
	  for (char d = 0; temp.size();)
		 if (write(1, &(d = (temp.top())), 1)) temp.pop();
	  
	  if (!write(1, " ", 1)) std::cerr<<"WAT.\n";
	  for (int x = m_buff.size() + 1; x -= write(1, "\b", 1););
	  return false;
   } if (m_buff.size()) {
	  // Buffer!
	  std::stack<char> temp = m_buff;
	  temp.pop();
	  for (char d = 0; temp.size(); ) {
		 d = temp.top(); temp.pop();
		 if (!write(1, &d, 1)) {
			perror("write");
			return false;
		 }
	  }
	  char b = ' ';
	  if (!write_with_error(1, " ", 1)) return false;	
	  else if (!write_with_error(1, "\b", 1)) return false;

	  m_buff.pop();
	  /* Move cursor to current position. */
	  for (size_t x = 0; x < m_buff.size(); ++x) if (!write(1, "\b", 1)) return false;;
   } return false;
}
