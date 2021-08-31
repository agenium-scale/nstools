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

#ifndef NS2_PARSER_SKIP_HPP
#define NS2_PARSER_SKIP_HPP

#include <ns2/parser/analyser.hpp>

namespace ns2 {
namespace parser {
namespace skip {

// ----------------------------------------------------------------------------

// Skip all lexemes between two patterns
void skip_between(size_t *index, std::string const &first,
                  std::string const &last,
                  std::vector<lexer::lexeme_t *> const &lexemes);

void skip_between(size_t *index, std::string const &first,
                  std::string const &last,
                  analyser::vector_index_lexeme_t const &lexemes);

// ----------------------------------------------------------------------------

void skip_separator(size_t *i, std::vector<lexer::lexeme_t *> const &lexemes);

// ----------------------------------------------------------------------------

void skip_separator_left(int *i, std::vector<lexer::lexeme_t *> const &lexemes);

// ----------------------------------------------------------------------------

// Next Argument, true if it succed
bool skip_argument(size_t *index,
                   analyser::vector_index_lexeme_t const &lexemes,
                   int par_not_closed = 0, int bracket_not_closed = 0);

bool skip_argument(size_t *index,
                   std::vector<ns2::lexer::lexeme_t *> const &lexemes,
                   int par_not_closed = 0, int bracket_not_closed = 0);
// ----------------------------------------------------------------------------

// Skip all (...) and {...} from the right side to the left
void skip_parameters_left(int *index,
                          std::vector<lexer::lexeme_t *> const &lexemes);
// ----------------------------------------------------------------------------

// Skip all (...) and {...} from the left side to the right
void skip_parameters(size_t *index,
                     std::vector<lexer::lexeme_t *> const &lexemes);

// ----------------------------------------------------------------------------

void skip_separator_and_parameters_left(
    int *i, std::vector<lexer::lexeme_t *> const &out);

void skip_separator(size_t *i, std::vector<lexer::lexeme_t *> const &out);

void skip_separator(size_t *i, analyser::vector_index_lexeme_t const &lexemes);
// ----------------------------------------------------------------------------

} // namespace skip
} // namespace parser
} // namespace ns2

#endif
