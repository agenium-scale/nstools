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

#include <ns2/ast/struct_decl.hpp>

using namespace ns2::parser;

namespace ns2 {
namespace ast {

struct_decl_t::struct_decl_t() {
  // TODO
}

struct_decl_t::struct_decl_t(const type_t & /*t*/, lexer::lexeme_t * /*name*/,
                             ast_node_t * /*init*/) {
  // TODO
}

struct_decl_t::struct_decl_t(const type_t & /*t*/, lexer::lexeme_t * /*name*/,
                             const int & /*p_start*/, const int & /*p_end*/,
                             ast_node_t * /*init*/) {
  // TODO
}

void struct_decl_t::set_init(ast_node_t * /*init*/) {
  // TODO
}

ast_node_t *struct_decl_t::get_init() const {
  // TODO
  return NULL;
}

void struct_decl_t::set_type(const type_t & /*t*/) {
  // TODO
}

type_t struct_decl_t::get_type() const { return m_type; }

void struct_decl_t::set_name(lexer::lexeme_t * /*name*/) {
  // TODO
}

lexer::lexeme_t *struct_decl_t::get_name() const {
  // TODO
  return NULL;
}

std::string struct_decl_t::pprint() const {
  // TODO
  return "";
}

std::string struct_decl_t::ast_print(std::string /*indent*/,
                                     bool /*last*/) const {
  // TODO
  return "";
}

void struct_decl_t::update_type() {}

} // namespace ast
} // namespace ns2