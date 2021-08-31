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

#ifndef NS2_PARSER_PATTERN_MATCHING_HPP
#define NS2_PARSER_PATTERN_MATCHING_HPP

#include <map>
#include <vector>

#include <ns2/ast/ast_node.hpp>
#include <ns2/ast/binary_operator.hpp>
#include <ns2/ast/callexpr.hpp>
#include <ns2/ast/compound_stmt.hpp>
#include <ns2/ast/decl_stmt.hpp>
#include <ns2/ast/for_stmt.hpp>
#include <ns2/ast/function_decl.hpp>
#include <ns2/ast/literal.hpp>
#include <ns2/ast/unary_operator.hpp>
#include <ns2/parser/analyser.hpp>

namespace ns2 {
namespace parser {
namespace match {

// ----------------------------------------------------------------------------

bool is_number(std::string const &str);

bool is_identifier(std::string const &var);

// ----------------------------------------------------------------------------

bool is_var_declaration_keyword(std::string const &lex);

// ----------------------------------------------------------------------------

bool is_keyword(std::string const &lex);

bool is_operator(lexer::lexeme_t::kind_t const &k);

// bool is_unary_operator(lexer::lexeme_t::kind_t const &k);

// ----------------------------------------------------------------------------

// Declaration Pattern
bool is_var_declaration(size_t *position,
                        parser::analyser::vector_index_lexeme_t const &lp,
                        std::vector<ns2::lexer::lexeme_t *> const &lexemes,
                        ns2::ast::ast_node_t **node);
bool is_func_declaration(size_t *positions,
                         parser::analyser::vector_index_lexeme_t const &lp,
                         std::vector<ns2::lexer::lexeme_t *> const &lexemes,
                         ns2::ast::function_decl_t **node);
bool is_func_template_declaration(std::vector<size_t> *positions,
                                  analyser::vector_index_lexeme_t const &lp);
bool is_struct_declaration(std::vector<size_t> *positions,
                           analyser::vector_index_lexeme_t const &lp);
bool is_param_declaration(std::vector<size_t> *positions,
                          analyser::vector_index_lexeme_t const &lp);

// ----------------------------------------------------------------------------

// Assignement Pattern
bool is_assignement(size_t *var_position,
                    analyser::vector_index_lexeme_t const &lp);

// Expression Pattern
bool is_expr(size_t *position, analyser::vector_index_lexeme_t const &lp,
             std::vector<ns2::lexer::lexeme_t *> const &lexemes,
             ns2::ast::expr_t **node);
bool is_callexpr(size_t *position, analyser::vector_index_lexeme_t const &lp,
                 std::vector<ns2::lexer::lexeme_t *> const &lexemes,
                 ns2::ast::expr_t **node);
bool is_lambdaexpr(size_t *position, analyser::vector_index_lexeme_t const &lp);
bool is_binary_op(size_t *position, analyser::vector_index_lexeme_t const &lp,
                  std::vector<ns2::lexer::lexeme_t *> const &lexemes,
                  ns2::ast::expr_t **node);
bool is_unary_op(size_t *position, analyser::vector_index_lexeme_t const &lp,
                 std::vector<ns2::lexer::lexeme_t *> const &lexemes,
                 ns2::ast::expr_t **node);
// ----------------------------------------------------------------------------

bool is_compound_stmt(size_t *position,
                      analyser::vector_index_lexeme_t const &lp,
                      std::vector<ns2::lexer::lexeme_t *> const &lexemes,
                      ns2::ast::compound_stmt_t **node, int nb_lcurly = 0);
bool is_return_stmt(size_t *position, analyser::vector_index_lexeme_t const &lp,
                    std::vector<ns2::lexer::lexeme_t *> const &lexemes,
                    ns2::ast::return_stmt_t **node);
bool is_for_stmt(size_t *position, analyser::vector_index_lexeme_t const &lp,
                 std::vector<ns2::lexer::lexeme_t *> const &lexemes,
                 ns2::ast::for_stmt_t **node);
bool is_while_stmt(size_t *position, analyser::vector_index_lexeme_t const &lp,
                   std::vector<ns2::lexer::lexeme_t *> const &lexemes,
                   ns2::ast::for_stmt_t **node);

// ----------------------------------------------------------------------------

bool is_instruction_block(lexer::lexeme_t *lex);

// ----------------------------------------------------------------------------

bool is_integer_b10(std::string const &str);

// ----------------------------------------------------------------------------

bool is_integer_b16(std::string const &str);

// ----------------------------------------------------------------------------

} // namespace match
} // namespace parser
} // namespace ns2

#endif
