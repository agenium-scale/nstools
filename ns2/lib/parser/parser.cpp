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

#include <ns2/ast/ast_node.hpp>
#include <ns2/ast/header.hpp>
#include <ns2/exception.hpp>
#include <ns2/parser/message.hpp>
#include <ns2/parser/parser.hpp>
#include <ns2/parser/pattern_matching.hpp>
#include <ns2/parser/remover.hpp>
#include <ns2/parser/skip.hpp>
#include <ns2/parser/vector.hpp>

#include <algorithm>
#include <cassert>
#include <iostream>

using namespace ns2::ast;
using namespace ns2::lexer;
using namespace ns2::parser::analyser;
using namespace ns2::parser::match;
using namespace ns2::parser::skip;
using namespace ns2::parser::remover;

namespace ns2 {

void build_ast_tree(std::vector<ast_node_t *> *out_ast,
                    std::string const & /*filename*/,
                    std::vector<lexeme_t *> const &in) {
  assert(out_ast != NULL);
  // std::vector<lexeme_t *> in_simplified = in;
  parser::analyser::vector_index_lexeme_t in_simplified;
  for (size_t i = 0; i < in.size(); i++) {
    if (in[i]->kind != lexeme_t::KIND_T_SEPARATOR) {
      std::pair<size_t, lexeme_t *> p;
      p.first = i;
      p.second = in[i];
      in_simplified.push_back(p);
    }
  }
  // TODO
  // simplify_lexemes_instructions(&in_simplified);

  for (size_t i = 0; i < in_simplified.size(); ++i) {
    parser::skip::skip_separator(&i, in_simplified);
    // HEADER
    if (in_simplified[i].second->kind == lexeme_t::KIND_T_MACRO &&
        in_simplified[i].second->data.find("#include") == 0) {
      std::string header_file;
      header_t *header = new header_t();
      header->pos_start = int(in_simplified[i].first);
      header->pos_end = int(in_simplified[i].first) + 1;

      // position after word "#include"
      size_t pos = 8;

      // Catch include file
      while (pos < in_simplified[i].second->data.size()) {
        // #include "toto.h"
        if (in_simplified[i].second->data[pos] == '\"') {
          ++pos;
          while (pos < in_simplified[i].second->data.size() &&
                 in_simplified[i].second->data[pos] != '\"') {
            header_file += in_simplified[i].second->data[pos];
            ++pos;
          }
          if (header_file.size() == 0) {
            NS2_THROW(std::invalid_argument,
                      ns2::to_string(in_simplified[i].second->lineno) + ":" +
                          ns2::to_string(in_simplified[i].second->col) +
                          "Invalid header");
          }
          break;
        }

        // #include <toto.h>
        else if (in_simplified[i].second->data[pos] == '<') {
          ++pos;
          while (pos < in_simplified[i].second->data.size() &&
                 in_simplified[i].second->data[pos] != '>') {
            header_file += in_simplified[i].second->data[pos];
            ++pos;
          }
          break;
        }
        ++pos;
      }

      header->set_name(header_file);
      header->set_data(ns2::parser::slice(in_simplified, header->pos_start,
                                          header->pos_end));
      out_ast->push_back(header);
    }

    // DECLARATION
    function_decl_t *func = NULL;
    is_func_declaration(&i, in_simplified, in, &func);
    if (func != NULL) {
      out_ast->push_back(func);
    }

    ast_node_t *decl = NULL;
    is_var_declaration(&i, in_simplified, in, &decl);
    if (decl != NULL) {
      out_ast->push_back(decl);
    }
    // is_func_declaration(&i, in_simplified, &decl);
    // is_structure_declaration(&i, in_simplified, &decl);

    // EXPRESSION
    // is_expr(&i, );
  }
}

} // namespace ns2
