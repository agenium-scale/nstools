// MIT License
//
// Copyright (c) 2019 Agenium Scale
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <ns2/config.hpp>
#include <ns2/exception.hpp>
#include <ns2/fs.hpp>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#ifdef NS2_IS_MSVC
#include <direct.h>
#include <io.h>
#include <shlwapi.h>
#include <windows.h>
#else
#include <cstdlib>
#include <dirent.h>
#include <fnmatch.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace ns2 {

// ----------------------------------------------------------------------------

std::string sanitize(std::string const &path) {
#ifdef NS2_IS_MSVC
  return replace(path, '/', '\\');
#else
  return path;
#endif
}

// ----------------------------------------------------------------------------

std::string unixes(std::string const &path) {
#ifdef NS2_IS_MSVC
  return replace(path, '\\', '/');
#else
  return path;
#endif
}

// ----------------------------------------------------------------------------
// buf_t members implementation

void buf_t::initp() {
  setp(buf_, buf_ + sizeof(buf_));
  setg(NULL, NULL, NULL);
}

// ---

buf_t::buf_t()
    : stream_(NULL), filename_("[unknown filename]"),
      nb_tries_eintr_(NS2_STDIO_DEFAULT_NB_TRIES_EINTR), own_file_(false) {
  initp();
}

// ---

buf_t::buf_t(FILE *stream)
    : stream_(stream), filename_("[unknown filename]"),
      nb_tries_eintr_(NS2_STDIO_DEFAULT_NB_TRIES_EINTR), own_file_(false) {
  initp();
}

// ---

buf_t::buf_t(FILE *stream, std::string const &filename)
    : stream_(stream), filename_(filename),
      nb_tries_eintr_(NS2_STDIO_DEFAULT_NB_TRIES_EINTR), own_file_(false) {
  initp();
}

// ---

buf_t::buf_t(std::string const &filename, const char *mode)
    : stream_(NULL), filename_(filename),
      nb_tries_eintr_(NS2_STDIO_DEFAULT_NB_TRIES_EINTR), own_file_(true) {
  stream_ = ::fopen(sanitize(filename).c_str(), mode);
  if (stream_ == NULL) {
    NS2_THROW(file_error_t,
              filename + ": cannot open file for reading: " + strerror(errno));
  }
  initp();
}

// ---

void buf_t::close() {
  if (stream_ != NULL && own_file_) {
    sync();
    if (::fclose(stream_) == EOF) {
      NS2_THROW(file_error_t,
                filename_ + ": error on close: " + strerror(errno));
    }
  }
  stream_ = NULL;
}

// ---

void buf_t::open(std::string const &filename, const char *mode) {
  close();
  filename_ = filename;
  stream_ = ::fopen(sanitize(filename).c_str(), mode);
  if (stream_ == NULL) {
    NS2_THROW(file_error_t,
              filename_ + ": cannot open file: " + strerror(errno));
  }
  own_file_ = true;
  initp();
}

// ---

bool buf_t::is_open() const { return stream_ != NULL; }

// ---

buf_t::~buf_t() {
  if (stream_ != NULL && own_file_) {
    sync();
    ::fclose(stream_);
  }
  stream_ = NULL;
}

// ---

#if NS2_CXX >= 2011
buf_t::buf_t(buf_t &&other)
    : stream_(other.stream_), filename_(other.filename_),
      nb_tries_eintr_(other.nb_tries_eintr_) {
  other.stream_ = NULL;
  other.filename_ = NS2_UNKNOWN_FILENAME;
}

buf_t &buf_t::operator=(buf_t &&other) {
  close();
  stream_ = other.stream_;
  filename_ = other.filename_;
  nb_tries_eintr_ = other.nb_tries_eintr_;
  other.stream_ = NULL;
  other.filename_ = NS2_UNKNOWN_FILENAME;
  return *this;
}
#endif

// ---

int buf_t::sync() {
  overflow();
  if (::fflush(stream_) == EOF) {
    NS2_THROW(file_error_t, filename_ + ": cannot flush: " + strerror(errno));
  }
  return 0;
}

// ---

FILE *buf_t::c_file() const { return stream_; }
const char *buf_t::filename() const { return filename_.c_str(); }

// ---

buf_t::traits_type::pos_type buf_t::seekoff(buf_t::traits_type::off_type off,
                                            std::ios_base::seekdir dir,
                                            std::ios_base::openmode) {
  int whence = SEEK_CUR;
  switch (dir) {
  case std::basic_ios<char>::beg:
    whence = SEEK_SET;
    break;
  case std::basic_ios<char>::cur:
    whence = SEEK_CUR;
    break;
  case std::basic_ios<char>::end:
    whence = SEEK_END;
    break;
  default: // only for GCC to avoid a warning:
    break; // https://github.com/pocoproject/poco/issues/928
  }
  sync();
  if (::fseek(stream_, long(off), whence) == -1) {
    NS2_THROW(file_error_t, filename_ + ": cannot seek: " + strerror(errno));
  }
  long pos = ::ftell(stream_);
  if (pos == -1) {
    NS2_THROW(file_error_t, filename_ + ": cannot tell: " + strerror(errno));
  }
  initp();
  return traits_type::pos_type(traits_type::off_type(pos));
}

// ---

buf_t::traits_type::pos_type buf_t::seekpos(buf_t::traits_type::pos_type pos,
                                            std::ios_base::openmode) {
  if (::fseek(stream_, long(pos), SEEK_SET) == -1) {
    NS2_THROW(file_error_t, filename_ + ": cannot seek: " + strerror(errno));
  }
  initp();
  return traits_type::pos_type(traits_type::off_type(pos));
}

// ---

buf_t::int_type buf_t::overflow(int_type ch) {
  if (buf_t::traits_type::eq_int_type(ch, buf_t::traits_type::eof())) {
    size_t len = size_t(pptr() - pbase());
    if (len > 0) {
      size_t begin = 0;
#ifdef NS2_IS_MSVC
      while (len > 0) {
        size_t code = ::fwrite(pbase() + begin, 1, len, stream_);
        if (code == 0) {
          NS2_THROW(file_error_t,
                    filename_ + ": cannot write: " + strerror(errno));
        }
        len -= code;
        begin += code;
      }
#else
      int i = 0;
      while (len > 0 && i < nb_tries_eintr_) {
        size_t code = ::fwrite(pbase() + begin, 1, len, stream_);
        if (code == 0) {
          if (errno == EINTR) {
            i++;
            continue;
          }
          NS2_THROW(file_error_t,
                    filename_ + ": cannot write: " + strerror(errno));
        }
        len -= code;
        begin += code;
      }
      if (i == nb_tries_eintr_) {
        NS2_THROW(file_error_t, filename_ + ": cannot write: too many EINTR");
      }
#endif
      setp(buf_, buf_ + sizeof(buf_));
    }
  } else if (pptr() == epptr()) {
    size_t len = size_t(pptr() - pbase());
#ifdef NS2_IS_MSVC
    size_t code = ::fwrite(pbase(), 1, len, stream_);
    if (code == 0) {
      NS2_THROW(file_error_t,
                filename_ + ": cannot write: " + strerror(errno));
    } else if (code == len) {
      setp(buf_, buf_ + sizeof(buf_));
    } else if (code < len) {
      memmove(buf_, pbase() + code, len - code);
      setp(buf_, buf_ + sizeof(buf_));
      pbump(int(len - code));
    }
    *pptr() = char(ch);
    pbump(1);
    return ch;
#else
    for (int i = 0; i < nb_tries_eintr_; i++) {
      size_t code = ::fwrite(pbase(), 1, len, stream_);
      if (code == 0) {
        if (errno == EINTR) {
          continue;
        }
        NS2_THROW(file_error_t,
                  filename_ + ": cannot write: " + strerror(errno));
      } else if (code == len) {
        setp(buf_, buf_ + sizeof(buf_));
      } else if (code < len) {
        memmove(buf_, pbase() + code, len - code);
        setp(buf_, buf_ + sizeof(buf_));
        pbump(int(len - code));
      }
      *pptr() = char(ch);
      pbump(1);
      return ch;
    }
    NS2_THROW(file_error_t, filename_ + ": cannot write: too many EINTR");
#endif
  }
  return ch;
}

// ---

buf_t::int_type buf_t::underflow() {
  if (gptr() == NULL || gptr() >= egptr()) {
#ifdef NS2_IS_MSVC
    errno = 0;
    size_t code = ::fread(buf_, 1, NS2_STREAMBUF_BUFFER_SIZE, stream_);
    if (code == 0) {
      if (errno != 0) {
        NS2_THROW(file_error_t,
                  filename_ + ": cannot read: " + strerror(errno));
      }
      return traits_type::eof();
    } else {
      setg(buf_, buf_, buf_ + code);
      return traits_type::to_int_type(buf_[0]);
    }
#else
    for (int i = 0; i < nb_tries_eintr_; i++) {
      errno = 0;
      size_t code = ::fread(buf_, 1, NS2_STREAMBUF_BUFFER_SIZE, stream_);
      if (code == 0) {
        if (errno == EINTR) {
          continue;
        }
        if (errno != 0) {
          NS2_THROW(file_error_t,
                    filename_ + ": cannot read: " + strerror(errno));
        }
        return traits_type::eof();
      } else {
        setg(buf_, buf_, buf_ + code);
        return traits_type::to_int_type(buf_[0]);
      }
    }
    NS2_THROW(file_error_t, filename_ + ": cannot read: too many EINTR");
#endif
  } else {
    return traits_type::to_int_type(*gptr());
  }
}

// ----------------------------------------------------------------------------
// ifile_t members implementation

ifile_t::ifile_t() : buf_t(), std::istream(this) {}
ifile_t::ifile_t(FILE *in) : buf_t(in), std::istream(this) {}
ifile_t::ifile_t(FILE *in, std::string const &filename)
    : buf_t(in, filename), std::istream(this) {}
ifile_t::ifile_t(std::string const &filename)
    : buf_t(filename, "rb"), std::istream(this) {}
ifile_t::~ifile_t() {}
void ifile_t::open(std::string const &filename) {
  buf_t::open(filename, "rb");
}
void ifile_t::close() { buf_t::close(); }
bool ifile_t::is_open() const { return buf_t::is_open(); }
std::string ifile_t::filename() const { return buf_t::filename(); }

// ---

FILE *ifile_t::c_file() {
  int offset = int(egptr() - gptr());
  FILE *stream = buf_t::c_file();
  if (offset > 0) {
    if (fseek(stream, offset, SEEK_CUR) == -1) {
      if (errno != 0 && errno != ESPIPE) {
        NS2_THROW(file_error_t, std::string(filename()) +
                                    ": cannot seek: " + strerror(errno));
      }
    }
  }
  return stream;
}

// ----------------------------------------------------------------------------
// ofile_t members implementation

ofile_t::ofile_t() : buf_t(), std::ostream(this) {}
ofile_t::ofile_t(FILE *in) : buf_t(in), std::ostream(this) {}
ofile_t::ofile_t(FILE *in, std::string const &filename)
    : buf_t(in, filename), std::ostream(this) {}
ofile_t::ofile_t(std::string const &filename)
    : buf_t(filename, "wb"), std::ostream(this) {}
ofile_t::~ofile_t() {}
void ofile_t::open(std::string const &filename) {
  buf_t::open(filename, "wb");
}
void ofile_t::close() { buf_t::close(); }
bool ofile_t::is_open() const { return buf_t::is_open(); }
std::string ofile_t::filename() const { return buf_t::filename(); }

// ---

FILE *ofile_t::c_file() {
  buf_t::sync();
  return buf_t::c_file();
}

// ----------------------------------------------------------------------------

std::string read_file(std::string const &path) {
  ns2::ifile_t in(path);
  return std::string(std::istreambuf_iterator<char>(in),
                     std::istreambuf_iterator<char>());
}

// ----------------------------------------------------------------------------

bool exists(std::string const &path) {
#ifdef NS2_IS_MSVC
  return _access(sanitize(path).c_str(), 0) == 0;
#else
  return access(path.c_str(), F_OK) == 0;
#endif
}

// ----------------------------------------------------------------------------

bool isdir(std::string const &path) {
#ifdef NS2_IS_MSVC
  DWORD code = GetFileAttributesA(sanitize(path).c_str());
  if (code == INVALID_FILE_ATTRIBUTES) {
    NS2_THROW(file_error_t, "cannot stat");
  }
  return (code & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
  struct stat buf;
  if (stat(path.c_str(), &buf) != 0) {
    NS2_THROW(file_error_t, "cannot stat");
  }
  return S_ISDIR(buf.st_mode);
#endif
}

// ----------------------------------------------------------------------------

bool isexe(std::string const &path) {
#ifdef NS2_IS_MSVC
  if (endswith(lower(path), ".exe")) {
    return true;
  } else {
    return false;
  }
#else
  struct stat buf;
  if (stat(path.c_str(), &buf) != 0) {
    NS2_THROW(file_error_t, "cannot stat");
  }
  return (buf.st_mode & S_IXUSR) != 0;
#endif
}

// ----------------------------------------------------------------------------

dir_file_t split_path(std::string const &pathname) {
  size_t i = unixes(pathname).rfind('/');
  if (i == std::string::npos) {
    return dir_file_t(std::string(), pathname);
  } else {
    return dir_file_t(std::string(pathname, 0, i),
                      std::string(pathname, i + 1));
  }
}

// ----------------------------------------------------------------------------

std::string join_path(std::string const &str1, std::string const &str2) {
  if (str1.size() == 0) {
    return str2;
  } else if (str2.size() == 0) {
    return str1;
  } else {
    return str1 + "/" + str2;
  }
}

// ----------------------------------------------------------------------------

static void glob_rec(std::vector<std::string> *ret,
                     std::string const &base_dir, std::string const &glob) {
  std::vector<std::string> other_dirs;
  std::string pattern(join_path(base_dir, glob));
#ifdef NS2_IS_MSVC
  std::string buf;
  std::string temp(join_path(base_dir, "*"));
  WIN32_FIND_DATAA entry;
  HANDLE dir = FindFirstFileA(sanitize(temp).c_str(), &entry);
  if (dir != INVALID_HANDLE_VALUE) {
    for (;;) {
      if (strcmp(entry.cFileName, ".") && strcmp(entry.cFileName, "..")) {
        try {
          buf = join_path(base_dir, entry.cFileName);
          if (isdir(buf)) {
            other_dirs.push_back(buf);
          }
        } catch (std::bad_alloc const &) {
          FindClose(dir);
          NS2_THROW_BAD_ALLOC();
        } catch (std::length_error const &) {
          FindClose(dir);
          NS2_THROW_BAD_ALLOC();
        }
        if (PathMatchSpecA(buf.c_str(), pattern.c_str())) {
          try {
            (*ret).push_back(buf);
          } catch (std::bad_alloc const &) {
            FindClose(dir);
            NS2_THROW_BAD_ALLOC();
          } catch (std::length_error const &) {
            FindClose(dir);
            NS2_THROW_BAD_ALLOC();
          }
        }
      }
      if (FindNextFileA(dir, &entry) == 0) {
        FindClose(dir);
        break;
      }
    }
    FindClose(dir);
  }
#else
  std::string buf;
  DIR *dir = opendir(base_dir == "" ? "." : base_dir.c_str());
  if (dir != NULL) {
    for (;;) {
      errno = 0;
      struct dirent *entry = readdir(dir);
      if (entry == NULL) {
        break;
      }
      if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) {
        continue;
      }
      try {
        buf = join_path(base_dir, entry->d_name);
        if (isdir(buf)) {
          other_dirs.push_back(buf);
        }
      } catch (std::bad_alloc const &) {
        closedir(dir);
        NS2_THROW_BAD_ALLOC();
      } catch (std::length_error const &) {
        closedir(dir);
        NS2_THROW_BAD_ALLOC();
      }
      if (fnmatch(pattern.c_str(), buf.c_str(), FNM_PATHNAME | FNM_NOESCAPE) ==
          0) {
        try {
          (*ret).push_back(buf);
        } catch (std::bad_alloc const &) {
          closedir(dir);
          NS2_THROW_BAD_ALLOC();
        } catch (std::length_error const &) {
          closedir(dir);
          NS2_THROW_BAD_ALLOC();
        }
      }
    }
    closedir(dir);
  }
#endif
  for (size_t i = 0; i < other_dirs.size(); i++) {
    glob_rec(ret, other_dirs[i], glob);
  }
}

// ----------------------------------------------------------------------------

std::vector<std::string> glob(std::string const &base_dir,
                              std::string const &pattern) {
  std::vector<std::string> ret;
  glob_rec(&ret, base_dir, pattern);
  return ret;
}

// ----------------------------------------------------------------------------

std::vector<std::string> glob(std::string const &pattern) {
  std::vector<std::string> ret;
  dir_file_t df = split_path(pattern);
  glob_rec(&ret, df.first, df.second);
  return ret;
}

// ----------------------------------------------------------------------------

dir_file_t find(std::vector<std::string> const &paths,
                std::vector<std::string> const &files) {
  for (size_t i = 0; i < paths.size(); i++) {
    for (size_t j = 0; j < files.size(); j++) {
      if (exists(join_path(paths[i], files[j]))) {
        return dir_file_t(paths[i], files[j]);
      }
    }
  }
  return dir_file_t(std::string(), std::string());
}

// ----------------------------------------------------------------------------

bool found(dir_file_t const &df) {
  return df.first.size() > 0 || df.second.size() > 0;
}

// ----------------------------------------------------------------------------

dir_file_t find(std::vector<std::string> const &user_paths,
                std::vector<std::string> const &files,
                const char **path_from_env, int nb_path_from_env,
                bool user_path_first) {
  dir_file_t ret;
  if (user_path_first) {
    ret = find(user_paths, files);
    if (found(ret)) {
      return ret;
    }
  }

  for (int i = 0; i < nb_path_from_env; i++) {
    const char *dirs = getenv(path_from_env[i]);
    if (dirs == NULL) {
      continue;
    }
#ifdef NS2_IS_MSVC
    std::vector<std::string> paths = split(dirs, ';');
#else
    std::vector<std::string> paths = split(dirs, ':');
#endif

    ret = find(paths, files);
    if (found(ret)) {
      return ret;
    }
  }

  if (!user_path_first) {
    ret = find(user_paths, files);
    if (found(ret)) {
      return ret;
    }
  }

  return dir_file_t(std::string(), std::string());
}

// ----------------------------------------------------------------------------

std::string absolute(std::string const &str) {
  if (str.size() > 0 && unixes(str)[0] == '/') {
    return str;
  }
#ifdef NS2_IS_MSVC
  char pwd[MAX_PATH];
  if (GetCurrentDirectory(MAX_PATH, pwd) == 0) {
    return join_path("C:/", str);
#else
  const char *pwd = getenv("PWD");
  if (pwd == NULL) {
    return join_path("/", str);
#endif
  } else {
    return join_path(pwd, str);
  }
}

// ----------------------------------------------------------------------------

std::string dirname(std::string const &str) {
  size_t pos = unixes(str).rfind('/');
  return pos == std::string::npos ? std::string() : std::string(str, 0, pos);
}

// ----------------------------------------------------------------------------

std::string gettempdir() {
#ifdef NS2_IS_MSVC
  std::vector<char> buf(MAX_PATH + 1, ' ');
  std::string ret(MAX_PATH + 1, ' ');
  if (GetTempPathA(DWORD(ret.size() + 1), &ret[0]) == 0) {
    return "C:/";
  }
  return std::string(&ret[0]);
#else
  return "/tmp";
#endif
}

// ----------------------------------------------------------------------------

std::string mkstemp(std::string const &dir_, std::string const &prefix) {
  // empty dir_ means we use system temp dir
  std::string dir(dir_.size() == 0 ? gettempdir() : dir_);

#ifdef NS2_IS_MSVC
  // there seems to be no equivalent on Windows, so we provide one
  dir = sanitize(dir);
  for (int i = 0; i < 100; i++) {
    std::string ret(dir + "\\" + prefix + to_string(::rand()));
    HANDLE fd = CreateFile(ret.c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW,
                           FILE_ATTRIBUTE_NORMAL, NULL);
    if (fd == INVALID_HANDLE_VALUE) {
      if (GetLastError() == ERROR_FILE_EXISTS) {
        continue;
      } else {
        break;
      }
    }
    CloseHandle(fd);
    return ret;
  }
  NS2_THROW(temp_file_error_t, "cannot create temporary file in \"" + dir +
                                   "\": " + win_strerror(GetLastError()));
#else
  std::string temp(dir + "/" + prefix + "XXXXXX");
  std::vector<char> buf(temp.c_str(), temp.c_str() + (temp.size() + 1));
  int fd = ::mkstemp(&buf[0]);
  if (fd != -1) {
    close(fd);
    return std::string(&buf[0]);
  } else {
    NS2_THROW(temp_file_error_t, "cannot create temporary file in \"" + dir +
                                     "\": " + strerror(errno));
  }
#endif
}

// ----------------------------------------------------------------------------

std::string mkdtemp(std::string const &dir_, std::string const &prefix) {
  // empty dir_ means we use system temp dir
  std::string dir(dir_.size() == 0 ? gettempdir() : dir_);

#ifdef NS2_IS_MSVC
  // there seems to be no equivalent on Windows, so we provide one
  dir = sanitize(dir);
  for (int i = 0; i < 100; i++) {
    std::string ret(dir + "\\" + prefix + to_string(::rand()));
    BOOL code = CreateDirectoryA(ret.c_str(), NULL);
    if (code == FALSE) {
      if (GetLastError() == ERROR_ALREADY_EXISTS) {
        continue;
      } else {
        break;
      }
    }
    return ret;
  }
  NS2_THROW(temp_file_error_t, "cannot create temporary directory in \"" +
                                   dir +
                                   "\": " + win_strerror(GetLastError()));
#else
  std::string temp(dir + "/" + prefix + "XXXXXX");
  std::vector<char> buf(temp.c_str(), temp.c_str() + (temp.size() + 1));
  char *code = ::mkdtemp(&buf[0]);
  if (code == NULL) {
    NS2_THROW(temp_file_error_t, "cannot create temporary file in \"" + dir +
                                     "\": " + strerror(errno));
  }
  return std::string(&buf[0]);
#endif
}

// ----------------------------------------------------------------------------

NS_DLLEXPORT void rm_rf(std::string const &path) {
#ifdef NS2_IS_MSVC
#define UNLINK _unlink
#define RMDIR _rmdir
#else
#define UNLINK unlink
#define RMDIR rmdir
#endif
  if (!exists(path)) {
    return;
  }
  if (isdir(path)) {
    std::vector<std::string> entries = glob(path, "*");
    for (size_t i = 0; i < entries.size(); i++) {
      rm_rf(entries[i]);
    }
    if (RMDIR(sanitize(path).c_str()) == -1) {
      NS2_THROW(rm_rf_error_t, "cannot remove directory \"" + path +
                                   "\": " + std::string(strerror(errno)));
    }
  } else {
    if (UNLINK(sanitize(path).c_str()) == -1) {
      NS2_THROW(rm_rf_error_t, "cannot remove file \"" + path +
                                   "\": " + std::string(strerror(errno)));
    }
  }
#undef UNLINK
#undef RMDIR
}

// ----------------------------------------------------------------------------

void copyfile(std::string const &from, std::string const &to) {
  ns2::ifile_t in(from);
  ns2::ofile_t out(to);
  out << in.rdbuf();
  out.close(); // explicitly close output to get error as dtor does not throw
}

// ----------------------------------------------------------------------------

std::pair<std::string, std::string> splitext(std::string const &filename) {
  std::string buf(unixes(filename));
  std::string ext;
  size_t i;
  for (i = buf.size(); i != 0; i--) {
    if (buf[i] == '/') {
      break;
    }
    if (buf[i] == '.') {
      ext = std::string(buf, i + 1);
      break;
    }
  }
  if (ext.size() == 0) {
    i = filename.size();
  }
  return std::pair<std::string, std::string>(std::string(filename, 0, i), ext);
}

// ----------------------------------------------------------------------------

void mkdir(std::string const &path) {
#ifdef NS2_IS_MSVC
#define MKDIR(path) _mkdir(path)
#else
#define MKDIR(path) ::mkdir(path, 0700)
#endif
  std::vector<std::string> pieces(split(path, '/'));
  std::string curr_dir;
  for (size_t i = 0; i < pieces.size(); i++) {
    curr_dir = join_path(curr_dir, pieces[i]);
    errno = 0;
    if (MKDIR(curr_dir.c_str()) == -1) {
      if (errno == EEXIST) {
        continue;
      } else {
        NS2_THROW(mkdir_error_t, "cannot create directory \"" + curr_dir +
                                     "\": " + std::string(strerror(errno)));
      }
    }
  }
#undef MKDIR
}

// ----------------------------------------------------------------------------

} // namespace ns2
