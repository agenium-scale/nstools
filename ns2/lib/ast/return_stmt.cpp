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

#include <ns2/ast/return_stmt.hpp>

using namespace ns2::parser;

namespace ns2 {
namespace ast {

return_stmt_t::return_stmt_t(ast_node_t *expr, const type_t &type) {
  m_expr = expr;
  m_type = type;
}

// TODO: improve
type_t return_stmt_t::get_type() const { return m_type; }

ast_node_t *return_stmt_t::get_expr() const { return m_expr; }

void return_stmt_t::set_expr(ast_node_t *expr) { m_expr = expr; }

std::string return_stmt_t::pprint() const {
  // TODO
  return "return " + m_expr->pprint();
}

std::string return_stmt_t::ast_print(std::string indent, bool last) const {
  indent += last ? "  " : "| ";
  std::string output =
      message::term_color("ReturnStmt", message::COLOR_T_MAGENTA) + " " +
      message::term_color(ns2::to_string(this), message::COLOR_T_YELLOW) + " " +
      message::term_color(this->get_position_string(), message::COLOR_T_YELLOW);
  if (m_expr != NULL) {
    output += "\n" + indent + " `-" + m_expr->ast_print(indent, true);
  }
  return output;
}

} // namespace ast
} // namespace ns2
