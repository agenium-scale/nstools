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

#ifndef NS2_EXCEPTION_HPP
#define NS2_EXCEPTION_HPP

#include <iostream>
#include <new>
#include <ns2/backtrace.hpp>
#include <ns2/config.hpp>
#include <sstream>
#include <string>

#ifdef NS2_IS_MSVC
#include <windows.h>
#endif

namespace ns2 {

// ----------------------------------------------------------------------------

#ifdef NS2_DEBUG
#define NS2_EXCEPTION_BACKTRACE                                               \
  std::cerr << "Thrown at " << __FILE__ << ":" << __LINE__ << std::endl       \
            << ns2::backtrace() << std::endl;
#else
#define NS2_EXCEPTION_BACKTRACE
#endif

#ifdef NS2_NO_EXCEPTION

#define NS2_THROW(NS2_THROW_type, NS2_THROW_msg)                              \
  do {                                                                        \
    std::cerr << (NS2_THROW_msg) << std::endl;                                \
    NS2_EXCEPTION_BACKTRACE                                                   \
    abort();                                                                  \
  } while (0)

#else

#define NS2_THROW(NS2_THROW_type, NS2_THROW_msg)                              \
  do {                                                                        \
    std::ostringstream NS2_THROW_os;                                          \
    NS2_THROW_os << (NS2_THROW_msg) << std::endl;                             \
    NS2_EXCEPTION_BACKTRACE                                                   \
    throw NS2_THROW_type(NS2_THROW_os.str());                                 \
  } while (0)

#endif

// ----------------------------------------------------------------------------

class exception : public std::exception {
  std::string what_;

public:
  exception(std::string const &what) : what_(what) {}

  virtual ~exception() throw() {}

  virtual const char *what() const throw() { return what_.c_str(); }
};

// ----------------------------------------------------------------------------
// std::bad_alloc does not take a what but we need to get the stacktrace

class bad_alloc : public std::bad_alloc {
  std::string what_;

public:
  bad_alloc(std::string const &what) : what_(what) {}

  virtual ~bad_alloc() throw() {}

  virtual const char *what() const throw() { return what_.c_str(); }
};

#define NS2_THROW_BAD_ALLOC()                                                 \
  NS2_THROW(ns2::bad_alloc, std::string("malloc failed"))

// ----------------------------------------------------------------------------

#ifdef NS2_IS_MSVC
NS_DLLSPEC const char *win_strerror(DWORD);
#endif

NS_DLLSPEC const char *get_last_system_error(void);

// ----------------------------------------------------------------------------

} // namespace ns2

#endif
