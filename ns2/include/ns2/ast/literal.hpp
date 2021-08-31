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

#ifndef NS2_LITERAL_NODE_HPP
#define NS2_LITERAL_NODE_HPP

#include <string>

#include <ns2/ast/ast_node.hpp>

namespace ns2 {
namespace ast {

enum literal_kind_t {
  LITERAL_KIND_STRING,
  LITERAL_KIND_NUMBER,
  LITERAL_KIND_BOOLEAN,
  LITERAL_KIND_NULL
};

std::string get_literal_string(const literal_kind_t &kind);

/// Class for an identifier
class literal_t : public expr_t {
private:
  lexer::lexeme_t *m_value;
  literal_kind_t m_kind;

public:
  // Constructor identifier
  literal_t() : m_value(NULL), m_kind(LITERAL_KIND_NULL) {
    m_is_literal = true;
  };

  literal_t(lexer::lexeme_t *value, const literal_kind_t &kind);

  // Set the name
  void set_value(lexer::lexeme_t *value);

  // Return the name of the var_decl
  lexer::lexeme_t *get_value() const;

  // Return the name of the var_decl
  literal_kind_t get_kind() const;

  void set_kind(const literal_kind_t &kind);

  type_t get_type() const;

  // Method to print
  std::string pprint() const;

  std::string ast_print(std::string indent, bool last) const;
};

} // namespace ast
} // namespace ns2

#endif