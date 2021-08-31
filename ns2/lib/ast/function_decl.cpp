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

#include <ns2/ast/function_decl.hpp>

using namespace ns2::parser;

namespace ns2 {
namespace ast {

function_decl_t::function_decl_t(const type_t &ret, lexer::lexeme_t *name,
                                 const std::vector<param_var_decl_t *> &args)
    : m_name(name), m_type(ret), m_args(args) {}

function_decl_t::function_decl_t(const type_t &ret, lexer::lexeme_t *name,
                                 const std::vector<param_var_decl_t *> &args,
                                 compound_stmt_t *body)
    : m_name(name), m_type(ret), m_args(args), m_body(body) {}

function_decl_t::~function_decl_t() { delete m_body; }

type_t function_decl_t::get_type() const { return m_type; }

void function_decl_t::set_type(const type_t &t) { m_type = t; }

lexer::lexeme_t *function_decl_t::get_name() const { return m_name; }

void function_decl_t::set_args(std::vector<param_var_decl_t *> const &args) {
  m_args = args;
}

int function_decl_t::get_nb_args() const { return int(m_args.size()); }

void function_decl_t::set_body(compound_stmt_t *body) {
  m_body = body;
  m_type = body->get_type();
}

compound_stmt_t *function_decl_t::get_body() const { return m_body; }

void function_decl_t::update_type() {
  // TODO
}

std::string function_decl_t::pprint() const {
  // TODO
  return "";
}

std::string function_decl_t::ast_print(std::string indent, bool last) const {
  indent += last ? "  " : "| ";
  std::string output =
      message::term_color("FunctionDecl", message::COLOR_T_GREEN) + " " +
      message::term_color(ns2::to_string(this), message::COLOR_T_YELLOW) + " " +
      message::term_color(this->get_position_string(),
                          message::COLOR_T_YELLOW) +
      " " +
      message::term_color("" + m_name->data + "", message::COLOR_T_CYAN, true) +
      " " +
      message::term_color("'" + m_type.pprint() + "'", message::COLOR_T_GREEN);
  for (size_t i = 0; i < m_args.size(); i++) {
    output += "\n" + indent + "|-" + m_args[i]->ast_print(indent, true);
  }
  output +=
      "\n" + indent + "`-" +
      (m_body != NULL ? m_body->ast_print(indent, true)
                      : message::term_color("<<NULL>>", message::COLOR_T_BLUE));
  return output;
}

} // namespace ast
} // namespace ns2