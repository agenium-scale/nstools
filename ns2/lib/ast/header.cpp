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

#include <ns2/ast/header.hpp>

using namespace ns2::parser;

namespace ns2 {
namespace ast {

header_t::header_t() {}

header_t::header_t(const std::string &name) : m_name(name) {}

// Set a new name
void header_t::set_name(const std::string &name) { m_name = name; }

// Return the name of the header_t
std::string header_t::get_name() const { return m_name; }

// Method to print a header_t
std::string header_t::pprint() const {
  // TODO
  return "";
}

std::string header_t::ast_print(std::string indent, bool last) const {
  indent += last ? "  " : "| ";
  std::string output =
      message::term_color("Header", message::COLOR_T_MAGENTA) + " " +
      message::term_color(ns2::to_string(this), message::COLOR_T_YELLOW) + " " +
      message::term_color(this->get_position_string(),
                          message::COLOR_T_YELLOW) +
      " " + message::term_color("" + m_name + "", message::COLOR_T_CYAN, true);
  return output;
}

} // namespace ast
} // namespace ns2
