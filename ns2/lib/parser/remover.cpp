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

#include <ns2/exception.hpp>
#include <ns2/parser/message.hpp>
#include <ns2/parser/remover.hpp>

#include <cassert>
#include <stdexcept>

#include <iostream>

namespace ns2 {
namespace parser {
namespace remover {

// ----------------------------------------------------------------------------

void remove_separators(std::vector<lexer::lexeme_t> *out,
                       analyser::view_t const &arg,
                       std::vector<lexer::lexeme_t> const &lexemes) {
  assert(out != NULL);
  out->clear();

  if (arg.first < 0 || arg.last < arg.first ||
      size_t(arg.last) >= lexemes.size()) {
    NS2_THROW(std::logic_error,
              ns2::to_string(__FILE__) + ":" + ns2::to_string(__LINE__));
  }

  out->reserve(size_t(arg.last - arg.first) + 1);

  for (size_t i = size_t(arg.first); i < size_t(arg.last + 1); ++i) {
    if (lexemes[i].kind != lexer::lexeme_t::KIND_T_SEPARATOR) {
      out->push_back(lexemes[i]);
    }
  }
}

// ----------------------------------------------------------------------------

void remove_until_equal(analyser::vector_index_lexeme_t *vector,
                        std::string str) {
  assert(vector != NULL);

  while (vector->size() > 0 && vector->at(0).second->data == str) {
    vector->erase(vector->begin());
  }
}

void remove_until_not_equal(analyser::vector_index_lexeme_t *vector,
                            std::string str) {
  assert(vector != NULL);

  while (vector->size() > 0 && vector->at(0).second->data != str) {
    vector->erase(vector->begin());
  }
}

// ----------------------------------------------------------------------------

void remove_trailing_space(std::string *str) {
  assert(str != NULL);
  std::string out;

  int i = int(str->size()) - 1;

  while (i >= 0 && str->at(size_t(i)) == ' ') {
    --i;
  }
  for (size_t j = 0; j <= size_t(i); ++j) {
    out += str->at(j);
  }

  *str = out;
}

} // namespace remover
} // namespace parser
} // namespace ns2
