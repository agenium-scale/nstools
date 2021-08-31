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

#include <ns2/ast/for_stmt.hpp>

using namespace ns2::parser;

namespace ns2 {
namespace ast {

for_stmt_t::for_stmt_t(ast_node_t *init, ast_node_t *cond, ast_node_t *incr,
                       compound_stmt_t *body) {
  m_init = init;
  m_cond = cond;
  m_incr = incr;
  m_body = body;
}

ast_node_t *for_stmt_t::get_init() const { return m_init; }

ast_node_t *for_stmt_t::get_cond() const { return m_cond; }

ast_node_t *for_stmt_t::get_incr() const { return m_incr; }

compound_stmt_t *for_stmt_t::get_body() const { return m_body; }

void for_stmt_t::set_init(ast_node_t *init) { m_init = init; }

void for_stmt_t::set_cond(ast_node_t *cond) { m_cond = cond; }

void for_stmt_t::set_incr(ast_node_t *incr) { m_incr = incr; }

void for_stmt_t::set_body(compound_stmt_t *body) { m_body = body; }

std::string for_stmt_t::pprint() const {
  // TODO
  return "";
}

std::string for_stmt_t::ast_print(std::string, bool) const {
  // TODO
  return "";
}

} // namespace ast
} // namespace ns2