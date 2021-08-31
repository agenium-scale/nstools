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

#ifndef NS2_PARSER_MESSAGE_HPP
#define NS2_PARSER_MESSAGE_HPP

#include <string>

namespace ns2 {
namespace parser {
namespace message {

enum color_t {
  COLOR_T_WHITE,
  COLOR_T_RED,
  COLOR_T_GREEN,
  COLOR_T_YELLOW,
  COLOR_T_BLUE,
  COLOR_T_MAGENTA,
  COLOR_T_CYAN
};

std::string term_color(std::string const &str, color_t const c,
                       bool const bold = true);

} // namespace message
} // namespace parser
} // namespace ns2

#endif
