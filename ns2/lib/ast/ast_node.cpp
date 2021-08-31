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

#include <ns2/ast/ast_node.hpp>

using namespace ns2::parser;

namespace ns2 {
namespace ast {

ast_node_t::ast_node_t() {
  m_parent = NULL;
  m_visited = false;
  pos_start = -1;
  pos_end = -1;
  m_is_literal = false;
}

ast_node_t::ast_node_t(const std::vector<lexer::lexeme_t *> &d, ast_node_t *p)
    : m_parent(p), m_data(d) {
  m_visited = false;
  pos_start = -1;
  pos_end = -1;
  m_is_literal = false;
}

ast_node_t::~ast_node_t() {}

bool ast_node_t::is_visited() const { return this->m_visited; }

void ast_node_t::set_visited(bool v) { this->m_visited = v; }

bool ast_node_t::is_literal() const { return this->m_is_literal; }

void ast_node_t::set_is_literal(bool l) { this->m_is_literal = l; }

void ast_node_t::set_data(const std::vector<lexer::lexeme_t *> &data) {
  m_data = data;
}

std::vector<lexer::lexeme_t *> ast_node_t::get_data() const { return m_data; }

std::string ast_node_t::get_position_string() const {
  size_t size = m_data.size();
  if (size > 0) {
    std::string pos = "<line:" + ns2::to_string(m_data[0]->lineno) + ":" +
                      ns2::to_string(m_data[0]->col) + ">";
    return pos;
  } else {
    return "<>";
  }
}

std::string ast_node_t::pprint() const { return ns2::pprint(m_data); }

std::string ast_node_t::ast_print(std::string indent, bool last) const {
  indent += last ? "  " : "| ";
  std::string output =
      message::term_color(m_is_literal ? "AstNode" : "Literal",
                          message::COLOR_T_MAGENTA) +
      " " + message::term_color(ns2::to_string(this), message::COLOR_T_YELLOW) +
      " " +
      message::term_color(this->get_position_string(),
                          message::COLOR_T_YELLOW) +
      " " +
      message::term_color("" + ns2::pprint(m_data) + "", message::COLOR_T_CYAN,
                          true);
  return output;
}

std::string expr_t::ast_print(std::string indent, bool last) const {
  indent += last ? "  " : "| ";
  std::string output =
      message::term_color(m_is_literal ? "ExprNode" : "Literal",
                          message::COLOR_T_MAGENTA) +
      " " + message::term_color(ns2::to_string(this), message::COLOR_T_YELLOW) +
      " " +
      message::term_color(this->get_position_string(),
                          message::COLOR_T_YELLOW) +
      " " +
      message::term_color("" + ns2::pprint(m_data) + "", message::COLOR_T_CYAN,
                          true);
  return output;
}

} // namespace ast

void ast_print(const std::vector<ast::ast_node_t *> &ast) {
  for (size_t i = 0; i < ast.size(); i++) {
    if (ast[i] != NULL)
      std::cout << "-" << ast[i]->ast_print("", true) << "\n";
  }
}

} // namespace ns2
