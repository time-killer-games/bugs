#include <cstdio>
#include <algorithm>

#include "findhardlinks.hpp"

#if defined(_WIN32)
#include <cwchar>
#endif

using std::string;
using std::vector;
using std::size_t;
#if defined(_WIN32)
using std::wstring;
#endif

namespace {

  /* read, write, append 
  open() permissions. */
  enum {
    FD_RDONLY,
    FD_WRONLY,
    FD_RDWR,
    FD_APPEND,
    FD_RDAP
  };

  #if defined(_WIN32) 
  // UTF-8 support on Windows: string to wstring.
  wstring widen(string str) {
    size_t wchar_count = str.size() + 1; vector<wchar_t> buf(wchar_count);
    return wstring{ buf.data(), (size_t)MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, buf.data(), (int)wchar_count) };
  }

  // UTF-8 support on Windows: wstring to string.
  string narrow(wstring wstr) {
    int nbytes = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), nullptr, 0, nullptr, nullptr); vector<char> buf(nbytes);
    return string{ buf.data(), (size_t)WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), buf.data(), nbytes, nullptr, nullptr) };
  }
  #endif

  // replace all substrings with new substring in string.
  string string_replace_all(string str, string substr, string nstr) {
    size_t pos = 0;
    while ((pos = str.find(substr, pos)) != string::npos) {
      str.replace(pos, substr.length(), nstr);
      pos += nstr.length();
    }
    return str;
  }

  // path without filename spec.
  string filename_path(string fname) {
    #if defined(_WIN32)
    size_t fp = fname.find_last_of("\\/");
    #else
    size_t fp = fname.find_last_of("/");
    #endif
    if (fp == string::npos) return fname;
    return fname.substr(0, fp + 1);
  }

  // path without directory path spec.
  string filename_name(string fname) {
    #if defined(_WIN32)
    size_t fp = fname.find_last_of("\\/");
    #else
    size_t fp = fname.find_last_of("/");
    #endif
    if (fp == string::npos) return fname;
    return fname.substr(fp + 1);
  }

  // get temporary directory.
  string directory_get_temporary_path() {
    std::error_code ec;
    string result = findhardlinks::fs::temp_directory_path(ec).string();
    if (result.back() != '/') result.push_back('/');
    #if defined(_WIN32)
    result = string_replace_all(result, "/", "\\");
    #endif
    return (ec.value() == 0) ? result : "";
  }

  // write string to file descriptor.
  long file_write_string(int fd, string str) {
    char *buffer = str.data();
    #if defined(_WIN32)
    long result = _write(fd, buffer, (unsigned)str.length());
    #else
    long result = write(fd, buffer, (unsigned)str.length());
    #endif
    return result;
  }

  /* create a temporary file with the string contents str.
  filename suffix - replaces XXXXXX in buffer randomly. */
  int file_open_from_string(string str) {
    string fname = directory_get_temporary_path() + "temp.XXXXXX";
    #if defined(_WIN32)
    int fd = -1; wstring wfname = widen(fname); 
    wchar_t *buffer = wfname.data(); if (_wmktemp_s(buffer, wfname.length() + 1)) return -1;
    if (_wsopen_s(&fd, buffer, _O_CREAT | _O_RDWR | _O_WTEXT, _SH_DENYNO, _S_IREAD | _S_IWRITE)) {
      return -1;
    }
    #else
    char *buffer = fname.data();
    int fd = mkstemp(buffer);
    #endif
    if (fd == -1) return -1;
    file_write_string(fd, str);
    #if defined(_WIN32)
    _lseek(fd, 0, SEEK_SET);
    #else
    lseek(fd, 0, SEEK_SET);
    #endif
    return fd;
  }

  // open file descriptor with mode (read, write, append, etc).
  int file_open(string fname, int mode) {
    #if defined(_WIN32)
    wstring wfname = widen(fname);
    FILE *fp = nullptr;
    switch (mode) {
      case  0: { if (!_wfopen_s(&fp, wfname.c_str(), L"rb, ccs=UTF-8" )) break; return -1; }
      case  1: { if (!_wfopen_s(&fp, wfname.c_str(), L"wb, ccs=UTF-8" )) break; return -1; }
      case  2: { if (!_wfopen_s(&fp, wfname.c_str(), L"w+b, ccs=UTF-8")) break; return -1; }
      case  3: { if (!_wfopen_s(&fp, wfname.c_str(), L"ab, ccs=UTF-8" )) break; return -1; }
      case  4: { if (!_wfopen_s(&fp, wfname.c_str(), L"a+b, ccs=UTF-8")) break; return -1; }
      default: return -1;
    }
    if (fp) { int fd = _dup(_fileno(fp));
    fclose(fp); return fd; }
    #else
    FILE *fp = nullptr;
    switch (mode) {
      case  0: { fp = fopen(fname.c_str(), "rb" ); break; }
      case  1: { fp = fopen(fname.c_str(), "wb" ); break; }
      case  2: { fp = fopen(fname.c_str(), "w+b"); break; }
      case  3: { fp = fopen(fname.c_str(), "ab" ); break; }
      case  4: { fp = fopen(fname.c_str(), "a+b"); break; }
      default: return -1;
    }
    if (fp) { int fd = dup(fileno(fp));
    fclose(fp); return fd; }
    #endif
    return -1;
  }

  // close file descriptor.
  int file_close(int fd) {
    #if defined(_WIN32)
    return _close(fd);
    #else
    return close(fd);
    #endif
  }

  // check if regular file (non-directory) exists.
  bool file_exists(string fname) {
    std::error_code ec;
    const findhardlinks::fs::path path = findhardlinks::fs::path(fname);
    return (findhardlinks::fs::exists(path, ec) && ec.value() == 0 && 
      (!findhardlinks::fs::is_directory(path, ec)) && ec.value() == 0);
  }

  // delete file or hardlink.
  bool file_delete(string fname) {
    std::error_code ec;
    if (!file_exists(fname)) return false;
    const findhardlinks::fs::path path = findhardlinks::fs::path(fname);
    return (findhardlinks::fs::remove(path, ec) && ec.value() == 0);
  }

  // check if directory exists.
  bool directory_exists(string dname) {
    std::error_code ec;
    const findhardlinks::fs::path path = findhardlinks::fs::path(dname);
    return (findhardlinks::fs::exists(path, ec) && ec.value() == 0 && 
      findhardlinks::fs::is_directory(path, ec) && ec.value() == 0);
  }

  // create directory recursively.
  bool directory_create(string dname) {
    std::error_code ec;
    const findhardlinks::fs::path path = findhardlinks::fs::path(dname);
    return (findhardlinks::fs::create_directories(path, ec) && ec.value() == 0);
  }

  // rename or move file to name and/or destination.
  bool file_rename(string oldname, string newname) {
    std::error_code ec;
    if (!file_exists(oldname)) return false;
    if (!directory_exists(filename_path(newname)))
      directory_create(filename_path(newname));
    const findhardlinks::fs::path path1 = findhardlinks::fs::path(oldname);
    const findhardlinks::fs::path path2 = findhardlinks::fs::path(newname);
    findhardlinks::fs::rename(path1, path2, ec);
    return (ec.value() == 0);
  }

  // create hardlink.
  bool hardlink_create(string fname, string newname) {
    if (file_exists(fname)) {
      if (!directory_exists(filename_path(newname)))
        directory_create(filename_path(newname));
      #if defined(_WIN32)
      std::error_code ec;
      const findhardlinks::fs::path path1 = findhardlinks::fs::path(fname);
      const findhardlinks::fs::path path2 = findhardlinks::fs::path(newname);
      findhardlinks::fs::create_hard_link(path1, path2, ec);
      return (ec.value() == 0);
      #else
      return (!link(fname.c_str(), newname.c_str()));
      #endif
    }
    return false;
  }

} // anonymous namespace

// our test case:
int main(int argc, char **argv) {
  int fd = file_open_from_string(filename_name(argv[0] ? argv[0] : "(null)")); 

  if (fd == -1) { 
    return 1; // failure
  }

  vector<string> dnames;
  dnames.push_back(directory_get_temporary_path());
  vector<string> p = findhardlinks::findhardlinks(fd, dnames, false);
  /* close file because Windows won't allow us to 
  move/rename it when opened; win32-only issue */
  file_close(fd); 

  if (p.empty()) {
    return 1; // failure
  }

  // rename original file to have hardlink number
  vector<string> p2;
  p2.push_back(p[0] + " - hardlink 00");
  file_rename(p[0], p2[0]); 

  // if file exists, create 99 hardlinks
  if (file_exists(p2[0])) {
    for (unsigned i = 1; i < 100; i++) {
      if (!hardlink_create(p2[0], p[0] + " - hardlink " + ((std::to_string(i).length() == 1) ?  ("0" + std::to_string(i)) : std::to_string(i)))) {
        return 1; // failure
      }
    }
  }

  // open original hardlink again, read-only
  fd = file_open(p2[0], FD_RDONLY);

  if (fd == -1) {
    return 1;
  }

  // get hardlinks
  p2 = findhardlinks::findhardlinks(fd, dnames, false);
  file_close(fd); 

  // sort alphabetically, numerically
  std::sort(p2.begin(), p2.end()); 

  // print hardlink filenames, then delete them.
  for (unsigned i = 0; i < p2.size(); i++) { 
    printf("%s\n", p2[i].c_str()); 
    file_delete(p2[i]); 
  } 

  // done:
  return 0;
}
