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

#ifndef NS2_AST_NODE_HPP
#define NS2_AST_NODE_HPP

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <ns2/ast/type.hpp>
#include <ns2/lexer/lexer.hpp>
#include <ns2/parser/message.hpp>
#include <ns2/string.hpp>

namespace ns2 {
namespace ast {

class ast_node_t {
protected:
  ast_node_t *m_parent;
  std::vector<lexer::lexeme_t *> m_data;
  bool m_visited;
  bool m_is_literal;

public:
  int pos_start;
  int pos_end;

  ast_node_t();

  ast_node_t(const std::vector<lexer::lexeme_t *> &data, ast_node_t *p = NULL);

  virtual ~ast_node_t();

  bool is_visited() const;

  void set_visited(bool v);

  bool is_literal() const;

  void set_is_literal(bool l);

  void set_data(const std::vector<lexer::lexeme_t *> &d);

  std::vector<lexer::lexeme_t *> get_data() const;

  void set_parent(ast_node_t *parent);

  std::string get_position_string() const;

  virtual std::string pprint() const;

  virtual std::string ast_print(std::string indent, bool last) const;
};

class decl_t : public ast_node_t {
public:
  decl_t() : ast_node_t(){};

  virtual ~decl_t(){};

  virtual std::string pprint() const = 0;
};

class expr_t : public ast_node_t {
public:
  expr_t() : ast_node_t(){};

  expr_t(const std::vector<lexer::lexeme_t *> &data, ast_node_t *p = NULL)
      : ast_node_t(data, p){};

  virtual ~expr_t(){};

  virtual type_t get_type() const = 0;

  virtual std::string ast_print(std::string, bool) const;
};

} // namespace ast

void ast_print(const std::vector<ast::ast_node_t *> &ast);

} // namespace ns2

#endif
