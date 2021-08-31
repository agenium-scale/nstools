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

#ifndef NS2_PARSER_ANALYSER_HPP
#define NS2_PARSER_ANALYSER_HPP

#include <map>

#include <ns2/ast/type.hpp>
#include <ns2/lexer/lexer.hpp>

namespace ns2 {
namespace parser {
namespace analyser {

// ----------------------------------------------------------------------------

// Bind index position for each lexemes (Because of removing whitespace to
// simplify parsing analysis)
typedef std::vector<std::pair<size_t, lexer::lexeme_t *> >
    vector_index_lexeme_t;

// ----------------------------------------------------------------------------

struct view_t {
  int first, last;
  bool is_function;
  view_t();
};

// ----------------------------------------------------------------------------

// Get instruction from the right side to the left side
void get_instruction_left(vector_index_lexeme_t *lexs, int *index,
                          std::vector<lexer::lexeme_t *> const &lexemes);

// ----------------------------------------------------------------------------

std::vector<analyser::vector_index_lexeme_t> get_args_between(
    std::string const &left, std::string const &right,
    std::string const &separator, int const not_close_,
    analyser::vector_index_lexeme_t const &lp, size_t *i,
    std::map<std::pair<std::string, std::string>, int> other_betweens);

// ----------------------------------------------------------------------------

bool make_type_t(std::vector<analyser::vector_index_lexeme_t> const &args,
                 std::vector<ast::type_t> *types);

bool make_type_t(analyser::vector_index_lexeme_t const &arg, ast::type_t *type,
                 size_t *i);

// ----------------------------------------------------------------------------

} // namespace analyser
} // namespace parser
} // namespace ns2

#endif
