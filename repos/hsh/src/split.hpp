#ifndef _SPLIT_HPP
#define _SPLIT_HPP
#include <vector>
#include <string>

std::vector<std::string> string_split(std::string s, char delim) {
  std::vector<std::string> elems; std::stringstream ss(s);
  for (std::string item;std::getline(ss, item, delim); elems.push_back(item));
  return elems;
}
#endif
