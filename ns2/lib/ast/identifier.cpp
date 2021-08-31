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

#include <ns2/ast/identifier.hpp>

using namespace ns2::parser;

namespace ns2 {
namespace ast {

identifier_t::identifier_t(lexer::lexeme_t *name, const type_t &type)
    : m_name(name), m_type(type) {
  m_decl = NULL;
}

identifier_t::identifier_t(var_decl_t *decl) {
  m_name = decl->get_name();
  m_type = decl->get_type();
  m_decl = decl;
}

type_t identifier_t::get_type() const { return m_type; }

void identifier_t::set_type(const type_t &t) { m_type = t; }

void identifier_t::set_name(lexer::lexeme_t *name) { m_name = name; }

lexer::lexeme_t *identifier_t::get_name() const { return m_name; }

var_decl_t *identifier_t::get_decl() const { return m_decl; }

void identifier_t::set_decl(var_decl_t *node) {
  try {
    if (node != NULL) {
      m_decl = node;
    } else {
    }
  } catch (...) {
  }
}

std::string identifier_t::pprint() const { return this->get_name()->data; }

std::string identifier_t::ast_print(std::string, bool) const {
  std::string output =
      message::term_color("Identifier", message::COLOR_T_MAGENTA) + " " +
      message::term_color(ns2::to_string(this), message::COLOR_T_YELLOW) + " " +
      message::term_color(this->get_position_string(),
                          message::COLOR_T_YELLOW) +
      " " +
      message::term_color("'" + m_type.pprint() + "'", message::COLOR_T_GREEN);
  if (m_decl != NULL) {
    output +=
        " ref " + message::term_color(" Var ", message::COLOR_T_GREEN) +
        message::term_color(ns2::to_string(m_decl), message::COLOR_T_YELLOW) +
        " " +
        message::term_color("'" + m_decl->get_name()->data + "'",
                            message::COLOR_T_CYAN) +
        " " +
        message::term_color("'" + m_decl->get_type().pprint() + "'",
                            message::COLOR_T_GREEN);
  } else {
    output += message::term_color(", Var ", message::COLOR_T_GREEN) + " " +
              message::term_color("???", message::COLOR_T_CYAN);
  }
  return output;
}

} // namespace ast
} // namespace ns2