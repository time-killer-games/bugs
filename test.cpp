#include <sys/types.h>
#include <sys/stat.h>
#if defined(_WIN32) 
#include <windows.h>
#include <io.h>
#endif

#include "apiprocess/process.h"
#include "apifilesystem/filesystem.h"

using namespace ngs::proc;
using namespace ngs::fs;

int test() {
  int fd = file_text_open_from_string(executable_get_filename());
  if (fd == -1) return 1;
  std::string p = file_bin_pathname(fd);
  #if defined(_WIN32)
  FILE_ID_INFO info2;
  if (GetFileInformationByHandleEx((HANDLE)_get_osfhandle(fd), FileIdInfo, &info2, sizeof(info2))) {
    printf("comm: %s\npath: %s\nfd: %d\ninode: %llu\ndev: %llu\n", 
      file_text_readln(fd).c_str(), p.c_str(), fd, info2.FileId, info2.VolumeSerialNumber);
  #else
  struct stat info = { 0 }; 
  if (!fstat(fd, &info)) {
    printf("comm: %s\npath: %s\nfd: %d\ninode: %llu\ndev: %d\n", 
      file_text_readln(fd).c_str(), p.c_str(), fd, info.st_ino, info.st_dev);
  #endif
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
