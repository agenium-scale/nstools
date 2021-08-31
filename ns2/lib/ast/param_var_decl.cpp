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

#include <ns2/ast/param_var_decl.hpp>

using namespace ns2::parser;

namespace ns2 {
namespace ast {

param_var_decl_t::param_var_decl_t() : var_decl_t(), m_func(NULL) {}

param_var_decl_t::param_var_decl_t(const type_t &t, lexer::lexeme_t *name,
                                   ast_node_t *func, ast_node_t *init) {
  m_name = name;
  m_type = t;
  m_init = init;
  m_func = func;
}

std::string param_var_decl_t::pprint() const {
  // TODO
  return "";
}

std::string param_var_decl_t::ast_print(std::string indent, bool last) const {
  indent += last ? "  " : "| ";
  std::string output =
      message::term_color("ParamVarDecl", message::COLOR_T_GREEN) + " " +
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

} // namespace ast
} // namespace ns2