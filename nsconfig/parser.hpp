// MIT License
//
// Copyright (c) 2019 Agenium Scale
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

#ifndef PARSER_HPP
#define PARSER_HPP

#include "compiler.hpp"
#include <map>
#include <ns2/fs.hpp>
#include <string>
#include <vector>

// ----------------------------------------------------------------------------
// Forward declaration

struct rules_t;

// ----------------------------------------------------------------------------

namespace parser {

// ----------------------------------------------------------------------------

struct cursor_t {
  enum before_after_expand_t {
    BeforeVariableExpansion,
    DuringVariableExpansion,
    AfterVariableExpansion
  };

  int lineno;
  int col;
  before_after_expand_t before_after_expand;
  std::string filename;
  std::string source;

  cursor_t()
      : lineno(0), col(0), before_after_expand(BeforeVariableExpansion) {}

  cursor_t(std::string const &filename_)
      : lineno(0), col(0), before_after_expand(BeforeVariableExpansion),
        filename(filename_) {}

  cursor_t(cursor_t const &cursor, std::string const &source_)
      : lineno(cursor.lineno),
        before_after_expand(cursor.BeforeVariableExpansion),
        filename(cursor.filename), source(source_) {}
};

// ----------------------------------------------------------------------------

struct token_t {
  std::string text;
  cursor_t cursor;
};

// ----------------------------------------------------------------------------

typedef std::map<std::string, std::pair<bool, std::string> > variables_t;

// ----------------------------------------------------------------------------

struct infos_t {
  std::string source_dir;
  std::string build_dir;
  int verbosity;
  bool generate_all, generate_clean, generate_update, generate_self,
      generate_install, generate_package;
  std::vector<std::string> outputs;
  compiler::list_t compilers;
  variables_t variables;
  std::string cmdline;
  std::string build_nsconfig;
  std::string output_file;
  std::string make_command;
  std::string install_prefix;
  std::string package_name;
  bool backend_supports_self_generation;
  bool backend_supports_header_deps;
  bool generate_header_deps_flags;
  bool translate;
  parser::cursor_t translate_cursor;
  compiler::infos_t current_compiler;

  // to collect helpers from ifnot_set
  bool getting_vars_list;
  struct var_helper_t {
    std::string var_name;
    std::string helper;
    parser::cursor_t cursor;
  };
  typedef std::vector<var_helper_t> vars_list_t;
  vars_list_t vars_list;

  enum action_t {
    Permissive, // P = permissive = translate what it can
    Translate,  // T = translate and error when cannot
    Raw         // R = do not translate, copy as-is
  } action;

  bool fill_action(char c) {
    switch (c) {
    case 'R':
      action = Raw;
      return true;
    case 'T':
      action = Translate;
      return true;
    case 'P':
      action = Permissive;
      return true;
    default:
      return false;
    }
  }
};

// ----------------------------------------------------------------------------

std::ostream &operator<<(std::ostream &, const token_t &);
rules_t parse(ns2::ifile_t &, infos_t *);
void add_variable(variables_t *, std::string const &, std::string const &,
                  bool, bool);
void die(std::string const &, cursor_t const &);
void list_variables(ns2::ifile_t &, infos_t *);

// ----------------------------------------------------------------------------

} // namespace parser

#endif
