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

#ifndef NS2_LEXER_HPP
#define NS2_LEXER_HPP

#include <string>
#include <vector>

namespace ns2 {
namespace lexer {

struct lexeme_t {

  enum kind_t {
    KIND_T_SEPARATOR,
    KIND_T_COMMENT,
    KIND_T_STRING,
    KIND_T_WORD,
    KIND_T_MACRO,

    // Punctuation
    KIND_T_LEFTPAREN,       // (
    KIND_T_RIGHTPAREN,      // )
    KIND_T_LEFTSQUARE,      // [
    KIND_T_RIGHTSQUARE,     // ]
    KIND_T_LEFTCURLY,       // {
    KIND_T_RIGHTCURLY,      // }
    KIND_T_LESSTHAN,        // <
    KIND_T_GREATERTHAN,     // >
    KIND_T_DOT,             // .
    KIND_T_COLON,           // ,
    KIND_T_SEMICOLON,       // ;
    KIND_T_PLUS,            // +
    KIND_T_MINUS,           // -
    KIND_T_ASTERISK,        // *
    KIND_T_DOUBLEDOT,       // :
    KIND_T_PERCENT,         // %
    KIND_T_SLASH,           // /
    KIND_T_AMPERSAND,       // &
    KIND_T_EXCLAMATIONMARK, // !
    KIND_T_QUESTIONMARK,    // ?
    KIND_T_EQUAL,           // =
    KIND_T_PIPE,            // |
    KIND_T_CIRCUMFLEX,      // ^
    KIND_T_TILDE,           // ~
    KIND_T_PUNCTUATION
  };

  lexeme_t();
  lexeme_t(int lineno, int col, enum kind_t kind, std::string data);

  int lineno, col;
  kind_t kind;
  std::string data;
};

bool is_separator(int a);

const char *stringify_lexeme_kind(lexeme_t::kind_t);

} // namespace lexer

void get_lexemes(std::vector<lexer::lexeme_t *> *, FILE *);
void get_lexemes(std::vector<lexer::lexeme_t *> *, std::string const &);

std::string pprint(std::vector<lexer::lexeme_t *> const &);

} // namespace ns2

#endif
