// MIT License
//
// Copyright (c) 2021 Agenium Scale
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

#include <cstring>

#include <ns2/parser/message.hpp>
#include <ns2/string.hpp>

namespace ns2 {
namespace parser {
namespace message {

// ----------------------------------------------------------------------------

std::string term_color(std::string const &str, enum color_t const c,
                       bool const bold) {

  if (ns2::startswith(ns2::to_string(getenv("TERM")), "xterm")) {
    std::string bold_str = (bold ? "1" : "0");
    if (c == COLOR_T_RED) {
      return "\033[" + bold_str + ";31m" + str + "\033[0m";
    } else if (c == COLOR_T_GREEN) {
      return "\033[" + bold_str + ";32m" + str + "\033[0m";
    } else if (c == COLOR_T_YELLOW) {
      return "\033[" + bold_str + ";33m" + str + "\033[0m";
    } else if (c == COLOR_T_WHITE) {
      return "\033[" + bold_str + ";37m" + str + "\033[0m";
    } else if (c == COLOR_T_BLUE) {
      return "\033[" + bold_str + ";34m" + str + "\033[0m";
    } else if (c == COLOR_T_MAGENTA) {
      return "\033[" + bold_str + ";35m" + str + "\033[0m";
    } else if (c == COLOR_T_CYAN) {
      return "\033[" + bold_str + ";36m" + str + "\033[0m";
    }
  }
  return str;
}

} // namespace message
} // namespace parser
} // namespace ns2
