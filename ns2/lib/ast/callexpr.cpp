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

#include <ns2/ast/callexpr.hpp>

using namespace ns2::parser;

namespace ns2 {
namespace ast {

callexpr_t::callexpr_t(lexer::lexeme_t *name,
                       const std::vector<ast_node_t *> &args,
                       type_t const &type)
    : m_name(name), m_args(args), m_type(type) {
  m_decl = NULL;
}

callexpr_t::callexpr_t(lexer::lexeme_t *name,
                       const std::vector<ast_node_t *> &args,
                       function_decl_t *decl, type_t const &type)
    : m_name(name), m_args(args), m_type(type) {
  m_decl = decl;
}

callexpr_t::~callexpr_t() { delete m_decl; }

type_t callexpr_t::get_type() const { return m_type; }

lexer::lexeme_t *callexpr_t::get_name() const { return m_name; }

std::vector<ast_node_t *> callexpr_t::get_args() const { return m_args; }

function_decl_t *callexpr_t::get_decl() const { return m_decl; }

ast_node_t *callexpr_t::get_arg(const size_t &index) const {
  if (m_args.size() < index || index <= 0) {
    return NULL;
  } else {
    return m_args[index];
  }
}

void callexpr_t::set_args(const std::vector<ast_node_t *> &args) {
  m_args = args;
}

void callexpr_t::set_decl(function_decl_t *decl) { m_decl = decl; }

void callexpr_t::set_type(const type_t &t) { m_type = t; }

void callexpr_t::add_arg(ast_node_t *arg) { m_args.push_back(arg); }

void callexpr_t::remove_arg(const int &index) {
  if (m_args.size() > size_t(index)) {
    m_args.erase(m_args.begin() + index);
  }
}

std::string callexpr_t::pprint() const {
  // TODO
  return "";
}

std::string callexpr_t::ast_print(std::string indent, bool last) const {
  indent += last ? "  " : "| ";
  std::string position = m_decl != NULL ? m_decl->get_position_string()
                                        : std::string("<line:?, line:?>");
  std::string output =
      message::term_color("CallExpr", message::COLOR_T_GREEN) + " " +
      message::term_color(ns2::to_string(this), message::COLOR_T_YELLOW) + " " +
      message::term_color(this->get_position_string(),
                          message::COLOR_T_YELLOW) +
      " " +
      message::term_color("'" + m_type.pprint() + "'", message::COLOR_T_GREEN) +
      " ref " + message::term_color(" Function ", message::COLOR_T_GREEN) +
      " " + message::term_color(position, message::COLOR_T_YELLOW) + " " +
      message::term_color(ns2::to_string(m_name->data), message::COLOR_T_CYAN);
  if (m_args.size() > 0) {
    for (size_t i = 0; i < m_args.size() - 1; i++) {
      output += "\n" + indent + "|-" + m_args[i]->ast_print(indent, false);
    }
    output += "\n" + indent + "`-" +
              m_args[m_args.size() - 1]->ast_print(indent, true);
  }
  return output;
}

} // namespace ast
} // namespace ns2