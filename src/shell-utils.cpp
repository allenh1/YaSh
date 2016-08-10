#include "shell-utils.hpp"

/** 
 * Returns the longest string common to many strings.
 * 
 * @param _vct Vector of strings.
 * 
 * @return The longest substring common to all strings.
 */
std::string longest_substring(const std::vector<std::string> & _vct) {
  if (!_vct.size()) return std::string(""); /* return an empty string */

  for (size_t len = 0; len < _vct[0].size(); ++len) {
	register char c = _vct[0][len];

	for (size_t x = 1; x < _vct.size(); ++x) {
	  if (len >= _vct[x].size() || _vct[x][len] != c) {
		return _vct[x].substr(0, len);
	  }
	}
  } return _vct[0];
}

/** 
 * Returns the length of the longest string
 * in a vector of strings.
 * 
 * @param _vct Vector of strings.
 * 
 * @return Length of the longest element of the vector.
 */
size_t size_of_longest(const std::vector<std::string> & _vct) {
  size_t max = 0;
  for (auto && x : _vct) max = (x.size() > max) ? x.size() : max;
  return max;
}

/** 
 * Evenly spaces a vector and prints
 * it to the screen. Used for things
 * like tab completion output.
 * 
 * @param _vct 
 */
void printEvenly(std::vector<std::string> & _vct) {
  size_t longst = size_of_longest(_vct); std::string * y;
  struct winsize w; ioctl(1, TIOCGWINSZ, &w);

  /* If the length is too long, print on its own line */
  if (longst >= w.ws_col) {
	for (auto && x : _vct) std::cout<<x<<std::endl;
	return;
  }

  /* Otherwise, print the strings evenly. */
  for(; longst < w.ws_col && (w.ws_col % longst); ++longst);
  int inc = _vct.size() / longst;
  for (size_t x = 0; x < _vct.size();) {
	for (size_t width = 0; width != w.ws_col; width += longst) {
	  if (x == _vct.size()) break;
	  y = new std::string(_vct[x++]);
	  for(;y->size() < longst; *y += " ");
	  std::cout<<*y; delete y;
	} std::cout<<std::endl;
  }
}

/** 
 * Tilde expansion!
 *
 * ~/ => /home/${USER}
 *
 * ~USER/ => (home of user)
 * 
 * @param input string to tilde expand.
 * 
 * @return A string with the expansion
 */
std::string tilde_expand(std::string input)
{
  std::string substr = input.substr(0, input.find_first_of('/'));
  if (*substr.c_str() == '~') {
	std::string user = substr.substr(1, substr.size());
	if (user.size() > 0) {
	  passwd * _passwd = getpwnam(user.c_str());
	  std::string _home = _passwd->pw_dir;
	  input.replace(0, substr.size(), _home);
	  return input;
	} else {
	  // Case current user (that is, the usual case).
	  passwd * _passwd = getpwuid(getuid());
	  std::string user_home = _passwd->pw_dir;
	  input.replace(0, substr.size(), user_home);
	  return input;
	}
  }
  return input;
}

/** 
 * String replace function.
 * 
 * @param str String on which we operate
 * @param sb Thing to replace
 * @param rep Thing with which we replace sb.
 * 
 * @return String with replacement.
 */
inline std::string replace(std::string str, const char * sb, const char * rep)
{
  std::string sub = std::string(sb);
  size_t pos = str.find(rep);
  if (pos != std::string::npos) {
	std::string p1 = str.substr(0, pos - 1);
	std::string p2 = str.substr(pos + std::string(sub).size() - 1);
	std::string tStr = p1 + rep + p2;
	return tStr;
  } else return str;
}

/** 
 * Environment variable expansion.
 * 
 * @param s String to expand.
 * 
 * @return Expanded version of s.
 */
std::string env_expand(std::string s)
{
  const char * str = s.c_str();
  char * temp = (char*) calloc(1024, sizeof(char));
  int index;
  for (index = 0; *str; ++str) {
	// aight. Let's just do it.
	if (*str == '$') {
	  // begin expansion
	  if (*(++str) != '{') continue;
	  // @todo maybe work without braces.
	  char * temp2 = (char*) calloc(s.size(), sizeof(char)); ++str;
	  for (char * tmp = temp2; *str && *str != '}'; *(tmp++) = *(str++));
	  if (*str == '}') {
		++str; char * out = getenv(temp2);
		if (out == NULL) continue;;
		for (char * t = out; *t; temp[index++] = *(t++));
	  } delete[] temp2;
	}// if not a variable, don't expand.
	temp[index++] = *str;
  } std::string ret = std::string((char*)temp);
  free(temp);
  return ret;
}
