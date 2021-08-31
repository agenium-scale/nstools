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

#include <ns2/ast/binary_operator.hpp>

using namespace ns2::parser;

namespace ns2 {
namespace ast {

binary_op_t::binary_op_t() {}

binary_op_t::binary_op_t(ast_node_t *left, ast_node_t *right,
                         const op_kind_t &kind, const type_t &ret_type)
    : m_left(left), m_right(right), m_kind_op(kind), m_ret_type(ret_type) {}

binary_op_t::~binary_op_t() {}

ast_node_t *binary_op_t::get_left() const { return m_left; }

ast_node_t *binary_op_t::get_right() const { return m_right; }

op_kind_t binary_op_t::get_kind_op() const { return m_kind_op; }

type_t binary_op_t::get_type() const { return m_ret_type; }

std::string binary_op_t::pprint() const {
  // TODO
  return "";
}

std::string binary_op_t::ast_print(std::string indent, bool last) const {
  std::string output =
      message::term_color("BinaryOperator", message::COLOR_T_MAGENTA) + " " +
      message::term_color(ns2::to_string(this), message::COLOR_T_YELLOW) + " " +
      message::term_color(this->get_position_string(),
                          message::COLOR_T_YELLOW) +
      " operator " +
      message::term_color("'" + get_op_string(m_kind_op) + "'",
                          message::COLOR_T_CYAN);
  indent += last ? "  " : "| ";
  output += "\n" + indent + "|-" + m_left->ast_print(indent, false);
  output += "\n" + indent + "`-" + m_right->ast_print(indent, true);
  return output;
}

std::string get_op_string(const op_kind_t &op) {
  switch (op) {
  case OP_KIND_ADD:
    return "+";
  case OP_KIND_ASSIGN:
    return "=";
  case OP_KIND_SUB:
    return "-";
  case OP_KIND_MUL:
    return "*";
  case OP_KIND_DIV:
    return "/";
  case OP_KIND_OR:
    return "|";
  case OP_KIND_AND:
    return "&";
  case OP_KIND_XOR:
    return "^";
  case OP_KIND_RSHIFT:
    return ">>";
  case OP_KIND_LSHIFT:
    return "<<";
  case OP_KIND_NOT:
    return "~";
  case OP_KIND_ANDL:
    return "&&";
  case OP_KIND_ORL:
    return "||";
  case OP_KIND_EQ:
    return "==";
  case OP_KIND_NEQ:
    return "!=";
  case OP_KIND_LT:
    return "<";
  case OP_KIND_GT:
    return ">";
  case OP_KIND_LEQ:
    return "<=";
  case OP_KIND_GEQ:
    return ">=";
  default:
    return "???";
  }
}

} // namespace ast
} // namespace ns2