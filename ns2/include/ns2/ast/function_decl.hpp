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

#ifndef NS2_FUNCTION_DECL_HPP
#define NS2_FUNCTION_DECL_HPP

#include <ns2/ast/ast_node.hpp>
#include <ns2/ast/compound_stmt.hpp>
#include <ns2/ast/param_var_decl.hpp>
#include <ns2/ast/type.hpp>

namespace ns2 {
namespace ast {

class function_decl_t : public ast_node_t {
protected:
  lexer::lexeme_t *m_name;
  type_t m_type;
  std::vector<param_var_decl_t *> m_args;
  compound_stmt_t *m_body;

public:
  // Constructors of funciton_decl
  function_decl_t() : m_name(), m_type(), m_args(), m_body(NULL) {}

  function_decl_t(const type_t &ret, lexer::lexeme_t *name,
                  const std::vector<param_var_decl_t *> &args);

  function_decl_t(const type_t &ret, lexer::lexeme_t *name,
                  const std::vector<param_var_decl_t *> &args,
                  compound_stmt_t *body);

  ~function_decl_t();

  // Return the type of this function_decl
  type_t get_type() const;

  // Set the type of this function_decl
  void set_type(const type_t &t);

  // Return the name of this function_decl
  lexer::lexeme_t *get_name() const;

  // Return the number of arguments
  int get_nb_args() const;

  void set_args(std::vector<param_var_decl_t *> const &args);

  // Set a new body
  void set_body(compound_stmt_t *body);

  compound_stmt_t *get_body() const;

  // Update the type of the decl if there are one candidate or more.
  void update_type();

  std::string pprint() const;

  std::string ast_print(std::string indent, bool last) const;
};

} // namespace ast
} // namespace ns2

#endif
