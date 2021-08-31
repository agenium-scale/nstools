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

#ifndef NS2_IDENTIFIER_NODE_HPP
#define NS2_IDENTIFIER_NODE_HPP

#include <string>

#include <ns2/ast/ast_node.hpp>
#include <ns2/ast/type.hpp>
#include <ns2/ast/var_decl.hpp>

namespace ns2 {
namespace ast {

/// Class for an identifier
class identifier_t : public expr_t {
private:
  lexer::lexeme_t *m_name;
  type_t m_type;
  var_decl_t *m_decl; // Reference to its declaration if exists

public:
  // Constructor identifier
  identifier_t() : m_name(NULL), m_type(), m_decl(NULL){};

  identifier_t(lexer::lexeme_t *name, const type_t &type);

  identifier_t(var_decl_t *decl);

  // Return the type
  type_t get_type() const;

  // Set the type
  void set_type(const type_t &t);

  // Set the name
  void set_name(lexer::lexeme_t *name);

  // Return the name of the var_decl
  lexer::lexeme_t *get_name() const;

  // Return the declaration node
  var_decl_t *get_decl() const;

  // Set the declaration node
  void set_decl(var_decl_t *node);

  // Method to print
  virtual std::string pprint() const;

  virtual std::string ast_print(std::string indent, bool last) const;
};

} // namespace ast
} // namespace ns2

#endif