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
#include <algorithm>
#include <string>
#include <memory>
#include <vector>
#include <stack>

#include "shell-readline.hpp"

/**
 * Constructor for a read_line.
 *
 * read_line is a function object
 * that handles keyboard input with
 * a series of callback functions.
 *
 * A call to the () operator will
 * read the next line of input from the user.
 * This is done reactively.
 *
 */
read_line::read_line()
{
  /* create history dir */
  mkdir(tilde_expand("~/.cache").c_str(), 0700);
  /**
   * TODO(allenh1): Do we need error checks? Probably not...
   * but we might look into it.
   */
  m_history = std::make_shared<std::vector<std::string>>();
  SimpleCommand::history = m_history;
  /* load current history */
  load_history();
}

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
  }
  return true;
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
  if (static_cast<size_t>(write(_fd, s, strlen(s))) != strlen(s)) {
    perror("write");
    return false;
  }
  return true;
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
  if (static_cast<size_t>(write(_fd, s, len)) != len) {
    perror("write");
    return false;
  }
  return true;
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
bool read_line::read_with_error(const int & _fd, char & c)
{
  char d;   /* temp, for reading */
  if (!read(_fd, &d, 1)) {
    perror("read");
    return false;
  } else {
    return (c = d), true;
  }
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
    for (; m_buff.size(); ) {
      ch = m_buff.top(); m_buff.pop();
      _line += ch;
      if (!write_with_error(1, ch)) {
        return false;
      }
    }
  }
  if (!write_with_error(1, input)) {
    return false;
  }
  history_index = m_history->size();
  return true;
}

/**
 * This is a rather unfortunate issue.
 * it seems that GCC optimizes this in a weird
 * way and screws things up on ARM. So, we
 * place these ugly pragmas here to make
 * GCC stop optimizing and then allow it
 * to resume with the last pragma.
 */
#pragma GCC push_options
#pragma GCC optimize ("O0")
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
  if (!_line.size()) {return false;}

  const size_t term_width = get_term_width();
  for (; _line.size(); ) {
    m_buff.push(_line.back()); _line.pop_back();
    /* Next check if we need to go up */
    /* @todo this does not quite work -- but it's close */
    if (_line.size() == term_width) {
      if (!write_with_error(1, "\033[1A\x1b[33;1m$ \x1b[0m")) {
        return false;
      } else if (!write_with_error(1, _line.c_str(), term_width - 3)) {return false;}

      for (size_t k = 0; k < term_width - 2; ++k) {
        if (!write_with_error(1, "\b", 1)) {
          return true;
        }
      }
    } else if (!write_with_error(1, "\b", 1)) {
      return false;
    }
  }
  return false;
}
#pragma GCC pop_options

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
  auto ctrle = std::shared_ptr<char>(
    new char[m_buff.size() + 1], [](auto s) {delete[] s;});
  size_t len = m_buff.size();
  for (char * d = ctrle.get(); m_buff.size(); ) {
    *(d) = m_buff.top(); m_buff.pop();
    _line += *(d++);
  }
  write_with_error(1, ctrle.get(), len);
  return false;
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
  if (!m_buff.size()) {
      return false;
  }
  if (!_line.size()) {
    m_buff.pop();
    std::stack<char> temp = m_buff;
    for (char d = 0; temp.size(); ) {
      if (write(1, &(d = (temp.top())), 1)) {
        temp.pop();
      }
    }
    if (!write(1, " ", 1)) {
      std::cerr << "WAT.\n";
    }
    for (int x = m_buff.size() + 1; x -= write(1, "\b", 1); ) {
    }
    return false;
  }
  if (m_buff.size()) {
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
    if (!(write_with_error(1, " ", 1) && write_with_error(1, "\b", 1))) {
      return false;
    }
    m_buff.pop();
    /* Move cursor to current position. */
    auto buf = std::shared_ptr<char>(
      new char[m_buff.size()], [](auto s) {delete[] s;});
    memset(buf.get(), '\b', m_buff.size());
    int ret = write(1, buf.get(), m_buff.size());
    if (-1 == ret) {
        perror("write");
        return false;
    }
  }
  return false;
}

/**
 * Handle the tab key.
 *
 * Tab key performs "tab completion."
 * Any file matching the current text
 * is completed as far as it can be.
 * Moreover, there is some binary completion.
 * If a binary in the PATH variable matches
 * the currently entered text, then it
 * is completed in a similar manner.
 *
 * @param _line The current line.
 *
 * @return False to continue, true otherwise.
 */
bool read_line::handle_tab(std::string & _line)
{
  Command::currentSimpleCommand = std::unique_ptr<SimpleCommand>(new SimpleCommand());

  /* Part 1: add a '*' to the end of the stream. */
  std::string _temp;

  std::vector<std::string> _split;
  if (_line.size()) {
    _split = string_split(_line, ' ');
    _temp = tilde_expand(_split.back()) + "*";
  } else {
      _temp = "*";
  }
  auto _complete_me = std::shared_ptr<char>(strndup(_temp.c_str(), _temp.size()), free);
  /* Part 2: Invoke wildcard expand */
  wildcard_expand(_complete_me);

  /**
   * Part 3: If wc_collector.size() <= 1, then complete the tab.
   *         otherwise, run "echo <text>*"
   */
  std::sort(
    Command::currentCommand.wc_collector.begin(), Command::currentCommand.wc_collector.end());
  /* TODO(allenh1): for some reason, wc_collector has no elements upon upstart tab */
  if (Command::currentCommand.wc_collector.size() == 1) {
    /* First check if the line has any spaces! */
    /*     If so, we will wrap in quotes!      */
    bool quote_wrap = false;
    if (Command::currentCommand.wc_collector[0].find(" ") !=
      std::string::npos)
    {
      Command::currentCommand.wc_collector[0].insert(0, "\"");
      quote_wrap = true;
    }
    char ch[_line.size() + 1]; char sp[_line.size() + 1];
    ch[_line.size()] = '\0'; sp[_line.size()] = '\0';
    memset(ch, '\b', _line.size()); memset(sp, ' ', _line.size());
    if (!write_with_error(1, ch, _line.size())) {
      return false;
    }
    if (!write_with_error(1, sp, _line.size())) {
      return false;
    }
    if (!write_with_error(1, ch, _line.size())) {
      return false;
    }
    _line = "";
    for (const auto & x : _split) {
      _line += x + " ";
    }
    _line += Command::currentCommand.wc_collector[0];
    if (quote_wrap) {
      _line = _line + "\"";
    }
    m_current_line_copy = _line;
    Command::currentCommand.wc_collector.clear();
    Command::currentCommand.wc_collector.shrink_to_fit();
  } else if (Command::currentCommand.wc_collector.size() == 0) {
    /* now we check the binaries! */
    const char * _path = getenv("PATH");
    if (nullptr == _path) {
      /* if path isn't set, continue */
      return false;
    }
    std::string path(_path);
    /* part 1: split the path variable into individual dirs */
    std::vector<std::string> _path_dirs = vector_split(path, ':');
    std::vector<std::string> path_dirs;

    /* part 1.5: remove duplicates */
    for (auto && x : _path_dirs) {
      /* @todo it would be nice to not do this */
      bool add = true;
      for (auto && y : path_dirs) {
        if (x == y) {add = false;}
      }
      if (add) {path_dirs.push_back(x);}
    }

    /* part 2: go through the paths, coallate matches */
    for (auto && x : path_dirs) {
      /* add trailing '/' if not already there */
      if (x.back() != '/') {x += '/';}
      /* append _complete_me to current path */
      x += _complete_me.get();
      /* duplicate the string */
      auto _x_cpy = std::shared_ptr<char>(strndup(x.c_str(), x.size()), free);

      /* invoke wildcard_expand */
      wildcard_expand(_x_cpy);
    }
    std::vector<std::string> wc_expanded =
      Command::currentCommand.wc_collector;

    /* part 4: check for a single match */
    if (wc_expanded.size() == 1) {
      /* First check if the line has any spaces! */
      /*     If so, we will wrap in quotes!      */
      bool quote_wrap = false;
      if (Command::currentCommand.wc_collector[0].find(" ") !=
        std::string::npos)
      {
        Command::currentCommand.wc_collector[0].insert(0, "\"");
        quote_wrap = true;
      }
      char ch[_line.size() + 1]; char sp[_line.size() + 1];
      ch[_line.size()] = '\0'; sp[_line.size()] = '\0';
      memset(ch, '\b', _line.size()); memset(sp, ' ', _line.size());
      if (!write_with_error(1, ch, _line.size())) {return false;}
      if (!write_with_error(1, sp, _line.size())) {return false;}
      if (!write_with_error(1, ch, _line.size())) {return false;}
      _line = "";
      for (size_t x = 0; x < _split.size() - 1; _line += _split[x++] + " ") {
      }
      _line += Command::currentCommand.wc_collector[0];
      if (quote_wrap) {_line = _line + "\"";}
      m_current_line_copy = _line;
      Command::currentCommand.wc_collector.clear();
      Command::currentCommand.wc_collector.shrink_to_fit();
    } else {     /* part 5: handle multiple matches */
      /* @todo appropriately handle multiple matches */
      return false;
    }

    /* free resources and print */
    write_with_error(1, _line.c_str(), _line.size());
    return false;
  } else {
    std::cout << std::endl;
    std::vector<std::string> _wcd = Command::currentCommand.wc_collector;
    std::vector<std::string> cpyd = Command::currentCommand.wc_collector;
    std::string longest_common((longest_substring(cpyd)));
    if (_wcd.size()) {
      printEvenly(_wcd); auto _echo = std::shared_ptr<char>(strdup("echo"), free);
      Command::currentSimpleCommand->insertArgument(_echo);
      Command::currentCommand.wc_collector.clear();
      Command::currentCommand.wc_collector.shrink_to_fit();
      Command::currentCommand.execute();

      /**
       * Now we add the largest substring of
       * the above to the current string so
       * that the tab completion isn't "butts."
       *
       * ~ Ana-Gabriel Perez
       */

      if (longest_common.size()) {
        char * to_add = strndup(longest_common.c_str() + strlen(_complete_me.get()) - 1,
            longest_common.size() - strlen(_complete_me.get()) + 1);
        _line += to_add; free(to_add);
        m_current_line_copy = _line;
      }
    } else {
      return false;
    }
  }
  write_with_error(1, _line.c_str(), _line.size());
  return false;
}

/**
 * Handle ctrl + arrow key
 *
 * This is the argument hopping feature.
 *
 * @param _line current line
 *
 * @return False if continue.
 */
bool read_line::handle_ctrl_arrow(std::string & _line)
{
  char ch3[4]; memset(ch3, 0, 4 * sizeof(char));
  if (read(0, ch3, 3) != 3) {
    perror("read");
    return false;
  }

  if (ch3[0] == 59 && ch3[1] == 53 && ch3[2] == 67) {
    /* ctrl + right arrow */
    if (!(m_buff.size())) {return false;}
    for (; (m_buff.size() &&
      ((_line += m_buff.top(), m_buff.pop(), _line.back()) != ' ') &&
      (write(1, &_line.back(), 1) == 1)) ||
      ((_line.back() == ' ') ? !(write(1, " ", 1)) : 0); )
    {
    }
  } else if (ch3[0] == 59 && ch3[1] == 53 && ch3[2] == 68) {
    /* ctrl + left arrow */
    if (!_line.size()) {return false;}
    for (; (_line.size() &&
      ((m_buff.push(_line.back()),
      _line.pop_back(), m_buff.top()) != ' ') &&
      (write(1, "\b", 1) == 1)) ||
      ((m_buff.top() == ' ') ? !(write(1, "\b", 1)) : 0); )
    {
    }
  }
  return false;
}

/**
 * Handle ctrl + k
 *
 * ctrl + k clears the remaining
 * parts of the line.
 *
 * @param _line Current line.
 *
 * @return False upon error.
 */
bool read_line::handle_ctrl_k()
{
  if (!m_buff.size()) {return false;}
  size_t count = m_buff.size() + 1;
  /* Clear the stack. On its own thread. */
  std::thread stack_killer([this]() {
      for (; m_buff.size(); m_buff.pop()) {
      }
    }); stack_killer.detach();

  const auto deleter = [](auto s) {delete[] s;};
  auto spaces = std::shared_ptr<char>(new char[count + 1], deleter);
  auto bspaces = std::shared_ptr<char>(new char[count + 1], deleter);

  memset(spaces.get(), ' ', count); *(spaces.get() + count) = '\0';
  memset(bspaces.get(), '\b', count); *(bspaces.get() + count) = '\0';

  if (!write_with_error(1, spaces.get(), count)) {
    return false;
  } else if (!write_with_error(1, bspaces.get(), count)) {return false;}
  return false;
}

/**
 * Handle the backspace key.
 *
 * @param _line Current line of text.
 *
 * @return False to continue, true to break.
 */
bool read_line::handle_backspace(std::string & _line)
{
  /* backspace */
  if (!_line.size()) {return false;}

  if (m_buff.size()) {
    // Buffer!
    if (!write_with_error(1, "\b", 1)) {return false;}
    _line.pop_back();
    std::stack<char> temp = m_buff;
    for (char d = 0; temp.size(); ) {
      d = temp.top(); temp.pop();
      if (!write_with_error(1, d)) {return false;}
    }

    if (!write_with_error(1, " ", 1)) {return false;} else if (!write_with_error(1, "\b", 1)) {
      return false;
    }

    /* get terminal width */
    size_t term_width = get_term_width();
    size_t line_size = _line.size() + m_buff.size() + 2;
    for (size_t x = 0; x < m_buff.size(); ++x, --line_size) {
      /* if the cursor is at the end of a line, print line up */
      if (line_size && (line_size % term_width == 0)) {
        /* need to go up a line */
        const size_t p_len = strlen("\033[1A\x1b[33;1m$ \x1b[0m");

        /* now we print the string */
        if (line_size == term_width) {
          /**
           * @todo Make sure you print the correct prompt!
           * this currently is not going to print that prompt.
           */
          if (!write_with_error(1, "\033[1A\x1b[33;1m$ \x1b[0m", p_len)) {
            return false;
          } else if (!write_with_error(1, _line.c_str(), _line.size())) {return false;} else {
            break;
          }
        } else {
          if (!write_with_error(1, "\033[1A \b")) {
            return false;
          } else if (!write_with_error(1, _line.c_str() + (_line.size() - line_size),
            _line.size() - line_size)) {return false;}
        }
      } else if (!write_with_error(1, "\b", 1)) {return false;}
    }
  } else {
    if (!write_with_error(1, "\b", 1)) {return false;} else if (!write_with_error(1, " ", 1)) {
      return false;
    } else if (!write_with_error(1, "\b", 1)) {return false;}
    _line.pop_back();
  }
  if (((size_t) history_index == m_history->size()) &&
    m_current_line_copy.size())
  {
    m_current_line_copy.pop_back();
  }
  return false;
}

bool read_line::handle_ctrl_del()
{
  char ch4, ch5;
  /* read the 53 and the 126 char */
  if (!read_with_error(0, ch4) ||
    !read_with_error(0, ch5)) {return false;}
  /* if not 126, go away */
  if (ch4 != 53 || ch5 != 126) {return false;}

  /* clear remaining output */
  size_t len = m_buff.size();
  char space[len], bspace[len];

  std::thread space_out([&space, len]() {
      memset(space, ' ', len);
    });
  std::thread black_out([&bspace, len]() {
      memset(bspace, '\b', len);
    });
  space_out.join(); black_out.join();

  if (!write_with_error(1, space, len)) {
    return false;
  } else if (!write_with_error(1, bspace, len)) {
    return false;
  }
  /* pop off stack until a space, or the end */
  for (char c = 'a'; c != ' ' && m_buff.size(); c = m_buff.top(), m_buff.pop()) {
  }
  if (!m_buff.size()) {
    return false;
  }
  len = m_buff.size();
  /* print out the residual buffer */
  auto bspace2 = std::shared_ptr<char>(
    new char[m_buff.size()], [](auto s) {delete[] s;});
  std::stack<char> temp = m_buff;
  char c = temp.top(); temp.pop();
  for (; write_with_error(1, c) && temp.size(); ) {
    c = temp.top();
    temp.pop();
  }
  memset(bspace2.get(), '\b', len);
  return !write_with_error(1, bspace2.get(), len);
}

/**
 * Handle the delete button.
 *
 * @param _line Current line of text.
 *
 * @return False to continue, true to break.
 */
bool read_line::handle_delete(std::string & _line)
{
  if (!m_buff.size()) {return false;}
  if (!_line.size()) {
    m_buff.pop();
    std::stack<char> temp = m_buff;
    for (char d = 0; temp.size(); ) {
      if (write_with_error(1, &(d = (temp.top())), 1)) {temp.pop();} else {return false;}
    }
    if (!write(1, " ", 1)) {std::cerr << "WAT.\n";}
    for (int x = m_buff.size() + 1; x-- && write_with_error(1, "\b", 1); ) {
    }
    return false;
  }

  if (m_buff.size()) {
    // Buffer!
    std::stack<char> temp = m_buff;
    temp.pop();
    for (char d = 0; temp.size(); ) {
      d = temp.top(); temp.pop();
      if (!write_with_error(1, d)) {return false;}
    }
    if (!write_with_error(1, " ", 1)) {return false;} else if (!write_with_error(1, "\b", 1)) {
      return false;
    }
    m_buff.pop();
    // Move cursor to current position.
    for (size_t x = 0; x < m_buff.size(); ++x) {
      if (!write_with_error(1, "\b", 1)) {return false;}
    }
  } else {return false;}
  if ((size_t) history_index == m_history->size()) {m_current_line_copy.pop_back();}
  return false;
}

/**
 * Handle the "!!" or "!-<n>" keys.
 *
 * @param _line Current line of text.
 *
 * @return False to continue, true to break.
 */
bool read_line::handle_bang(std::string & _line)
{
  if (!write_with_error(0, "!", 1)) {return false;}

  /* Check for "!!" and "!-<n>" */
  if (!m_history->size()) {
    _line += "!";
    return false;
  }
  char ch1;
  /* read next char from stdin */
  if (!read_with_error(0, ch1)) {return false;}

  /* print newline and stop looping */
  if ((ch1 == '\n') && !write_with_error(1, "\n", 1)) {return true;} else if (ch1 == '!') {
    // "!!" = run prior command
    if (!write_with_error(1, "!", 1)) {return false;}

    _line += m_history->at(m_history->size() - 1);
    _line.pop_back();
    m_show_line = true;
    return false;
  } else if (ch1 == '-') {
    if (!write_with_error(1, "-", 1)) {return false;}

    auto && is_digit = [](char b) {
        return '0' <= b && b <= '9';
      };
    /* "!-<n>" = run what I did n commands ago. */
    auto buff = std::shared_ptr<char>(
      new char[20], [](auto s) {delete[] s;});
    char * b;
    for (b = buff.get(); read(0, b, 1) && write(1, b, 1) && is_digit(*b); *(++b + 1) = 0) {
    }
    int n = atoi(buff.get()); bool run_cmd = false;
    if (*b == '\n') {run_cmd = true;}
    if (n > 0) {
      int _idx = m_history->size() - n;
      _line += m_history->at((_idx >= 0) ? _idx : 0);
      _line.pop_back();
      m_show_line = true;
      if (run_cmd) {
        if (m_buff.size()) {
          char ch;
          for (; m_buff.size(); ) {
            ch = m_buff.top(); m_buff.pop();
            _line += ch;
            if (!write_with_error(1, ch)) {return false;}
          }
        }
        history_index = m_history->size();
        return true;
      }
    }
  } else {
    if (!write_with_error(1, ch1)) {return false;}
    _line += "!"; _line += ch1;
    return false;
  }
  return false;
}

/**
 * Handle the up arrow.
 *
 * @param _line Current line of text.
 *
 * @return False to continue, true to break.
 */
bool read_line::handle_up_arrow(std::string & _line)
{
  /* check for the reverse search mode variable */
  const char * rev_search = getenv("REV_SEARCH_MODE");
  bool search_mode = false;
  search_mode =
    rev_search && !strcmp(rev_search, "UP_ARROW") && _line.size();

  std::vector<std::string> * hist;
  ssize_t * index = &history_index;
  if (search_mode) {
    if (_line != search_str) {
      m_rev_search.clear();
      m_rev_search.shrink_to_fit();
      hist = &m_rev_search;
      search_str = _line;
      std::copy_if(
        m_history->begin(), m_history->end(),
        std::back_inserter(*hist),
        [_line](const auto & s) {
          return (s.size() >= _line.size()) &&
          s.substr(0, _line.size()) == _line;
        });
      search_index = hist->size();
    } else {hist = &m_rev_search;}
    index = &search_index;
  } else {hist = m_history.get();}

  /* check if we can go up */
  if (!hist->size()) {return false;}

  /* clear input so far */
  std::string ending = "";
  for (; m_buff.size(); m_buff.pop()) {
    ending += m_buff.top();
  }
  if (!write_with_error(1, ending.c_str(), ending.size())) {return false;}
  _line += ending;
  char ch[_line.size() + 1]; char sp[_line.size() + 1];
  memset(ch, '\b', _line.size()); memset(sp, ' ', _line.size());

  if (!write_with_error(1, ch, _line.size())) {
    return false;
  } else if (!write_with_error(1, sp, _line.size())) {
    return false;
  } else if (!write_with_error(1, ch, _line.size())) {return false;}

  /* stay in bounds */
  if ((size_t) *index == hist->size()) {--(*index);}

  /* only decrement if we are going beyond the first command (duh) */
  _line = hist->at(*index);
  if (*index) {*index = *index - 1;}

  /* print the line */
  if (_line.size()) {_line.pop_back();}
  if (!write_with_error(1, _line.c_str(), _line.size()) || !rev_search) {return false;}
  if (!search_mode) {search_str = "";}
  size_t len_diff = 0;
  for (; m_buff.size(); ) {
    m_buff.pop();
  }

  /* fill the buffer */
  for (; _line != search_str; ++len_diff) {
    m_buff.push(_line.back());
    _line.pop_back();
  }
  /* iterate back to search prefix */
  auto ch2 = std::shared_ptr<char>(
    new char[len_diff], [](auto s) {delete[] s;});
  memset(ch2.get(), '\b', len_diff);
  write_with_error(1, ch2.get(), len_diff);
  return false;
}

/**
 * Handle the down arrow.
 *
 * @param _line Current line of text.
 *
 * @return False to continue, true to break.
 */
bool read_line::handle_down_arrow(std::string & _line)
{
  /* check for the reverse search mode variable */
  const char * rev_search = getenv("REV_SEARCH_MODE");
  bool search_mode = false;
  search_mode =
    rev_search && !strcmp(rev_search, "UP_ARROW") && _line.size();

  std::vector<std::string> * hist = m_history.get();
  ssize_t * index = &history_index;
  if (search_mode) {
    if (_line != search_str) {
      m_rev_search.clear();
      m_rev_search.shrink_to_fit();
      hist = &m_rev_search;
      search_str = _line;
      std::copy_if(
        m_history->begin(), m_history->end(),
        std::back_inserter(*hist),
        [_line](const auto & s) {
          return (s.size() >= _line.size()) &&
          s.substr(0, _line.size()) == _line;
        });
      search_index = hist->size();
    } else {hist = &m_rev_search;}
    index = &search_index;
  }

  if (static_cast<size_t>(*index) == hist->size()) {return false;}


  /* clear input so far */
  std::string ending = "";
  for (; m_buff.size(); m_buff.pop()) {
    ending += m_buff.top();
  }
  if (!write_with_error(1, ending.c_str(), ending.size())) {return false;}
  _line += ending;
  char ch[_line.size() + 1]; char sp[_line.size() + 1];
  memset(ch, '\b', _line.size()); memset(sp, ' ', _line.size());

  if (!write_with_error(1, ch, _line.size())) {
    return false;
  } else if (!write_with_error(1, sp, _line.size())) {
    return false;
  } else if (!write_with_error(1, ch, _line.size())) {return false;}

  /* don't increment past the last command */
  if (static_cast<size_t>((*index)) != hist->size() - 1) {
    (*index)++;
    _line = hist->at(*index);
    _line.pop_back();
  }

  /* print the line */
  if (!write_with_error(1, _line.c_str(), _line.size()) || !rev_search) {return false;}
  if (!search_mode) {search_str = "";}
  size_t len_diff = 0;
  /* clear the buffer */
  for (; m_buff.size(); ) {
    m_buff.pop();
  }

  /* fill the buffer */
  for (; _line != search_str; ++len_diff) {
    m_buff.push(_line.back());
    _line.pop_back();
  }
  /* iterate back to search prefix */
  auto ch2 = std::shared_ptr<char>(
    new char[len_diff], [](auto s) {delete[] s;});
  memset(ch2.get(), '\b', len_diff);
  write_with_error(1, ch2.get(), len_diff);
  return false;
}

/**
 * Handle the right arrow key.
 *
 * @param _line Current line of text.
 *
 * @return False to continue, true to break.
 */
bool read_line::handle_right_arrow(std::string & _line)
{
  /* Right Arrow Key */
  if (!m_buff.size()) {return false;}

  char wrt = m_buff.top();
  if (!write(1, &wrt, 1)) {
    perror("write");
    return false;
  }
  _line += wrt; m_buff.pop();
  return false;
}

/**
 * Handle the left arrow key.
 *
 * @param _line Current line of text.
 *
 * @return False to continue, true to break.
 */
bool read_line::handle_left_arrow(std::string & _line)
{
  if (!_line.size()) {return false;}
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
      return false;
    } else if (!write(1, _line.c_str(), term_width - 2)) {
      perror("write");
      return false;
    }
  } else if (!((_line.size() + 2) % (term_width))) {
    /* This case is for more than one line of backtrack */
    if (!write(1, "\033[1A \b", strlen("\033[1A \b"))) {
      perror("write");
    } else {
      auto ret =
        !write(
        1, _line.c_str() - 2 + (term_width * ((_line.size() - 2) + term_width)), term_width);
      if (term_width != ret) {
        perror("write");
        return false;
      }
    }
  }
  m_buff.push(_line.back());
  char bsp = '\b';
  if (!write(1, &bsp, 1)) {
    perror("write");
    return false;
  }
  _line.pop_back();
  return false;
}

void read_line::load_history()
{
  /* load history from ~/.cache/yash-history */
  std::ifstream history_file(tilde_expand("~/.cache/yash_history"));
  std::string _line;
  for (; std::getline(history_file, _line); history_index++, m_history->push_back(_line + "\n")) {
  }
}
