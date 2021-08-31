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

#ifndef NS2_UNARY_OP_HPP
#define NS2_UNARY_OP_HPP

#include <ns2/ast/ast_node.hpp>

namespace ns2 {
namespace ast {

enum up_kind_t {
  UP_KIND_2ADD,
  UP_KIND_2SUB,
  UP_KIND_ADD,
  UP_KIND_SUB,
  UP_KIND_REF,
  UP_KIND_POINTER,
  UP_KIND_NOTL,
  UP_KIND_NOTB
};

class unary_op_t : public expr_t {
private:
  ast_node_t *m_lhs;
  up_kind_t m_kind_op;
  bool m_after;
  type_t m_ret_type; // Used to determine the type of data used.

public:
  unary_op_t(ast_node_t *left, const up_kind_t &kind, type_t const &ret_type,
             bool after = false);

  type_t get_type() const;

  void set_type(const type_t &t);

  void update_type();

  std::string pprint() const;

  std::string ast_print(std::string indent, bool last) const;
};

} // namespace ast
} // namespace ns2

#endif
