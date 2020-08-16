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

#include <ns2/cursor.hpp>

#include <algorithm>

namespace ns2 {

// ----------------------------------------------------------------------------

NS_DLLSPEC cursor_t::cursor_t() : lineno(0), col(0) {}

NS_DLLSPEC cursor_t::cursor_t(cursor_t const &other)
    : lineno(other.lineno), col(other.col), line(other.line) {}

NS_DLLSPEC void cursor_t::newline() {
  lineno++;
  col = 0;
}

NS_DLLSPEC cursor_t &cursor_t::operator=(cursor_t const &other) {
  this->lineno = other.lineno;
  this->col = other.col;
  this->line = other.line;
  return *this;
}

NS_DLLSPEC void cursor_t::nextchar() { col++; }

NS_DLLSPEC void cursor_t::advanceby(size_t n) { col += n; }

NS_DLLSPEC void cursor_t::advanceby(int n) { col += size_t(n); }

NS_DLLSPEC std::string cursor_t::to_string() const {
  std::string ret("L" + std::to_string(lineno) + ":C" + std::to_string(col));
  if (line.size() > 0) {
    size_t i0 = size_t((std::max)(int(col - 1) - 12, 0));
    size_t i1 = (std::min)(size_t(col - 1 + 12), line.size() - 1);
    ret += " (near '" + line.substr(i0, i1 - i0 + 1) + "')";
  }
  return ret;
}

NS_DLLSPEC cursor_t &cursor_t::operator+=(size_t offset) {
  col += offset;
  return *this;
}

// ----------------------------------------------------------------------------

NS_DLLSPEC cursor_t operator+(cursor_t const &base, size_t offset) {
  cursor_t ret(base);
  ret.col += offset;
  return ret;
}

// ----------------------------------------------------------------------------

} // namespace ns2
