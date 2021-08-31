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

#ifndef NS2_CALLEXPR_HPP
#define NS2_CALLEXPR_HPP

#include <string>
#include <vector>

#include <ns2/ast/function_decl.hpp>

namespace ns2 {
namespace ast {

// Class callexpr node
class callexpr_t : public expr_t {
protected:
  lexer::lexeme_t *m_name;
  std::vector<ast_node_t *> m_args;
  function_decl_t *m_decl;
  type_t m_type;

public:
  callexpr_t(lexer::lexeme_t *name, const std::vector<ast_node_t *> &args,
             type_t const &type = void_type());

  callexpr_t(lexer::lexeme_t *name, const std::vector<ast_node_t *> &args,
             function_decl_t *decl, type_t const &type = void_type());

  ~callexpr_t();

  type_t get_type() const;

  lexer::lexeme_t *get_name() const;

  function_decl_t *get_decl() const;

  std::vector<ast_node_t *> get_args() const;

  ast_node_t *get_arg(const size_t &) const;

  void set_args(const std::vector<ast_node_t *> &);

  void set_decl(function_decl_t *);

  void set_type(const type_t &t);

  void add_arg(ast_node_t *);

  void remove_arg(const int &index);

  std::string pprint() const;

  std::string ast_print(std::string indent, bool last) const;
};

} // namespace ast
} // namespace ns2

#endif
