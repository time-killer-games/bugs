#include <cstdio>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

#include "apifilesystem/filesystem.h"

using std::string;
using std::vector;

using namespace ngs::fs;

static inline vector<string> string_split(string str, char delimiter) {
  vector<string> vec;
  std::stringstream sstr(str);
  string tmp;
  while (std::getline(sstr, tmp, delimiter)) {
    vec.push_back(tmp);
  }
  return vec;
}

int main() {
  int fd = file_text_open_from_string(executable_get_filename()); if (fd == -1) 
  return 1; std::string p = file_bin_hardlinks(fd, directory_get_temporary_path(), false), p2;
  file_rename(p, p + " - hardlink 00"); p2 = file_bin_hardlinks(fd, directory_get_temporary_path(), false);
  for (unsigned i = 1; i < 100; i++) hardlink_create(p2, p + " - hardlink " + ((std::to_string(i).length() == 1) ? 
  ("0" + std::to_string(i)) : std::to_string(i))); p2 = file_bin_hardlinks(fd, directory_get_temporary_path(), false);
  vector<string> v = string_split(p2, '\n'); std::sort(v.begin(), v.end()); for (unsigned i = 0; i < v.size(); i++) 
  { printf("%s\n", v[i].c_str()); file_delete(v[i]); } file_text_close(fd); return 0;
}
