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

#ifndef NS2_BINARY_NODE_HPP
#define NS2_BINARY_NODE_HPP

#include <ns2/ast/ast_node.hpp>
#include <ns2/ast/type.hpp>

namespace ns2 {
namespace ast {

enum op_kind_t {
  OP_KIND_ADD,
  OP_KIND_ASSIGN,
  OP_KIND_SUB,
  OP_KIND_MUL,
  OP_KIND_DIV,
  OP_KIND_OR,
  OP_KIND_AND,
  OP_KIND_XOR,
  OP_KIND_RSHIFT,
  OP_KIND_LSHIFT,
  OP_KIND_NOT,
  // logical operations
  OP_KIND_ANDL,
  OP_KIND_ORL,
  OP_KIND_EQ,
  OP_KIND_NEQ,
  OP_KIND_LT,
  OP_KIND_GT,
  OP_KIND_LEQ,
  OP_KIND_GEQ
};

class binary_op_t : public expr_t {
private:
  ast_node_t *m_left;
  ast_node_t *m_right;
  op_kind_t m_kind_op;
  type_t m_ret_type;

public:
  binary_op_t();

  binary_op_t(ast_node_t *left, ast_node_t *right, const op_kind_t &kind,
              const type_t &ret_type);

  ~binary_op_t();

  ast_node_t *get_left() const;

  void set_left(ast_node_t *);

  ast_node_t *get_right() const;

  void set_right(ast_node_t *);

  op_kind_t get_kind_op() const;

  type_t get_type() const;

  std::string pprint() const;

  std::string ast_print(std::string indent, bool last) const;
};

std::string get_op_string(const op_kind_t &op);

} // namespace ast
} // namespace ns2

#endif
