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

#ifndef NS2_PARSER_VECTOR_HPP
#define NS2_PARSER_VECTOR_HPP

#include <ns2/lexer/lexer.hpp>
#include <ns2/parser/analyser.hpp>

namespace ns2 {
namespace parser {

template <class T> std::vector<T> slice(std::vector<T> const &v, int m, int n) {
  std::vector<T> vec(v.cbegin() + m, v.cbegin() + n + 1);
  return vec;
}

std::vector<ns2::lexer::lexeme_t *>
slice(std::vector<ns2::lexer::lexeme_t *> const &v, int m, int n);

std::vector<ns2::lexer::lexeme_t *>
slice(analyser::vector_index_lexeme_t const &v, int m, int n);

} // namespace parser
} // namespace ns2

#endif