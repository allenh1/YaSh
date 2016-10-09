#include "shell-readline.hpp"

/** 
 * Wrapper for the write function, given a character.
 * 
 * @param _fd File descriptor on which to write.
 * @param c Character to write.
 * 
 * @return True upon successful write.
 */
bool write_with_error(int _fd, char & c) {
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
bool write_with_error(int _fd, const char * s) {
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
bool write_with_error(int _fd, const char * s, const size_t & len) {
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
bool read_with_error(int _fd, char & c) {
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
size_t get_term_width() {
   struct winsize w;
   ioctl(1, TIOCGWINSZ, &w);
   return w.ws_col;
}
