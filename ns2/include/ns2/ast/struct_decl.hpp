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

#ifndef NS2_STRUCT_DECL_HPP
#define NS2_STRUCT_DECL_HPP

#include <ns2/ast/ast_node.hpp>

namespace ns2 {
namespace ast {

// Class var_decl_t node
class struct_decl_t : public ast_node_t {
protected:
  lexer::lexeme_t *m_name;
  type_t m_type;
  ast_node_t *m_init;

public:
  // Constructors
  struct_decl_t();

  struct_decl_t(const type_t &t, lexer::lexeme_t *name,
                ast_node_t *init = NULL);

  struct_decl_t(const type_t &t, lexer::lexeme_t *name, const int &p_start,
                const int &p_end, ast_node_t *init = NULL);

  // Set a new initialisation
  void set_init(ast_node_t *init);

  // Return a pointer to the initialisation of the var_decl_t
  ast_node_t *get_init() const;

  // Set a new type
  void set_type(const type_t &t);

  // Return the type of the var_decl_t
  type_t get_type() const;

  // Set a new name
  void set_name(lexer::lexeme_t *name);

  // Return the name of the var_decl_t
  lexer::lexeme_t *get_name() const;

  // Method to print a var_decl_t
  std::string pprint() const;

  std::string ast_print(std::string indent, bool last) const;

  // Update the type of the decl if there are one candidate or more.
  void update_type();
};

} // namespace ast
} // namespace ns2

#endif