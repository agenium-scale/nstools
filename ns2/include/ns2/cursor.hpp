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

#ifndef NS2_CURSOR_HPP
#define NS2_CURSOR_HPP

#include <ns2/config.hpp>
#include <string>

namespace ns2 {

// ----------------------------------------------------------------------------

struct cursor_t {
  size_t lineno, col;
  std::string line;

  NS_DLLSPEC cursor_t();
  NS_DLLSPEC cursor_t(cursor_t const &);
  NS_DLLSPEC cursor_t &operator=(cursor_t const &);
  NS_DLLSPEC void newline();
  NS_DLLSPEC void nextchar();
  NS_DLLSPEC void advanceby(size_t);
  NS_DLLSPEC void advanceby(int);
  NS_DLLSPEC std::string to_string() const;
  NS_DLLSPEC cursor_t &operator+=(size_t);
};

// ----------------------------------------------------------------------------

NS_DLLSPEC cursor_t operator+(cursor_t const &, size_t);

// ----------------------------------------------------------------------------

NS_DLLSPEC cursor_t make_cursor(cursor_t const &, size_t, std::string const &,
                                size_t);

// ----------------------------------------------------------------------------

} // namespace ns2

#endif
