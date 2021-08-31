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

#include <ns2/ast/literal.hpp>

using namespace ns2::parser;

namespace ns2 {
namespace ast {

std::string get_literal_string(const literal_kind_t &kind) {
  switch (kind) {
  case LITERAL_KIND_STRING:
    return "String";
  case LITERAL_KIND_NUMBER:
    return "Number";
  case LITERAL_KIND_BOOLEAN:
    return "Boolean";
  case LITERAL_KIND_NULL:
    return "NULL";
  default:
    return "";
  }
}

literal_t::literal_t(lexer::lexeme_t *value, const literal_kind_t &kind) {
  m_value = value;
  m_kind = kind;
  m_is_literal = true;
}

void literal_t::set_value(lexer::lexeme_t *value) { m_value = value; }

lexer::lexeme_t *literal_t::get_value() const { return m_value; }

literal_kind_t literal_t::get_kind() const { return m_kind; }

void literal_t::set_kind(const literal_kind_t &kind) { m_kind = kind; }

type_t literal_t::get_type() const { return unknown_type(); }

std::string literal_t::pprint() const {
  return m_value != NULL ? m_value->data : "";
}

std::string literal_t::ast_print(std::string indent, bool last) const {
  indent += last ? "  " : "| ";
  std::string output =
      message::term_color("Literal", message::COLOR_T_MAGENTA) + " " +
      message::term_color(ns2::to_string(this), message::COLOR_T_YELLOW) + " " +
      message::term_color(this->get_position_string(),
                          message::COLOR_T_YELLOW) +
      " " +
      message::term_color("'" + get_literal_string(m_kind) + "'",
                          message::COLOR_T_GREEN, true) +
      " " +
      message::term_color("" + ns2::pprint(m_data) + "", message::COLOR_T_CYAN,
                          true);
  return output;
}

} // namespace ast
} // namespace ns2
