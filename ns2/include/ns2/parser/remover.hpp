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

#ifndef NS2_PARSER_REMOVER_HPP
#define NS2_PARSER_REMOVER_HPP

#include <ns2/parser/analyser.hpp>

namespace ns2 {
namespace parser {
namespace remover {

// ----------------------------------------------------------------------------

void remove_separators(std::vector<lexer::lexeme_t> *out,
                       analyser::view_t const &arg,
                       std::vector<lexer::lexeme_t> const &lexemes);

// ----------------------------------------------------------------------------

void remove_until_equal(analyser::vector_index_lexeme_t *vector,
                        std::string str);

void remove_until_not_equal(analyser::vector_index_lexeme_t *vector,
                            std::string str);

// ----------------------------------------------------------------------------

void remove_trailing_space(std::string *str);

} // namespace remover
} // namespace parser
} // namespace ns2
#endif
