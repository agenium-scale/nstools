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

#ifndef NS2_STRING_HPP
#define NS2_STRING_HPP

#include <ns2/config.hpp>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace ns2 {

// ----------------------------------------------------------------------------

template <typename T> std::string to_string(T const &a) {
  std::ostringstream ostr;
  ostr << a;
  return ostr.str();
}

// ----------------------------------------------------------------------------

NS_DLLEXPORT std::vector<std::string> split(std::string const &, char);

NS_DLLEXPORT std::vector<std::string> split(std::string const &,
                                            std::string const &);

NS_DLLEXPORT bool is_blank(char);

NS_DLLEXPORT std::string strip(std::string const &);

NS_DLLEXPORT bool startswith(std::string const &, std::string const &);

NS_DLLEXPORT bool endswith(std::string const &, std::string const &);

NS_DLLEXPORT std::string replace(std::string const &, char, char);

NS_DLLEXPORT std::string replace(std::string const &, std::string const &,
                                 std::string const &);

NS_DLLEXPORT std::string join(std::vector<std::string> const &,
                              std::string const &);

NS_DLLEXPORT std::string lower(std::string const &);

// ----------------------------------------------------------------------------

template <typename ConstIterator>
std::string join(ConstIterator const &begin, ConstIterator const &end,
                 std::string const &sep) {
  ConstIterator it = begin;
  if (it == end) {
    return std::string();
  }
  std::string ret(to_string(*it));
  for (it++; it != end; it++) {
    ret += sep;
    ret += to_string(*it);
  }
  return ret;
}

// ----------------------------------------------------------------------------

template <typename T>
std::string join(std::vector<T> const &vec, std::string const &sep) {
  return join(vec.begin(), vec.end(), sep);
}

// ----------------------------------------------------------------------------

NS_DLLEXPORT std::string leading_whitespace(std::string const &);

NS_DLLEXPORT std::string indent_finder(std::string const &);

NS_DLLEXPORT std::string deindent(std::string const &);

// ----------------------------------------------------------------------------

} // namespace ns2

#endif
