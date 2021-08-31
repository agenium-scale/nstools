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

#include <ns2/ast/compound_stmt.hpp>

using namespace ns2::parser;

namespace ns2 {
namespace ast {

compound_stmt_t::compound_stmt_t(const std::vector<ast_node_t *> &body)
    : m_body(body) {
  if (body.size() == 0) {
    m_type = void_type();
  } else {
    ast_node_t *last_stmt = body.back();
    // TODO improve
    if (return_stmt_t *ret = dynamic_cast<return_stmt_t *>(last_stmt)) {
      m_type = ret->get_type();
    } else {
      m_type = void_type();
    }
  }
}

type_t compound_stmt_t::get_type() const { return m_type; }

std::vector<ast_node_t *> compound_stmt_t::get_body() const { return m_body; }

void compound_stmt_t::add_node(ast_node_t *new_node) {
  m_body.push_back(new_node);
}

void compound_stmt_t::remove_node(const int &index) {
  m_body.erase(m_body.begin() + index);
}

std::string compound_stmt_t::pprint() const {
  // TODO
  return "";
}

std::string compound_stmt_t::ast_print(std::string indent, bool last) const {
  indent += last ? "  " : "| ";
  std::string output =
      message::term_color("CompoundStmt", message::COLOR_T_MAGENTA) + " " +
      message::term_color(ns2::to_string(this), message::COLOR_T_YELLOW) + " " +
      message::term_color(this->get_position_string(), message::COLOR_T_YELLOW);
  if (m_body.size() >= 1) {
    for (size_t i = 0; i < m_body.size() - 1; i++) {
      output += "\n" + indent + "|-" +
                (m_body[i] != NULL
                     ? m_body[i]->ast_print(indent, false)
                     : message::term_color("<<NULL>>", message::COLOR_T_BLUE));
    }
    output += "\n" + indent + "`-" +
              (m_body[m_body.size() - 1] != NULL
                   ? m_body[m_body.size() - 1]->ast_print(indent, true)
                   : message::term_color("<<NULL>>", message::COLOR_T_BLUE));
  }
  return output;
}

} // namespace ast
} // namespace ns2
