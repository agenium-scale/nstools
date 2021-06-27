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

#ifndef NSCONFIG_HPP
#define NSCONFIG_HPP

#include "parser.hpp"
#include <string>
#include <vector>

// ----------------------------------------------------------------------------

#define VERBOSITY_QUIET 0
#define VERBOSITY_NORMAL 1
#define VERBOSITY_DEBUG 2

#define OUTPUT std::cout << "-- "
#define WARNING std::cout << "WW "

inline void output_multiline(std::string const &msg) {
  if (msg.size() == 0) {
    OUTPUT << "|" << std::endl;
    return;
  }
  size_t i = 0;
  for (; i < msg.size(); i++) {
    OUTPUT << "| ";
    for (; i < msg.size() && msg[i] != '\n'; i++) {
      std::cout << msg[i];
    }
    std::cout << '\n';
  }
  std::cout << std::flush;
}

#define NINJA_BUILD_SCRIPT "_ninja_build_scripts"

// ----------------------------------------------------------------------------
// For some reason on MSVC min and max are empty macros

template <typename T> inline T maximum(T a, T b) { return a > b ? a : b; }

template <typename T> inline T minimum(T a, T b) { return a < b ? a : b; }

// ----------------------------------------------------------------------------

enum rule_desc_step { WithTokens, WithShellTranslation };

template <rule_desc_step step> struct cmds_t {};

template <> struct cmds_t<WithShellTranslation> {
  std::vector<std::string> data;
  parser::infos_t::action_t get_action(size_t) { return parser::infos_t::Raw; }
};

template <> struct cmds_t<WithTokens> {
  std::vector<std::vector<parser::token_t> > data;
  std::vector<parser::infos_t::action_t> actions;
  parser::infos_t::action_t get_action(size_t i) { return actions[i]; }
};

enum rule_desc_type_t {
  RuleSingleFile,
  RuleMultipleFiles,
  RulePhony,
  RuleSelfGenerate
};

template <rule_desc_step step> struct rule_desc_t {
  rule_desc_type_t type;
  std::string target;
  std::string output;
  std::vector<std::string> deps;
  cmds_t<step> cmds;
  std::vector<std::pair<std::string, std::string> > out_ins;
  parser::cursor_t cursor;
  bool autodeps; // rule deps are automatically computed? (gcc -M)
  compiler::infos_t::type_t
      autodeps_by;           // compiler for header deps computation
  std::string autodeps_file; // file written by the compiler containing deps
  bool force; // rule does not care about deps? (target prefixed by "f_")

  template <rule_desc_step step_other>
  void copy_all_but_cmds(rule_desc_t<step_other> const &other) {
    type = other.type;
    target = other.target;
    output = other.output;
    deps = other.deps;
    out_ins = other.out_ins;
    cursor = other.cursor;
    autodeps = other.autodeps;
    autodeps_by = other.autodeps_by;
    autodeps_file = other.autodeps_file;
    force = other.force;
  }

  void fill_for_as_is() {
    force = false;
    target = output;
  }

  void fill_for_force() {
    if (type == RulePhony || type == RuleSelfGenerate) {
      NS2_THROW(std::runtime_error,
                "cannot turn into force rule a Phony or of SelfGenrate rule");
    }
    force = true;
    target = "f_" + ns2::replace(output, '/', '.');
    autodeps = false;
  }

  void clear() {
    output.clear();
    deps.clear();
    cmds.data.clear();
    out_ins.clear();
  }
};

// ----------------------------------------------------------------------------

struct rules_t {
  // map : target --> rule_desc_t
  typedef std::map<std::string, rule_desc_t<WithShellTranslation> >
      target_to_rule_t;
  target_to_rule_t target_to_rule;

  // map : output --> rule_desc_t
  typedef std::map<std::string, rule_desc_t<WithShellTranslation> *>
      output_to_rule_t;
  output_to_rule_t force_output_to_rule;

  // installation paths for targets: install, package
  std::vector<ns2::dir_file_t> file_install_paths;
  std::vector<ns2::dir_file_t> dir_install_paths;

  rule_desc_t<WithShellTranslation> *
  find_by_target(std::string const &target) {
    target_to_rule_t::iterator it = target_to_rule.find(target);
    return (it == target_to_rule.end() ? NULL : &(it->second));
  }

  rule_desc_t<WithShellTranslation> const *
  find_by_target(std::string const &target) const {
    target_to_rule_t::const_iterator it = target_to_rule.find(target);
    return (it == target_to_rule.end() ? NULL : &(it->second));
  }

  rule_desc_t<WithShellTranslation> *
  find_force_by_output(std::string const &output) {
    output_to_rule_t::iterator it = force_output_to_rule.find(output);
    return (it == force_output_to_rule.end() ? NULL : it->second);
  }

  rule_desc_t<WithShellTranslation> const *
  find_force_by_output(std::string const &output) const {
    output_to_rule_t::const_iterator it = force_output_to_rule.find(output);
    return (it == force_output_to_rule.end() ? NULL : it->second);
  }

  void insert(std::string const &target,
              rule_desc_t<WithShellTranslation> const &rd) {
    if (rd.force) {
      force_output_to_rule[rd.output] = &(target_to_rule[target] = rd);
    } else {
      target_to_rule[target] = rd;
    }
  }

  size_t size() { return target_to_rule.size(); }

  typedef target_to_rule_t::const_iterator const_iterator;

  const_iterator begin() const { return target_to_rule.begin(); }

  const_iterator end() const { return target_to_rule.end(); }

  typedef target_to_rule_t::iterator iterator;

  iterator begin() { return target_to_rule.begin(); }

  iterator end() { return target_to_rule.end(); }
};

// ----------------------------------------------------------------------------

#endif
