#ifndef __PATH_T__
#define __PATH_T__
#include <string>
#include <vector>

struct path_t
{
public:
  path_t();
  path_t(std::string _path);

  const std::string & get() { return m_path; }
private:
  std::string m_path;
  std::vector<std::string> m_structure;
};

path_t::path_t(std::string _path)
{
  m_path = _path;

  /**
   * @todo Implement this constructor
   */

  // 1. Get current path

  // 2. Parse through _path rel PWD

  // 3. Move along splitting, follow .. and .

  // 4. Generate new string
}
#endif
