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

#ifndef NS2_FS_HPP
#define NS2_FS_HPP

#include <exception>
#include <string>
#include <utility>
#include <vector>

#include <ns2/config.hpp>
#include <ns2/exception.hpp>
#include <ns2/string.hpp>

namespace ns2 {

// ----------------------------------------------------------------------------

typedef std::pair<std::string, std::string> dir_file_t;

// ----------------------------------------------------------------------------

class file_error_t : public std::exception {
  std::string what_;

public:
  file_error_t(std::string const &what) : what_(what) {}

  const char *what() const throw() { return what_.c_str(); }

  ~file_error_t() throw() {}
};

// ----------------------------------------------------------------------------

#define NS2_STREAMBUF_BUFFER_SIZE 4096
#define NS2_STDIO_DEFAULT_NB_TRIES_EINTR 10
#define NS2_UNKNOWN_FILENAME "[unknown filename]"

// ----------------------------------------------------------------------------

class buf_t : public std::streambuf {
  FILE *stream_;
  char buf_[NS2_STREAMBUF_BUFFER_SIZE];
  std::string filename_;
  int nb_tries_eintr_;
  bool own_file_;
  NS_DLLSPEC void initp();

public:
  NS_DLLSPEC buf_t();
  NS_DLLSPEC buf_t(FILE *);
  NS_DLLSPEC buf_t(FILE *, std::string const &);
  NS_DLLSPEC buf_t(std::string const &, const char *);
  NS_DLLSPEC ~buf_t();
#if NS2_CXX >= 2011
  NS_DLLSPEC buf_t(buf_t &&);
  NS_DLLSPEC buf_t &operator=(buf_t &&);
#endif
  NS_DLLSPEC void open(std::string const &, const char *);
  NS_DLLSPEC void close();
  NS_DLLSPEC bool is_open() const;

protected:
  NS_DLLSPEC int sync();
  NS_DLLSPEC int_type overflow(int_type ch = traits_type::eof());
  NS_DLLSPEC int_type underflow();
  NS_DLLSPEC FILE *c_file() const;
  NS_DLLSPEC const char *filename() const;
  NS_DLLSPEC traits_type::pos_type seekoff(
      traits_type::off_type, std::ios_base::seekdir,
      std::ios_base::openmode which = std::ios_base::in | std::ios_base::out);
  NS_DLLSPEC traits_type::pos_type seekpos(
      traits_type::pos_type,
      std::ios_base::openmode which = std::ios_base::in | std::ios_base::out);
};

// ----------------------------------------------------------------------------

class ifile_t : private buf_t, public std::istream {
private:
  // no copy
  ifile_t &operator=(ifile_t const &);
  ifile_t(ifile_t const &);

public:
  NS_DLLSPEC ifile_t();
  NS_DLLSPEC ifile_t(FILE *);
  NS_DLLSPEC ifile_t(std::string const &);
  NS_DLLSPEC ifile_t(FILE *, std::string const &);
  NS_DLLSPEC void open(std::string const &);
  NS_DLLSPEC void close();
  NS_DLLSPEC bool is_open() const;
  NS_DLLSPEC ~ifile_t();
  NS_DLLSPEC FILE *c_file();
  NS_DLLSPEC std::string filename() const;
};

// ----------------------------------------------------------------------------

class ofile_t : private buf_t, public std::ostream {
private:
  // no copy
  ofile_t &operator=(ofile_t const &);
  ofile_t(ofile_t const &);

public:
  NS_DLLSPEC ofile_t();
  NS_DLLSPEC ofile_t(FILE *);
  NS_DLLSPEC ofile_t(std::string const &);
  NS_DLLSPEC ofile_t(FILE *, std::string const &);
  NS_DLLSPEC void open(std::string const &);
  NS_DLLSPEC void close();
  NS_DLLSPEC bool is_open() const;
  NS_DLLSPEC ~ofile_t();
  NS_DLLSPEC FILE *c_file();
  NS_DLLSPEC std::string filename() const;
};

// ----------------------------------------------------------------------------

NS_DLLSPEC std::string read_file(std::string const &);

NS_DLLSPEC std::string sanitize(std::string const &);

NS_DLLSPEC bool isdir(std::string const &);

NS_DLLSPEC std::vector<std::string> glob(std::string const &,
                                         std::string const &);

NS_DLLSPEC dir_file_t find(std::vector<std::string> const &,
                           std::vector<std::string> const &);

NS_DLLSPEC dir_file_t find(std::vector<std::string> const &,
                           std::vector<std::string> const &, const char **,
                           int, bool);

NS_DLLSPEC bool exists(std::string const &);

NS_DLLSPEC bool isexe(std::string const &);

NS_DLLSPEC std::vector<std::string> glob(std::string const &);

NS_DLLSPEC bool found(dir_file_t const &);

NS_DLLSPEC dir_file_t split_path(std::string const &);

NS_DLLSPEC std::string absolute(std::string const &);

NS_DLLSPEC std::string join_path(std::string const &, std::string const &);

NS_DLLSPEC std::string dirname(std::string const &);

NS_DLLSPEC std::string gettempdir();

NS_DLLSPEC void copyfile(std::string const &, std::string const &);

NS_DLLSPEC std::pair<std::string, std::string> splitext(std::string const &);

// ----------------------------------------------------------------------------

#define EXCEPTION_HELPER(prefix)                                              \
  class prefix##_error_t : public std::exception {                            \
    std::string what_;                                                        \
                                                                              \
  public:                                                                     \
    prefix##_error_t(std::string const &what) : what_(what) {}                \
    const char *what() const throw() { return what_.c_str(); }                \
    ~prefix##_error_t() throw() {}                                            \
  };

// ----------------------------------------------------------------------------

EXCEPTION_HELPER(mkdir)

NS_DLLSPEC void mkdir(std::string const &);

// ----------------------------------------------------------------------------

EXCEPTION_HELPER(temp_file)

NS_DLLSPEC std::string mkstemp(std::string const &, std::string const &);

NS_DLLSPEC std::string mkdtemp(std::string const &, std::string const &);

// ----------------------------------------------------------------------------

EXCEPTION_HELPER(rm_rf)

NS_DLLEXPORT void rm_rf(std::string const &);

// ----------------------------------------------------------------------------

} // namespace ns2

#endif
