#include <string>
#include <vector>

#if !defined(USE_GHC_FILESYSTEM)
#include <filesystem>
#endif

#include <climits>
#include <cstdlib>
#include <cstring>
#if defined(_WIN32)
#include <cwchar>
#endif

#if defined(USE_GHC_FILESYSTEM)
#include "filesystem.hpp"
#endif

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#if defined(_WIN32) 
#include <windows.h>
#include <share.h>
#include <io.h>
#else
#include <unistd.h>
#endif

namespace findhardlinks {

  #if defined(USE_GHC_FILESYSTEM)
  namespace fs = ghc::filesystem;
  #else
  namespace fs = std::filesystem;
  #endif

  namespace {

    inline void message_pump() {
      #if defined(_WIN32) 
      MSG msg; while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
      #endif
    }

    #if defined(_WIN32) 
    inline std::wstring widen(std::string str) {
      std::size_t wchar_count = str.size() + 1; std::vector<wchar_t> buf(wchar_count);
      return std::wstring{ buf.data(), (std::size_t)MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, buf.data(), (int)wchar_count) };
    }

    inline std::string narrow(std::wstring wstr) {
      int nbytes = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), nullptr, 0, nullptr, nullptr); std::vector<char> buf(nbytes);
      return std::string{ buf.data(), (std::size_t)WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), buf.data(), nbytes, nullptr, nullptr) };
    }
    #endif

    inline std::string environment_get_variable(std::string name) {
      #if defined(_WIN32)
      std::string value;
      wchar_t buffer[32767];
      std::wstring u8name = widen(name);
      if (GetEnvironmentVariableW(u8name.c_str(), buffer, 32767) != 0) {
        value = narrow(buffer);
      }
      return value;
      #else
      char *value = getenv(name.c_str());
      return value ? value : "";
      #endif
    }

    inline bool environment_get_variable_exists(std::string name) {
      #if defined(_WIN32)
      std::string value;
      wchar_t buffer[32767];
      std::wstring u8name = widen(name);
      GetEnvironmentVariableW(u8name.c_str(), buffer, 32767);
      return (GetLastError() != ERROR_ENVVAR_NOT_FOUND);
      #else
      return (getenv(name.c_str()) != nullptr);
      #endif
    }

    inline std::string environment_expand_variables(std::string str) {
      if (str.find("${") == std::string::npos) return str;
      std::string pre = str.substr(0, str.find("${"));
      std::string post = str.substr(str.find("${") + 2);
      if (post.find('}') == std::string::npos) return str;
      std::string variable = post.substr(0, post.find('}'));
      std::size_t pos = post.find('}') + 1; post = post.substr(pos);
      std::string value = environment_get_variable(variable);
      if (!environment_get_variable_exists(variable))
        return str.substr(0, pos) + environment_expand_variables(str.substr(pos));
      return environment_expand_variables(pre + value + post);
    }

    inline std::string expand_without_trailing_slash(std::string dname) {
      std::error_code ec;
      dname = environment_expand_variables(dname);
      fs::path p = fs::path(dname);
      p = fs::absolute(p, ec);
      if (ec.value() != 0) return "";
      dname = p.string();
      #if defined(_WIN32)
      while ((dname.back() == '\\' || dname.back() == '/') && 
        (p.root_name().string() + "\\" != dname && p.root_name().string() + "/" != dname)) {
        message_pump(); p = fs::path(dname); dname.pop_back();
      }
      #else
      while (dname.back() == '/' && (!dname.empty() && dname[0] != '/' && dname.length() != 1)) {
        dname.pop_back();
      }
      #endif
      return dname;
    }

    inline std::string expand_with_trailing_slash(std::string dname) {
      dname = expand_without_trailing_slash(dname);
      #if defined(_WIN32)
      if (dname.back() != '\\') dname += "\\";
      #else
      if (dname.back() != '/') dname += "/";
      #endif
      return dname;
    }

    inline bool file_exists(std::string fname) {
      std::error_code ec;
      fname = expand_without_trailing_slash(fname);
      const fs::path path = fs::path(fname);
      return (fs::exists(path, ec) && ec.value() == 0 && 
        (!fs::is_directory(path, ec)) && ec.value() == 0);
    }

    inline bool directory_exists(std::string dname) {
      std::error_code ec;
      dname = expand_without_trailing_slash(dname);
      dname = expand_without_trailing_slash(dname);
      const fs::path path = fs::path(dname);
      return (fs::exists(path, ec) && ec.value() == 0 && 
        fs::is_directory(path, ec) && ec.value() == 0);
    }

    inline std::string filename_absolute(std::string fname) {
      std::string result;
      if (directory_exists(fname)) {
        result = expand_with_trailing_slash(fname);
      } else if (file_exists(fname)) {
        result = expand_without_trailing_slash(fname);
      }
      return result;
    }

    struct findhardlinks_struct {
      std::vector<std::string> vec;
      bool recursive;
      unsigned index;
      #if defined(_WIN32)
      BY_HANDLE_FILE_INFORMATION info;
      #else
      struct stat info;
      #endif
    };

    /* fs::equivalent would be useful here but sadly it does 
    not support passing a file descriptor and only accepts a file path */
    std::vector<std::string> findhardlinks_result;
    inline void findhardlinks_helper(findhardlinks_struct *s) {
      #if defined(_WIN32)
      if (findhardlinks_result.size() >= s->info.nNumberOfLinks) return;
      #else
      if (findhardlinks_result.size() >= s->info.st_nlink) return;
      #endif
      std::error_code ec; if (!directory_exists(s->vec[s->index])) return;
      s->vec[s->index] = expand_without_trailing_slash(s->vec[s->index]);
      const fs::path path = fs::path(s->vec[s->index]);
      if (directory_exists(s->vec[s->index]) || path.root_name().string() + "\\" == path.string()) {
        fs::directory_iterator end_itr;
        for (fs::directory_iterator dir_ite(path, ec); dir_ite != end_itr; dir_ite.increment(ec)) {
          message_pump(); if (ec.value() != 0) { break; }
          fs::path file_path = fs::path(filename_absolute(dir_ite->path().string()));
          #if defined(_WIN32)
          int fd = -1;
          BY_HANDLE_FILE_INFORMATION info = { 0 };
          if (file_exists(file_path.string())) {
            // printf("%s\n", file_path.string().c_str());
            if (!_wsopen_s(&fd, file_path.wstring().c_str(), _O_RDONLY, _SH_DENYNO, _S_IREAD)) {
              bool success = GetFileInformationByHandle((HANDLE)_get_osfhandle(fd), &info);
              bool matches = (info.ftLastWriteTime.dwLowDateTime == s->info.ftLastWriteTime.dwLowDateTime && 
                info.ftLastWriteTime.dwHighDateTime == s->info.ftLastWriteTime.dwHighDateTime && 
                info.nFileIndexHigh == s->info.nFileIndexHigh && info.nFileIndexLow == s->info.nFileIndexLow &&
                info.nFileSizeHigh == s->info.nFileSizeHigh && info.nFileSizeLow == s->info.nFileSizeLow && 
                info.dwVolumeSerialNumber == s->info.dwVolumeSerialNumber);
              if (matches && success) {
                findhardlinks_result.push_back(file_path.string());
                if (findhardlinks_result.size() >= info.nNumberOfLinks) {
                  s->info.nNumberOfLinks = info.nNumberOfLinks; 
                  s->vec.clear();
                  _close(fd);
                  return;
                }
              }
              _close(fd);
            }
          }
          #else
          struct stat info = { 0 }; 
          if (file_exists(file_path.string())) {
            // printf("%s\n", file_path.string().c_str());
            if (!stat(file_path.string().c_str(), &info)) {
              if (info.st_dev == s->info.st_dev && info.st_ino == s->info.st_ino && 
                info.st_size == s->info.st_size && info.st_mtime == s->info.st_mtime) {
                findhardlinks_result.push_back(file_path.string());
                if (findhardlinks_result.size() >= info.st_nlink) {
                  s->info.st_nlink = info.st_nlink; s->vec.clear();
                  return;
                }
              }
            }
          }
          #endif
          if (s->recursive && directory_exists(file_path.string())) {
            // printf("%s\n", file_path.string().c_str());
            s->vec.push_back(file_path.string());
            s->index++; findhardlinks_helper(s);
          }
        }
      }
      while (s->index < s->vec.size() - 1) {
        message_pump(); s->index++;
        findhardlinks_helper(s);
      }
    }

 } // anonymous namespace

  inline std::vector<std::string> findhardlinks(int fd, std::vector<std::string> dnames, bool recursive) {
    std::vector<std::string> paths;
    #if defined(_WIN32)
    BY_HANDLE_FILE_INFORMATION info = { 0 };
    if (GetFileInformationByHandle((HANDLE)_get_osfhandle(fd), &info) && info.nNumberOfLinks) {
    #else
    struct stat info = { 0 };
    if (!fstat(fd, &info) && info.st_nlink) {
    #endif
      findhardlinks_result.clear();
      struct findhardlinks_struct s; 
      s.vec             = dnames;
      s.index           = 0;
      s.recursive       = recursive;
      s.info            = info;
      findhardlinks_helper(&s);
      paths = findhardlinks_result;
    }
    return paths;
  }

} // namespace findhardlinks
