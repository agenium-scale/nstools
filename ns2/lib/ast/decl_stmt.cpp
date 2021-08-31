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

#include <ns2/ast/decl_stmt.hpp>
#include <ns2/ast/type.hpp>

using namespace ns2::parser;

namespace ns2 {
namespace ast {

type_t decl_stmt_t::get_type() const {
  if (m_decls.size() > 0) {
    return m_decls[0]->get_type();
  } else {
    return unknown_type();
  }
}

std::string decl_stmt_t::pprint() const {
  // TODO
  return "";
}

std::string decl_stmt_t::ast_print(std::string indent, bool last) const {
  indent += last ? "  " : "| ";
  if (m_decls.size() <= 0) {
    return "";
  } else {
    std::string output =
        message::term_color("DeclStmt", message::COLOR_T_MAGENTA) + " " +
        message::term_color(ns2::to_string(this), message::COLOR_T_YELLOW) +
        " " +
        message::term_color(this->get_position_string(),
                            message::COLOR_T_YELLOW);
    for (size_t i = 0; i < m_decls.size() - 1; i++) {
      output += "\n" + indent + "|-" + m_decls[i]->ast_print(indent, false);
    }
    output += "\n" + indent + "`-" +
              m_decls[m_decls.size() - 1]->ast_print(indent, true);
    return output;
  }
}

} // namespace ast
} // namespace ns2