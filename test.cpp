#include <sys/types.h>
#include <sys/stat.h>
#if defined(_WIN32) 
#include <windows.h>
#endif

#include "apiprocess/process.h"
#include "apifilesystem/filesystem.h"

using namespace ngs::proc;
using namespace ngs::fs;

int main() {
  int fd = file_text_open_from_string(executable_get_filename()); if (fd == -1) return 1;
  std::string p = file_bin_pathname(fd, ngs::fs::directory_get_temporary_path(), false); printf("%s\n", p.c_str());
  file_text_close(fd); if (file_exists(p)) { file_delete(p); return 0; } return 1;
}
