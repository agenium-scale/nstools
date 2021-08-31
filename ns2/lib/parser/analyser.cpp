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

#include <ns2/parser/analyser.hpp>
#include <ns2/parser/message.hpp>
#include <ns2/parser/pattern_matching.hpp>
#include <ns2/parser/remover.hpp>
#include <ns2/parser/skip.hpp>

#include <cassert>
#include <iostream>

using namespace ns2::ast;
using namespace ns2::lexer;
using namespace ns2::parser::match;
using namespace ns2::parser::remover;
using namespace ns2::parser::skip;

namespace ns2 {
namespace parser {
namespace analyser {

// ----------------------------------------------------------------------------

view_t::view_t() : first(0), last(0), is_function(false) {}

// ----------------------------------------------------------------------------

// Get instruction from the right side to the left side
void get_instruction_left(vector_index_lexeme_t *lexs, int *index,
                          std::vector<lexeme_t *> const &lexemes) {
  assert(lexs != NULL);
  assert(index != NULL);

  if (*index >= int(lexemes.size())) {
    return;
  }

  while (*index >= 0 && lexemes[size_t(*index)]->data != ";" &&
         lexemes[size_t(*index)]->kind != lexeme_t::KIND_T_MACRO &&
         !is_instruction_block(lexemes[size_t(*index)])) {

    // Skip only {...} if '}' is detected
    while (*index >= 0 && (lexemes[size_t(*index)]->data == ")" ||
                           lexemes[size_t(*index)]->data == "}")) {
      int last_index = *index;
      skip_parameters_left(&index[0], lexemes);
      for (int i = last_index; i > *index; --i) {
        if (lexemes[size_t(i)]->kind != lexeme_t::KIND_T_SEPARATOR) {
          lexs->insert(lexs->begin(),
                       std::make_pair(size_t(i), lexemes[size_t(i)]));
        }
      }
    }

    // End of instruction ?
    if (lexemes[size_t(*index)]->data == ";") {
      break;
    }

    // Else push lexemes
    if (lexemes[size_t(*index)]->kind != lexeme_t::KIND_T_SEPARATOR &&
        lexemes[size_t(*index)]->kind != lexeme_t::KIND_T_COMMENT) {
      lexs->insert(lexs->begin(),
                   std::make_pair(size_t(*index), lexemes[size_t(*index)]));
    }
    *index -= 1;

    // For initializer
    if (*index > 0 && lexemes[size_t(*index)]->data == "}" &&
        size_t(*index + 1) < lexemes.size() &&
        lexemes[size_t(*index + 1)]->data == ",") {
      skip_parameters_left(&index[0], lexemes);
    }
  }

  // Add instruction keyword if necessary
  if (size_t(*index) < lexemes.size() &&
      is_instruction_block(lexemes[size_t(*index)])) {
    lexs->insert(lexs->begin(),
                 std::make_pair(size_t(*index), lexemes[size_t(*index)]));
  }

  // Remove Beginning of the current block
  //...{
  //    INSTRUCTION;    =>    INSTRUCTION;
  do {
    size_t i = 0;
    while (i < lexs->size() && lexs->at(i).second->data != "{") {
      ++i;
    }
    if (i == lexs->size()) {
      break;
    }
    try {
      skip_between(&i, "{", "}", *lexs);
      lexs->erase(lexs->begin(), lexs->begin() + int(i));
    } catch (...) {
      remove_until_not_equal(&lexs[0], "{");
      remove_until_equal(&lexs[0], "{");
    }
  } while (true);

  // Remove prefix
  // public: INSTRUCTION;    =>    INSTRUCTION;
  if (lexs->size() != 0 && (lexs->at(0).second->data == "public" ||
                            lexs->at(0).second->data == "private" ||
                            lexs->at(0).second->data == "goto")) {
    remove_until_not_equal(&lexs[0], ":");
    remove_until_equal(&lexs[0], ":");
  }
}

// ----------------------------------------------------------------------------

std::vector<analyser::vector_index_lexeme_t> get_args_between(
    std::string const &left, std::string const &right,
    std::string const &separator, int const not_close_,
    analyser::vector_index_lexeme_t const &lp, size_t *i,
    std::map<std::pair<std::string, std::string>, int> other_betweens) {
  assert(i != NULL);

  int not_closed = not_close_;
  std::vector<analyser::vector_index_lexeme_t> args;
  analyser::vector_index_lexeme_t arg;

  while (*i < lp.size() && not_closed != 0) {
    if (lp[*i].second->data == left) {
      not_closed++;
    } else if (lp[*i].second->data == right) {
      not_closed--;
    } else {
      std::map<std::pair<std::string, std::string>, int>::iterator it;
      for (it = other_betweens.begin(); it != other_betweens.end(); it++) {
        if (lp[*i].second->data == it->first.first) {
          it->second++;
        } else if (lp[*i].second->data == it->first.second) {
          it->second--;
        }
      }
    }

    if (not_closed != 0) {
      arg.push_back(lp[*i]);
    }

    *i += 1;

    if (not_closed == 0) {
      if (arg.size() > 0) {
        args.push_back(arg);
      }
      arg.clear();
      break;
    }

    bool ok = true;
    {
      std::map<std::pair<std::string, std::string>, int>::iterator it;
      for (it = other_betweens.begin(); it != other_betweens.end(); it++) {
        if (it->second != 0) {
          ok = false;
          break;
        }
      }
    }

    if (*i < lp.size() && lp[*i].second->data == separator && !ok) {
      arg.push_back(lp[*i]);
      *i += 1;
    } else if (*i < lp.size() && lp[*i].second->data == separator &&
               not_closed > 1) {
      arg.push_back(lp[*i]);
      *i += 1;
    } else if ((*i < lp.size() && lp[*i].second->data == separator &&
                (not_closed != 0)) ||
               not_closed == 0) {
      args.push_back(arg);
      arg.clear();
      *i += 1;
    }
  }

  return args;
}

// ----------------------------------------------------------------------------

bool make_type_t(std::vector<analyser::vector_index_lexeme_t> const &args,
                 std::vector<type_t> *types) {
  assert(types != NULL);
  for (size_t cpt = 0; cpt < args.size(); ++cpt) {
    type_t type;
    size_t i = 0;
    if (make_type_t(args[cpt], &type, &i)) {
      types->push_back(type);
    } else {
      return false;
    }
  }
  return true;
}

bool make_type_t(analyser::vector_index_lexeme_t const &arg, type_t *type,
                 size_t *i) {
  assert(type != NULL);

  // static, const, ...
  while (is_var_declaration_keyword(arg[*i].second->data)) {
    type->qualifier_before.push_back(arg[*i].second);
    *i += 1;
    if (*i >= arg.size()) {
      return false;
    }
  }

  // Type
  if (arg[*i].second->kind != lexer::lexeme_t::KIND_T_WORD ||
      is_keyword(arg[*i].second->data)) {
    return false;
  }
  type->name = arg[*i].second;
  *i += 1;
  ;
  if (*i >= arg.size()) {
    return false;
  }

  // T<...>
  if (arg[*i].second->kind == lexer::lexeme_t::KIND_T_LESSTHAN) {
    *i += 1;
    std::map<std::pair<std::string, std::string>, int> other_betweens;
    other_betweens[std::make_pair("(", ")")] = 0;
    other_betweens[std::make_pair("{", "}")] = 0;
    other_betweens[std::make_pair("[", "]")] = 0;
    std::vector<analyser::vector_index_lexeme_t> template_args =
        analyser::get_args_between("<", ">", ",", 1, arg, &i[0],
                                   other_betweens);
    bool ok = make_type_t(template_args, &type->template_types);
    if (!ok) {
      return false;
    }
  }

  // T const ...
  while (is_var_declaration_keyword(arg[*i].second->data)) {
    type->qualifier_after.push_back(arg[*i].second);
    *i += 1;
    if (*i >= arg.size()) {
      return false;
    }
  }

  // T(\*)+ | T&
  if (arg[*i].second->kind == lexer::lexeme_t::KIND_T_AMPERSAND) {
    type->ref = arg[*i].second;
    *i += 1;
    if (*i >= arg.size()) {
      return false;
    }
  } else {
    while (arg[*i].second->kind == lexer::lexeme_t::KIND_T_ASTERISK) {
      type->asterisk.push_back(arg[*i].second);
      *i += 1;
      if (*i >= arg.size()) {
        return false;
      }
    };
  }
  return true;
}

// ----------------------------------------------------------------------------

} // namespace analyser
} // namespace parser
} // namespace ns2
