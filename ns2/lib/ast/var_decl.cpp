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

#include <ns2/ast/var_decl.hpp>

using namespace ns2::parser;

namespace ns2 {
namespace ast {

var_decl_t::var_decl_t() : m_name(NULL), m_type(), m_init(NULL) {}

var_decl_t::var_decl_t(const type_t &t, lexer::lexeme_t *name, ast_node_t *init)
    : m_name(name), m_type(t), m_init(init) {}

var_decl_t::var_decl_t(const type_t &t, lexer::lexeme_t *name,
                       const int &p_start, const int &p_end, ast_node_t *init)
    : m_name(name), m_type(t), m_init(init) {
  pos_start = p_start;
  pos_end = p_end;
}

void var_decl_t::set_init(ast_node_t *init) { m_init = init; }

ast_node_t *var_decl_t::get_init() const { return m_init; }

void var_decl_t::set_type(const type_t &t) {
  // m_type.set_name_type(t.get_name_type());
  m_type = t;
}

type_t var_decl_t::get_type() const { return m_type; }

void var_decl_t::set_name(lexer::lexeme_t *name) { m_name = name; }

lexer::lexeme_t *var_decl_t::get_name() const { return m_name; }

std::string var_decl_t::pprint() const {
  // TODO
  return "";
}

std::string var_decl_t::ast_print(std::string indent, bool last) const {
  indent += last ? "  " : "| ";
  std::string output =
      message::term_color("VarDecl", message::COLOR_T_GREEN) + " " +
      message::term_color(ns2::to_string(this), message::COLOR_T_YELLOW) + " " +
      message::term_color(this->get_position_string(),
                          message::COLOR_T_YELLOW) +
      " " +
      message::term_color("" + m_name->data + "", message::COLOR_T_CYAN, true) +
      " " +
      message::term_color("'" + m_type.pprint() + "'", message::COLOR_T_GREEN);
  if (m_init != NULL) {
    output += "cinit\n" + indent + " `-" + m_init->ast_print(indent, true);
  }
  return output;
}

void var_decl_t::update_type() {
  // TODO
}

} // namespace ast
} // namespace ns2