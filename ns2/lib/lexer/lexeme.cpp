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

#include <ns2/lexer/lexeme.hpp>

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include <algorithm>
#include <numeric>
#include <sstream>

#include <iostream>

namespace ns2 {
namespace lexer {

//

lexeme_t::lexeme_t() : lineno(0), col(0), kind(KIND_T_PUNCTUATION), data("") {}

lexeme_t::lexeme_t(int lineno, int col, enum kind_t kind, std::string data)
    : lineno(lineno), col(col), kind(kind), data(data) {}

// ----------------------------------------------------------------------------

const char *stringify_lexeme_kind(enum lexeme_t::kind_t k) {
  switch (k) {
  case lexeme_t::KIND_T_STRING:
    return "String";
  case lexeme_t::KIND_T_COMMENT:
    return "Comment";
  case lexeme_t::KIND_T_PUNCTUATION:
    return "Punctuation";
  case lexeme_t::KIND_T_WORD:
    return "Word";
  case lexeme_t::KIND_T_SEPARATOR:
    return "Separator";
  case lexeme_t::KIND_T_MACRO:
    return "Macro";
  case lexeme_t::KIND_T_LEFTPAREN:
    return "LeftParen";
  case lexeme_t::KIND_T_RIGHTPAREN:
    return "RightParen";
  case lexeme_t::KIND_T_LEFTSQUARE:
    return "LeftSquare";
  case lexeme_t::KIND_T_RIGHTSQUARE:
    return "RightSquare";
  case lexeme_t::KIND_T_LEFTCURLY:
    return "LeftCurly";
  case lexeme_t::KIND_T_RIGHTCURLY:
    return "RightCurly";
  case lexeme_t::KIND_T_LESSTHAN:
    return "LessThan";
  case lexeme_t::KIND_T_GREATERTHAN:
    return "GreaterThan";
  case lexeme_t::KIND_T_DOT:
    return "Dot";
  case lexeme_t::KIND_T_COLON:
    return "Colon";
  case lexeme_t::KIND_T_SEMICOLON:
    return "Semicolon";
  case lexeme_t::KIND_T_PLUS:
    return "Plus";
  case lexeme_t::KIND_T_MINUS:
    return "Minus";
  case lexeme_t::KIND_T_ASTERISK:
    return "Asterisk";
  case lexeme_t::KIND_T_DOUBLEDOT:
    return "DoubleDot";
  case lexeme_t::KIND_T_PERCENT:
    return "Percent";
  case lexeme_t::KIND_T_SLASH:
    return "Slash";
  case lexeme_t::KIND_T_AMPERSAND:
    return "Ampersand";
  case lexeme_t::KIND_T_EXCLAMATIONMARK:
    return "ExclamationMark";
  case lexeme_t::KIND_T_QUESTIONMARK:
    return "QuestionMark";
  case lexeme_t::KIND_T_EQUAL:
    return "Equal";
  case lexeme_t::KIND_T_PIPE:
    return "Pipe";
  case lexeme_t::KIND_T_CIRCUMFLEX:
    return "Circumflex";
  case lexeme_t::KIND_T_TILDE:
    return "Tilde";
  }
  return NULL; // never reached
}

// ----------------------------------------------------------------------------

bool is_letter(int a) {
  return (a >= '0' && a <= '9') || (a >= 'a' && a <= 'z') ||
         (a >= 'A' && a <= 'Z') || (a == '_' || a == '#');
}

// ----------------------------------------------------------------------------

bool is_separator(int a) {
  return a == ' ' || a == '\t' || a == '\n' || a == '\r';
}

// ----------------------------------------------------------------------------

bool is_punctuation(int a) {
  return a == '(' || a == ')' || a == '[' || a == ']' || a == '{' || a == '}' ||
         a == '<' || a == '>' || a == '.' || a == ',' || a == ';' || a == '+' ||
         a == '-' || a == '*' || a == ':' || a == '%' || a == '/' || a == '&' ||
         a == '!' || a == '?' || a == '=' || a == '|' || a == '^' || a == '~';
}

// ----------------------------------------------------------------------------
int get_char(FILE *stream) { return fgetc(stream); }

int get_char(std::stringstream *stream) { return stream->get(); }

template <typename stream_t> class window_t {
  char win_[2];
  int lineno_, col_;
  int lineno0_, col0_;
  stream_t *stream_;

public:
  window_t(stream_t *in) {
    assert(in != NULL);
    stream_ = in;
    lineno_ = 1;
    col_ = 0;
    win_[0] = char(EOF);
    win_[1] = char(EOF);
  }

  int slide() {
    win_[0] = win_[1];
    errno = 0;
    win_[1] = char(get_char(&stream_[0]));
    if (win_[0] == '\n') {
      lineno_++;
      col_ = 0;
    } else if (win_[0] == '\r') {
      col_ = 0;
    } else {
      col_++;
    }
    return errno;
  }

  void push_location() {
    lineno0_ = lineno_;
    col0_ = col_;
  }

  int pop_lineno() { return lineno0_; }

  int get_lineno() { return lineno_; }

  int pop_col() { return col0_; }

  int get_col() { return col_; }

  char buffer(int a) {
    assert(a >= 0 && a <= 1);
    return win_[a];
  }
};

// ----------------------------------------------------------------------------

template <typename stream_t>
void get_lexeme_process(std::vector<lexeme_t *> *lexemes,
                        window_t<stream_t> *win) {
  assert(win != NULL);
  assert(lexemes != NULL);
  lexemes->clear();

  std::string data;

  // Get first char
  if (win->slide()) {
    throw errno;
  }

  for (;;) {
    data.clear();

    // Get next char
    if (win->slide()) {
      throw errno;
    }

    // EOF
    if (win->buffer(0) == char(EOF)) {
      break;
    }

    // Next lexeme is a Macro
    if (win->buffer(0) == '#') {
      win->push_location();

      int pos;
      do {
        while (win->buffer(1) != '\n' && win->buffer(1) != char(EOF)) {
          data.push_back(win->buffer(0));
          if (win->slide()) {
            throw errno;
          }
        }
        data.push_back(win->buffer(0));
        if (win->slide()) {
          throw errno;
        }
        pos = int(data.size()) - 1;
        for (; pos >= 0; --pos) {
          if (!is_separator(data[size_t(pos)])) {
            break;
          }
        }
      } while (data[size_t(pos)] == '\\');
      data += "\n";
      lexemes->push_back(new lexeme_t(win->pop_lineno(), win->pop_col(),
                                      lexeme_t::KIND_T_MACRO, data));
      continue;
    }

    // Next lexeme is a // style comment
    if (win->buffer(0) == '/' && win->buffer(1) == '/') {
      win->push_location();
      while (win->buffer(1) != '\n') {
        data.push_back(win->buffer(0));
        if (win->slide()) {
          throw errno;
        }
      }
      data.push_back(win->buffer(0));
      lexemes->push_back(new lexeme_t(win->pop_lineno(), win->pop_col(),
                                      lexeme_t::KIND_T_COMMENT, data));
      continue;
    }

    // Next lexeme is a /* style comment
    if (win->buffer(0) == '/' && win->buffer(1) == '*') {
      win->push_location();
      while (win->buffer(0) != '*' || win->buffer(1) != '/') {
        data.push_back(win->buffer(0));
        if (win->slide()) {
          throw errno;
        }
      }
      data.push_back('*');
      data.push_back('/');
      if (win->slide()) {
        throw errno;
      }
      lexemes->push_back(new lexeme_t(win->pop_lineno(), win->pop_col(),
                                      lexeme_t::KIND_T_COMMENT, data));
      continue;
    }

    // Next lexeme is a string/char
    if (win->buffer(0) == '"' || win->buffer(0) == '\'') {
      char end = win->buffer(0);
      win->push_location();
      data.push_back(win->buffer(0));
      if (win->slide()) {
        throw errno;
      }
      while (win->buffer(0) != end) {
        if (win->buffer(0) == '\\') {
          data.push_back('\\');
          if (win->slide()) {
            throw errno;
          }
        }
        data.push_back(win->buffer(0));
        if (win->slide()) {
          throw errno;
        }
      }
      data.push_back(end);
      lexemes->push_back(new lexeme_t(win->pop_lineno(), win->pop_col(),
                                      lexeme_t::KIND_T_STRING, data));
      continue;
    }

    // Next lexeme is a punctuation
    if (is_punctuation(win->buffer(0))) {
      data.push_back(win->buffer(0));
      lexeme_t::kind_t lex;

      switch (win->buffer(0)) {
      case '(':
        lex = lexeme_t::KIND_T_LEFTPAREN;
        break;
      case ')':
        lex = lexeme_t::KIND_T_RIGHTPAREN;
        break;
      case '[':
        lex = lexeme_t::KIND_T_LEFTSQUARE;
        break;
      case ']':
        lex = lexeme_t::KIND_T_RIGHTSQUARE;
        break;
      case '{':
        lex = lexeme_t::KIND_T_LEFTCURLY;
        break;
      case '}':
        lex = lexeme_t::KIND_T_RIGHTCURLY;
        break;
      case '<':
        lex = lexeme_t::KIND_T_LESSTHAN;
        break;
      case '>':
        lex = lexeme_t::KIND_T_GREATERTHAN;
        break;
      case '.':
        lex = lexeme_t::KIND_T_DOT;
        break;
      case ',':
        lex = lexeme_t::KIND_T_COLON;
        break;
      case ';':
        lex = lexeme_t::KIND_T_SEMICOLON;
        break;
      case '+':
        lex = lexeme_t::KIND_T_PLUS;
        break;
      case '-':
        lex = lexeme_t::KIND_T_MINUS;
        break;
      case '*':
        lex = lexeme_t::KIND_T_ASTERISK;
        break;
      case ':':
        lex = lexeme_t::KIND_T_DOUBLEDOT;
        break;
      case '%':
        lex = lexeme_t::KIND_T_PERCENT;
        break;
      case '/':
        lex = lexeme_t::KIND_T_SLASH;
        break;
      case '&':
        lex = lexeme_t::KIND_T_AMPERSAND;
        break;
      case '!':
        lex = lexeme_t::KIND_T_EXCLAMATIONMARK;
        break;
      case '?':
        lex = lexeme_t::KIND_T_QUESTIONMARK;
        break;
      case '=':
        lex = lexeme_t::KIND_T_EQUAL;
        break;
      case '|':
        lex = lexeme_t::KIND_T_PIPE;
        break;
      case '^':
        lex = lexeme_t::KIND_T_CIRCUMFLEX;
        break;
      case '~':
        lex = lexeme_t::KIND_T_TILDE;
        break;
      default:
        lex = lexeme_t::KIND_T_PUNCTUATION;
      }

      lexemes->push_back(
          new lexeme_t(win->get_lineno(), win->get_col(), lex, data));
    }

    // Next lexeme is a word
    if (is_letter(win->buffer(0))) {
      win->push_location();
      while (is_letter(win->buffer(1))) {
        data.push_back(win->buffer(0));
        if (win->slide()) {
          throw errno;
        }
      }
      data.push_back(win->buffer(0));
      lexemes->push_back(new lexeme_t(win->pop_lineno(), win->pop_col(),
                                      lexeme_t::KIND_T_WORD, data));
    }

    // Next lexeme is a separator
    if (is_separator(win->buffer(0))) {
      win->push_location();
      while (is_separator(win->buffer(1))) {
        data.push_back(win->buffer(0));
        if (win->slide()) {
          throw errno;
        }
      }
      data.push_back(win->buffer(0));
      lexemes->push_back(new lexeme_t(win->pop_lineno(), win->pop_col(),
                                      lexeme_t::KIND_T_SEPARATOR, data));
    }
  }
}

} // namespace lexer

void get_lexemes(std::vector<lexer::lexeme_t *> *lexemes, FILE *in) {
  // Wrap in
  lexer::window_t<FILE> win(in);
  lexer::get_lexeme_process(&lexemes[0], &win);
}

void get_lexemes(std::vector<lexer::lexeme_t *> *lexemes,
                 std::string const &s) {
  // Wrap in
  std::stringstream ss(s);
  lexer::window_t<std::stringstream> win(&ss);
  lexer::get_lexeme_process(&lexemes[0], &win);
}

std::string pprint(std::vector<lexer::lexeme_t *> const &lexemes) {
  size_t size = lexemes.size();
  std::string output = "";
  for (size_t i = 0; i < size; i++) {
    output += lexemes[i]->data;
  }
  return output;
}

} // namespace ns2
