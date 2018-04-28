// Copyright 2016 Hunter L. Allen
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include <utility>
#include <memory>
#include <string>
#include <vector>

#include "shell-utils.hpp"

/**
 * Returns the longest string common to many strings.
 *
 * @param _vct Vector of strings.
 *
 * @return The longest substring common to all strings.
 */
std::string longest_substring(const std::vector<std::string> & _vct)
{
  if (!_vct.size()) {
    return std::string("");                       /* return an empty string */
  }
  for (size_t len = 0; len < _vct[0].size(); ++len) {
    char c = _vct[0][len];

    for (size_t x = 1; x < _vct.size(); ++x) {
      if (len >= _vct[x].size() || _vct[x][len] != c) {
        return _vct[x].substr(0, len);
      }
    }
  }
  return _vct[0];
}

/**
 * Returns the length of the longest string
 * in a vector of strings.
 *
 * @param _vct Vector of strings.
 *
 * @return Length of the longest element of the vector.
 */
size_t size_of_longest(const std::vector<std::string> & _vct)
{
  size_t max = 0;
  for (auto && x : _vct) {
    max = (x.size() > max) ? x.size() : max;
  }
  return max == 0 ? 1 : max;
}

/**
 * Evenly spaces a vector and prints
 * it to the screen. Used for things
 * like tab completion output.
 *
 * @param _vct
 */
void printEvenly(std::vector<std::string> & _vct)
{
  size_t longst = size_of_longest(_vct); std::string * y;
  struct winsize w; ioctl(1, TIOCGWINSZ, &w);

  /* If the length is too long, print on its own line */
  if (longst >= w.ws_col) {
    for (auto && x : _vct) {
      std::cout << x << std::endl;
    }
    return;
  }

  /* Otherwise, print the strings evenly. */
  for (; longst < w.ws_col && (w.ws_col % longst); ++longst) {
  }
  for (size_t x = 0; x < _vct.size(); ) {
    for (size_t width = 0; width != w.ws_col; width += longst) {
      if (x == _vct.size()) {break;}
      y = new std::string(_vct[x++]);
      for (; y->size() < longst; *y += " ") {
      }
      std::cout << *y; delete y;
    }
    std::cout << std::endl;
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
  if (substr[0] == '~') {
    std::string user = substr.substr(1, substr.size());
    if (user.size() > 0) {
      passwd * _passwd = new passwd();
      auto at_exit = make_scope_exit(
        [_passwd]() {
            delete _passwd;
          });
      const size_t len = 1024;
      auto buff = std::shared_ptr<char>(new char[len], [](auto s) {delete[] s;});
      int ret = getpwnam_r(user.c_str(), _passwd, buff.get(), len, &_passwd);
      if (ret || nullptr == _passwd) {
        /* user wasn't found. */
        return input;
      }
      std::string _home = _passwd->pw_dir;
      input.replace(0, substr.size(), _home);
      return input;
    } else {
      /* Case current user (that is, the usual case). */
      passwd * _passwd = new passwd();
      auto at_exit = make_scope_exit(
        [_passwd]() {
            delete _passwd;
          });
      const size_t len = 1024;
      auto buff = std::shared_ptr<char>(new char[len], [](auto s) {delete[] s;});
      int ret = getpwuid_r(getuid(), _passwd, buff.get(), len, &_passwd);
      if (ret || nullptr == _passwd) {
        return input;
      }
      input.replace(0, substr.size(), _passwd->pw_dir);
      return input;
    }
  }
  return input;
}

std::string get_username()
{
  passwd * _passwd = new passwd();
  auto at_exit = make_scope_exit(
    [_passwd]() {
        delete _passwd;
      });
  const size_t len = 1024;
  auto buff = std::shared_ptr<char>(new char[len], [](auto s) {delete[] s;});
  int ret = getpwuid_r(getuid(), _passwd, buff.get(), len, &_passwd);
  if (ret || nullptr == _passwd) {
    return std::string("<unknown user>");
  }
  return std::string(_passwd->pw_name);
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
std::string replace(std::string str, const char * sb, const char * rep)
{
  std::string sub = std::string(sb);
  if (auto pos = str.find(rep); pos != std::string::npos) {
    std::string p1 = str.substr(0, pos - 1);
    std::string p2 = str.substr(pos + std::string(sub).size() - 1);
    std::string tStr = p1 + rep + p2;
    return tStr;
  }
  return str;
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
  auto t = std::shared_ptr<char>(
    reinterpret_cast<char *>(calloc(1024, sizeof(char))), free);
  char * temp = t.get();
  int index;
  for (index = 0; static_cast<size_t>(str - s.c_str()) < s.size(); ++str) {
    // aight. Let's just do it.
    if (*str == '$') {
      // begin expansion
      if (*(++str) != '{') {continue;}
      // @todo maybe work without braces.
      auto t2 = std::shared_ptr<char>(
        reinterpret_cast<char *>(calloc(s.size(), sizeof(char))),
        free);
      ++str;
      char * temp2 = t2.get();
      for (char * tmp = temp2; *str && *str != '}'; *(tmp++) = *(str++)) {
      }
      if (*str == '}') {
        ++str;
        const char * out = getenv(temp2);
        if (nullptr == out) {
          continue;
        }
        for (const char * t = out; *t; ++t) {
          temp[index++] = *t;
        }
      }
    }            // if not a variable, don't expand.
    temp[index++] = *str;
  }
  return std::string(temp);
}

/**
 * Function to change a directory into
 * the provided argument.
 *
 * @param s Directory in which to change.
 *
 * @return True upon successful change.
 */
bool changedir(std::string & s)
{
  /* verify that the directory exists */
  DIR * _dir;

  if (!s.empty() && (_dir = opendir(s.c_str()))) {
    /* directory is there */
    closedir(_dir);
  } else if (!s.empty() && errno == ENOENT) {
    /* directory doesn't exist! */
    std::cerr << u8"¯\\_(ツ)_/¯" << std::endl;
    std::cerr << "No such file or directory" << std::endl;
    return false;
  } else if (!s.empty()) {
    /* cd failed because... ¯\_(ツ)_/¯ */
    std::cerr << u8"¯\\_(ツ)_/¯" << std::endl;
    return false;
  }

  if (*s.c_str() && *s.c_str() != '/') {
    // we need to append the current directory.
    for (; s.back() == '/'; s.pop_back()) {
    }
    s = std::string(getenv("PWD")) + "/" + s;
  } else if (!*s.c_str()) {
    for (; s.back() == '/'; s.pop_back()) {
    }
    passwd * _passwd = new passwd();
    auto at_exit = make_scope_exit(
      [_passwd]() {
          delete _passwd;
        });
    const size_t len = 1024;
    auto buff = std::shared_ptr<char>(new char[len], [](auto s) {delete[] s;});
    int ret = getpwuid_r(getuid(), _passwd, buff.get(), len, &_passwd);
    if (ret || nullptr == _passwd) {
      return true;
    }
    std::string user_home = _passwd->pw_dir;
    return chdir(user_home.c_str()) == 0;
  }
  for (; *s.c_str() != '/' && s.back() == '/'; s.pop_back()) {
  }
  return chdir(s.c_str()) == 0;
}

bool is_directory(std::string str)
{
  struct stat s;
  return !stat(str.c_str(), &s) && s.st_mode & S_IFDIR;
}

std::vector<std::string> vector_split(std::string s, char delim)
{
  std::vector<std::string> elems; std::stringstream ss(s);
  std::string item;
  for (; std::getline(ss, item, delim); elems.push_back(std::move(item))) {
  }
  return elems;
}


timeval operator-(timeval & t1, timeval & t2)
{
  /* Perform the carry for the later subtraction by updating y. */
  if (t1.tv_usec < t2.tv_usec) {
    int nsec = (t2.tv_usec - t1.tv_usec) / 1000000 + 1;
    t2.tv_usec -= 1000000 * nsec;
    t2.tv_sec += nsec;
  }
  if (t1.tv_usec - t2.tv_usec > 1000000) {
    int nsec = (t1.tv_usec - t2.tv_usec) / 1000000;
    t2.tv_usec += 1000000 * nsec;
    t2.tv_sec -= nsec;
  }

  /**
   * Compute the time remaining to wait.
   * tv_usec is certainly positive.
   */
  timeval result;
  result.tv_sec = t1.tv_sec - t2.tv_sec;
  result.tv_usec = t1.tv_usec - t2.tv_usec;

  return result;
}

/* This is from bash */
timeval & difftimeval(
  timeval & d,
  timeval & t1,
  timeval & t2)
{
  d.tv_sec = t2.tv_sec - t1.tv_sec;
  d.tv_usec = t2.tv_usec - t1.tv_usec;

  if (d.tv_usec < 0) {
    d.tv_usec += 1000000;
    d.tv_sec -= 1;

    if (d.tv_sec < 0) {d.tv_sec = d.tv_usec = 0;}
  }
  return d;
}

/* also from bash */
timeval & addtimeval(
  timeval & d,
  timeval & t1,
  timeval & t2)
{
  d.tv_sec = t1.tv_sec + t2.tv_sec;
  d.tv_usec = t1.tv_usec + t2.tv_usec;

  if (d.tv_usec >= 1000000) {
    d.tv_usec -= 1000000;
    d.tv_sec += 1;
  }
  return d;
}

/* more things I stole from bash */
void timeval_to_secs(
  timeval & tvp,
  time_t & sp,
  int & sfp)
{
  int rest;

  sp = tvp.tv_sec;

  sfp = tvp.tv_usec % 1000000;       /* pretty much a no-op */
  rest = sfp % 1000;
  sfp = (sfp * 1000) / 1000000;

  if (rest >= 500) {sfp += 1;}

  /* Sanity check */
  if (sfp >= 1000) {
    sp += 1;
    sfp -= 1000;
  }
}
