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
#include <ns2/parser/skip.hpp>

#include <cassert>
#include <stdexcept>

using namespace ns2::lexer;

namespace ns2 {
namespace parser {
namespace skip {

// ----------------------------------------------------------------------------

lexeme_t *get(std::pair<size_t, lexeme_t *> const &p) { return p.second; }
lexeme_t *get(lexeme_t *l) { return l; }

// Skip all lexemes between two patterns
template <class vector_t>
void skip_between_(size_t *index, std::string const &first,
                   std::string const &last, vector_t const &lexemes) {
  if (*index >= lexemes.size() || get(lexemes[*index])->data != first) {
    return;
  }
  *index += 1;
  int not_closed = 1;
  if (*index >= lexemes.size()) {
    return;
  }
  lexeme_t *lex = get(lexemes[*index]);

  while (*index < lexemes.size() && not_closed != 0) {
    if (*index < lexemes.size() && get(lexemes[*index])->data == first) {
      not_closed++;
    } else if (*index < lexemes.size() && get(lexemes[*index])->data == last) {
      not_closed--;
    }
    *index += 1;
  }
  if (not_closed != 0) {
    NS2_THROW(std::invalid_argument,
              ns2::to_string(lex->lineno) + ":" + ns2::to_string(lex->col) +
                  " character '" + first +
                  "' is not closed properly -> EOF reached");
  }
}

void skip_between(size_t *index, std::string const &first,
                  std::string const &last,
                  std::vector<lexeme_t *> const &lexemes) {
  skip_between_(&index[0], first, last, lexemes);
}

void skip_between(size_t *index, std::string const &first,
                  std::string const &last,
                  analyser::vector_index_lexeme_t const &lexemes) {
  skip_between_(&index[0], first, last, lexemes);
}

// ----------------------------------------------------------------------------

void skip_separator(size_t *i, std::vector<lexeme_t> const &lexemes) {
  while (*i < lexemes.size() &&
         lexemes[*i].kind == lexeme_t::KIND_T_SEPARATOR) {
    *i += 1;
  }
}

// ----------------------------------------------------------------------------

void skip_separator_left(int *i, std::vector<lexeme_t *> const &lexemes) {
  while (*i > 0 && lexemes[size_t(*i)]->kind == lexeme_t::KIND_T_SEPARATOR) {
    *i -= 1;
  }
}

// ----------------------------------------------------------------------------

// Next Argument, true if it succed
bool skip_argument(size_t *index,
                   analyser::vector_index_lexeme_t const &lexemes,
                   int par_not_closed, int bracket_not_closed) {
  while (*index < lexemes.size() && lexemes[*index].second->data != ",") {
    if (lexemes[*index].second->data == "(") {
      par_not_closed++;
    } else if (lexemes[*index].second->data == ")") {
      par_not_closed--;
    } else if (lexemes[*index].second->data == "{") {
      bracket_not_closed++;
    } else if (lexemes[*index].second->data == "}") {
      bracket_not_closed--;
    }
    *index += 1;
    if (*index < lexemes.size() && lexemes[*index].second->data == "," &&
        (par_not_closed != 0 || bracket_not_closed != 0)) {
      *index += 1;
    }
  }
  return par_not_closed == 0 && bracket_not_closed == 0;
}
// ----------------------------------------------------------------------------

// Next Argument, true if it succed
bool skip_argument(size_t *index, std::vector<lexeme_t *> const &lexemes,
                   int par_not_closed, int bracket_not_closed) {
  while (*index < lexemes.size() && lexemes[*index]->data != ",") {
    if (lexemes[*index]->data == "(") {
      par_not_closed++;
    } else if (lexemes[*index]->data == ")") {
      par_not_closed--;
    } else if (lexemes[*index]->data == "{") {
      bracket_not_closed++;
    } else if (lexemes[*index]->data == "}") {
      bracket_not_closed--;
    }
    *index += 1;
    if (*index < lexemes.size() && lexemes[*index]->data == "," &&
        (par_not_closed != 0 || bracket_not_closed != 0)) {
      *index += 1;
    }
  }
  return par_not_closed == 0 && bracket_not_closed == 0;
}
// ----------------------------------------------------------------------------
// Skip all (...) and {...} from the right side to the left
void skip_parameters_left(int *index, std::vector<lexeme_t *> const &lexemes) {
  assert(index != NULL);

  if (*index < 0) {
    return;
  }

  int bracket_not_closed = 0;
  int par_not_closed = 0;

  if (lexemes[size_t(*index)]->data == ")") {
    par_not_closed = 1;
  } else if (lexemes[size_t(*index)]->data == "}") {
    bracket_not_closed = 1;
  } else {
    return;
  }

  *index -= 1;
  while (*index > 0 && (bracket_not_closed != 0 || par_not_closed != 0)) {
    if (lexemes[size_t(*index)]->data == "(") {
      par_not_closed--;
    } else if (lexemes[size_t(*index)]->data == ")") {
      par_not_closed++;
    }
    if (lexemes[size_t(*index)]->data == "{") {
      bracket_not_closed--;
    } else if (lexemes[size_t(*index)]->data == "}") {
      bracket_not_closed++;
    }
    *index -= 1;
  }
}
// ----------------------------------------------------------------------------

// Skip all (...) and {...} from the left side to the right
void skip_parameters(size_t *index, std::vector<lexeme_t *> const &lexemes) {
  assert(index != NULL);

  if (*index >= lexemes.size()) {
    return;
  }

  int bracket_not_closed = 0;
  int par_not_closed = 0;

  if (lexemes[*index]->data == "(") {
    par_not_closed = 1;
  } else if (lexemes[*index]->data == "{") {
    bracket_not_closed = 1;
  } else {
    return;
  }

  *index += 1;
  while (*index < lexemes.size() &&
         (bracket_not_closed != 0 || par_not_closed != 0)) {
    if (lexemes[*index]->data == "(") {
      par_not_closed++;
    } else if (lexemes[*index]->data == ")") {
      par_not_closed--;
    }
    if (lexemes[*index]->data == "{") {
      bracket_not_closed++;
    } else if (lexemes[*index]->data == "}") {
      bracket_not_closed--;
    }
    *index += 1;
  }
}

// ----------------------------------------------------------------------------

void skip_separator_and_parameters_left(int *i,
                                        std::vector<lexeme_t *> const &out) {
  do {
    skip_separator_left(&i[0], out);
    skip_parameters_left(&i[0], out);
    skip_separator_left(&i[0], out);
  } while (size_t(*i) < out.size() &&
           (out[size_t(*i)]->data == ")" || out[size_t(*i)]->data == "}"));
}

void skip_separator(size_t *i, std::vector<lexer::lexeme_t *> const &out) {
  while (*i < out.size() &&
         out[*i]->kind == lexer::lexeme_t::KIND_T_SEPARATOR) {
    *i += 1;
  }
}

// ----------------------------------------------------------------------------

void skip_separator(size_t *i, analyser::vector_index_lexeme_t const &lexemes) {
  while (*i < lexemes.size() &&
         lexemes[*i].second->kind == lexeme_t::KIND_T_SEPARATOR) {
    *i += 1;
  }
}
// ----------------------------------------------------------------------------

} // namespace skip
} // namespace parser
} // namespace ns2
