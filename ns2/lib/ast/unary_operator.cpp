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

#include <ns2/ast/unary_operator.hpp>

using namespace ns2::parser;

namespace ns2 {
namespace ast {

unary_op_t::unary_op_t(ast_node_t *left, const up_kind_t &kind,
                       type_t const &ret_type, bool after)
    : m_lhs(left), m_kind_op(kind), m_after(after), m_ret_type(ret_type) {}

// Return the type of this unary_op
type_t unary_op_t::get_type() const { return m_ret_type; }

// Set a new type
void unary_op_t::set_type(const type_t &t) {
  // m_ret_type.set_name_type(t.get_name_type());
  m_ret_type.name = t.name;
}

// Update the type of the decl if there are one candidate or more.
void unary_op_t::update_type() {
  // TODO
}

// Method to print a binary_op node
std::string unary_op_t::pprint() const {
  // TODO
  return "";
}

std::string unary_op_t::ast_print(std::string, bool) const {
  // TODO
  return "";
}

} // namespace ast
} // namespace ns2