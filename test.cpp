#include <sys/stat.h>

#include "apiprocess/process.h"
#include "apifilesystem/filesystem.h"

using namespace ngs::proc;
using namespace ngs::fs;

int test() {
  int fd = file_text_open_from_string(executable_get_filename());
  if (fd == -1) return 1; 
  std::string p = file_bin_pathname(fd); 
  struct stat sb = { 0 }; 
  if (!fstat(fd, &sb)) {
    printf("comm: %s\npath: %s\nfd: %d\ninode: %llu\ndev: %d\n", 
      file_text_readln(fd).c_str(), p.c_str(), fd, sb.st_ino, sb.st_dev);
  } 
  file_text_close(fd); 
  if (file_exists(p)) {
    file_delete(p);
    return 0;
  }
  return 1;
}

int main() {
  for (unsigned i = 0; i < 10; i++) {
    if (test()) return 1;
  }
  return 0;
}
