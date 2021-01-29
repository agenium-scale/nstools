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

#include "parser.hpp"
#include "compiler.hpp"
#include "find_exe_lib_header.hpp"
#include "nsconfig.hpp"
#include "shell.hpp"
#include <cstdlib>
#include <map>
#include <ns2/fs.hpp>
#include <ns2/process.hpp>
#include <ns2/string.hpp>
#include <ns2/levenshtein.hpp>
#include <set>
#include <utility>

namespace parser {

// ----------------------------------------------------------------------------

std::ostream &operator<<(std::ostream &os, const token_t &token) {
  os << token.text;
  return os;
}

// ----------------------------------------------------------------------------

static std::string
get_first_suggestions(std::vector<std::pair<std::string, int> > const &vec) {
  std::string ret;
  if (vec.size() > 0) {
    for (size_t i = 0; i < vec.size() && vec[i].second == vec[0].second; i++) {
      if (i > 0) {
        ret += " or ";
      }
      ret += '"' +  vec[i].first + '"';
    }
  }
  return ret;
}

// ----------------------------------------------------------------------------

static const char *statements[] = {
  "disable_all",
  "disable_update",
  "disable_clean",
  "disable_install",
  "disable_package",
  "set",
  "ifnot_set",
  "glob",
  "popen",
  "ifnot_glob",
  "getenv",
  "build_file",
  "phony",
  "build_files",
  "find_exe",
  "find_lib",
  "find_header",
  "echo",
  "include",
  "install_dir",
  "install_file",
  "package_name"
};

static const char *constants[] = {
  "@source_dir",
  "@build_dir",
  "@obj_ext",
  "@asm_ext",
  "@static_lib_ext",
  "@shared_lib_ext",
  "@shared_link_ext",
  "@exe_ext",
  "@in",
  "@item",
  "@out",
  "@make_command",
  "@prefix",
  "@ccomp_suite",
  "@ccomp_path",
  "@cppcomp_suite",
  "@cppcomp_path"
};

// ----------------------------------------------------------------------------

static bool sregex(std::string const &str, std::string const &regex) {
  if (str.size() != regex.size()) {
    return false;
  }

  for (size_t i = 0; i < str.size(); i++) {
    if (regex[i] != '?' && regex[i] != str[i]) {
      return false;
    }
  }

  return true;
}

// ----------------------------------------------------------------------------

void add_variable(variables_t *variables, std::string const &key,
                  std::string const &value, bool used, bool force) {
  if (force) {
    (*variables)[key] = std::pair<bool, std::string>(used, value);
  } else {
    variables_t::const_iterator it = (*variables).find(key);
    if (it == (*variables).end()) {
      (*variables)
          .insert(std::pair<std::string, std::pair<bool, std::string> >(
              key, std::pair<bool, std::string>(used, value)));
    }
  }
}

// ----------------------------------------------------------------------------

static void tokenize(std::vector<token_t> &tokens, std::string const &str,
                     cursor_t const &cursor) {
  std::string buf;
  cursor_t cur(cursor);
  cur.col = -1;
  for (size_t i = 0; i < str.size(); i++) {
    if (str[i] == '#') {
      break;
    } else if (ns2::is_blank(str[i])) {
      if (cur.col >= 0) {
        token_t token;
        token.text = buf;
        token.cursor = cur;
        tokens.push_back(token);
        buf.clear();
        cur.col = -1;
      }
      continue;
    } else if (str[i] == '"') {
      cur.col = int(i);
      for (i++; i < str.size() && str[i] != '"'; i++) {
        buf += str[i];
      }
      if (i == str.size()) {
        die("cannot find ending double-quote", cur);
      }
    } else {
      if (cur.col == -1) {
        cur.col = int(i);
      }
      if (str[i] == '\\' && (i + 1) < str.size() && str[i + 1] == '"') {
        buf += "\\\"";
        i++;
      } else {
        buf += str[i];
      }
    }
  }
  if (cur.col >= 0) {
    token_t token;
    token.text = buf;
    token.cursor = cur;
    tokens.push_back(token);
  }
}

// ----------------------------------------------------------------------------

static std::string substitute(std::string const &str, cursor_t const &cursor,
                              infos_t *pi_) {
  infos_t &pi = *pi_;
  variables_t &variables = pi.variables;
  std::string ret;
  cursor_t cur(cursor, str);

  // variables substitution
  for (size_t i = 0; i < str.size(); i++) {
    if (str[i] == '$') {
      if (i + 1 >= str.size()) {
        cur.col = int(i + 1);
        die("unexpected end of line", cur);
      }

      // case of '$$', we return '$'
      if (str[i + 1] == '$') {
        ret += '$';
        i++;
        continue;
      }

      size_t i0, i1;
      std::string key;

      // case of '${ ... }', we evaluate what's inside to get the variable
      // name and return its value
      if (str[i + 1] == '{') {
        i0 = i + 2;
        int curly = 1;
        for (i1 = i0; i1 < str.size(); i1++) {
          if (str[i1] == '$' && i1 + 1 < str.size() && str[i1 + 1] == '{') {
            curly++;
          }
          if (str[i1] == '}') {
            curly--;
            if (curly == 0) {
              break;
            }
          }
        }
        if (curly > 0) {
          cur.col = int(i + 1);
          die("cannot find closing '}'", cur);
        }
        key = substitute(
            std::string(str.begin() + long(i0), str.begin() + long(i1)), cur,
            &pi);

        // case of '$...$', we take what's inside as the variable name and
        // return its value
      } else {
        i0 = i + 1;
        for (i1 = i0;
             i1 < str.size() && str[i1] != '$' && !ns2::is_blank(str[i1]);
             i1++)
          ;
        key = std::string(str.begin() + long(i0), str.begin() + long(i1));
      }

      variables_t::iterator it = variables.find(key);
      if (it == variables.end()) {
        if (pi.getting_vars_list) {
          ret += key;
        } else {
          cur.col = int(i);
          std::string suggestions(
              get_first_suggestions(ns2::levenshtein_sort(variables, key)));
          if (suggestions.size() > 0) {
            die("don't know how to expand this: \"" + key +
                    "\", did you mean " + suggestions + "?",
                cur);
          } else {
            die("don't know how to expand this: \"" + key + "\"", cur);
          }
        }
      } else {
        ret += it->second.second;
        it->second.first = true;
      }
      if (i1 < str.size() && ns2::is_blank(str[i1])) {
        ret += str[i1];
      }
      i = i1;
    } else {
      ret += str[i];
    }
  }

  return ret;
}

// ----------------------------------------------------------------------------

static void add_target(rule_desc_t const &rule_desc, rules_t *rules,
                       infos_t *pi_) {
  infos_t &pi = *pi_;

  // First determine if the rule is a phony or not, if the rule is not
  // marked as phony but does not contain any command then it is in fact
  // a phony.
  rule_desc_t::type_t rule_type = rule_desc.type;
  if (rule_desc.type != rule_desc_t::Phony && rule_desc.cmds.size() == 0) {
    rule_type = rule_desc_t::Phony;
  }

  switch (rule_desc.type) {

  // We treat here simple rules, phony and build_file (build single file)
  case rule_desc_t::SingleFile:
  case rule_desc_t::SelfGenerate:
  case rule_desc_t::Phony: {

    // Original rule
    rule_desc_t rd0(rule_desc);
    rule_desc_t rd(rule_desc);
    rd.autodeps_by = pi.current_compiler.type;

    // Replace @in and @out
    std::string output(shell::stringify(shell::ify(rd0.output)));
    std::string input(ns2::join(shell::ify(rd0.deps), " "));
    for (size_t i = 0; i < rd.cmds.size(); i++) {
      rd0.cmds[i] = ns2::replace(rd0.cmds[i], "@out", output);
      rd0.cmds[i] = ns2::replace(rd0.cmds[i], "@in", input);
    }

    // If the rule has autodeps we may need to replace @@autodeps_flags for
    // header dependencies
    if (rule_desc.autodeps &&
        pi.current_compiler.type != compiler::infos_t::None) {
      std::string flags = shell::autodeps_flags(pi.current_compiler.type,
                                                rule_desc.autodeps_file);
      for (size_t i = 0; i < rd.cmds.size(); i++) {
        rd.cmds[i] = ns2::replace(rd0.cmds[i], "@@autodeps_flags", flags);
      }
    } else {
      rd.cmds = rd0.cmds;
    }

    // We add the dependency for self-regeneration for all types of rule
    // if the user demands it
    if (pi.generate_self) {
      rd.deps.push_back(pi.output_file);
    }

    // First add the rule as-is
    rd.fill_for_as_is();

    // Check if the rule already exist.
    rule_desc_t const *erd = rules->find_by_target(rd.target);
    if (erd != NULL) {
      die("rule name '" + rd.target + "' already defined at line " +
              ns2::to_string(erd->cursor.lineno),
          rule_desc.cursor);
    }

    // Add the rule
    rules->insert(rd.target, rd);
    if (pi.verbosity >= VERBOSITY_NORMAL) {
      OUTPUT << "Add new target: '" << rd.target << "'" << std::endl;
    }

    // Add the file to the list of outputs
    if (rule_type != rule_desc_t::Phony) {
      pi.outputs.push_back(ns2::sanitize(rule_desc.output));
    }

    // Then add the corresponding "force" rule
    if (rule_desc.type != rule_desc_t::Phony &&
        rule_desc.type != rule_desc_t::SelfGenerate) {

      rd.fill_for_force();
      rd.autodeps = false;
      for (size_t i = 0; i < rd.cmds.size(); i++) {
        rd.cmds[i] = ns2::replace(rd0.cmds[i], "@@autodeps_flags", "");
      }

      // Check if the rule already exists
      rule_desc_t const *erd = rules->find_by_target(rd.target);
      if (erd != NULL) {
        die("rule name already defined at line " +
                ns2::to_string(erd->cursor.lineno),
            rule_desc.cursor);
      }

      // Add the rule
      rules->insert(rd.target, rd);
      if (pi.verbosity >= VERBOSITY_NORMAL) {
        OUTPUT << "Add new target: '" << rd.target << "'" << std::endl;
      }
    }

    break;
  }

  // We treat here build_files (build a bunch of files)
  case rule_desc_t::MultipleFiles:

    for (size_t i = 0; i < rule_desc.out_ins.size(); i++) {
      rule_desc_t rd;

      // Build rule of type SingleFile
      rd.output = rule_desc.out_ins[i].first;
      rd.type = rule_desc_t::SingleFile;
      rd.autodeps_file = rd.output + ".d";
      rd.autodeps_by = pi.current_compiler.type;
      rd.autodeps = rule_desc.autodeps;
      rd.cursor = rule_desc.cursor;

      // Compute dependencies (if @item is one of them do string replacement)
      std::string input(shell::ify(rule_desc.out_ins[i].second));
      for (size_t j = 0; j < rule_desc.deps.size(); j++) {
        if (rule_desc.deps[j] == "@item") {
          rd.deps.push_back(input);
        } else {
          rd.deps.push_back(rule_desc.deps[j]);
        }
      }

      // Replace @item
      rd.type = rule_desc_t::SingleFile;
      for (size_t j = 0; j < rule_desc.cmds.size(); j++) {
        rd.cmds.push_back(ns2::replace(rule_desc.cmds[j], "@item", input));
      }
      rd.cursor = rule_desc.cursor;

      // temporarily deactivate verbosity for recursive call
      int temp = pi.verbosity;
      pi.verbosity = VERBOSITY_QUIET;
      add_target(rd, rules, &pi);
      pi.verbosity = temp;
    }

    if (pi.verbosity >= VERBOSITY_NORMAL) {
      OUTPUT << "Add " << rule_desc.out_ins.size()
             << " new targets from globbing: '" << rule_desc.output << "'"
             << std::endl;
    }
    break;
  }
}

// ----------------------------------------------------------------------------

inline std::string get_ext(compiler::infos_t::type_t type,
                           std::string const &which) {
  switch (type) {
  case compiler::infos_t::MSVC:
#ifdef NS2_IS_MSVC
  case compiler::infos_t::NVCC:
#endif
    if (which == "@asm_ext") {
      return ".asm";
    } else if (which == "@obj_ext") {
      return ".obj";
    } else if (which == "@static_lib_ext") {
      return ".lib";
    } else if (which == "@shared_lib_ext") {
      return ".dll";
    } else if (which == "@shared_link_ext") {
      return ".lib";
    } else {
      return ".exe";
    }
    break;
#ifndef NS2_IS_MSVC
  case compiler::infos_t::NVCC:
#endif
  case compiler::infos_t::GCC:
  case compiler::infos_t::Clang:
  case compiler::infos_t::ARMClang:
  case compiler::infos_t::FCC_trad_mode:
  case compiler::infos_t::FCC_clang_mode:
  case compiler::infos_t::HIPCC:
  case compiler::infos_t::HCC:
  case compiler::infos_t::DPCpp:
  case compiler::infos_t::ICC:
    if (which == "@asm_ext") {
      return ".s";
    } else if (which == "@obj_ext") {
      return ".o";
    } else if (which == "@static_lib_ext") {
      return ".a";
    } else if (which == "@shared_lib_ext") {
#if defined(NS2_IS_MSVC)
      return ".dll";
#elif defined(NS2_IS_MACOS)
      return ".dylib";
#else
      return ".so";
#endif
    } else if (which == "@shared_link_ext") {
#ifdef NS2_IS_MSVC
      return ".a";
#else
      return ".so";
#endif
    } else {
#if defined(NS2_IS_MSVC)
      return ".exe";
#else
      return "";
#endif
    }
    break;
  case compiler::infos_t::Emscripten:
    if (which == "@asm_ext") {
      return ".s";
    } else if (which == "@exe_ext") {
      return ".js";
    } else {
      return ".o";
    }
  case compiler::infos_t::None:
    NS2_THROW(std::runtime_error, "Invalid compiler");
    break;
  }

  // should never be reached
  return std::string();
}

// ----------------------------------------------------------------------------

static std::string build_files_format(std::string const &fmt,
                                      std::string const &input) {
  ns2::dir_file_t df = ns2::split_path(input);
  std::string const &basename = df.second;
  std::pair<std::string, std::string> buf = ns2::splitext(basename);
  std::string const &rootname = buf.first;
  std::string const &extension = buf.second;

  std::string ret;
  ret = ns2::replace(fmt, "%b", basename);
  ret = ns2::replace(ret, "%e", extension);
  ret = ns2::replace(ret, "%r", rootname);
  return ret;
}

// ----------------------------------------------------------------------------

static void parse_rec(rules_t *rules_, ns2::ifile_t &in, infos_t *pi_) {
  rules_t &rules = *rules_;
  std::string line;
  cursor_t cursor(in.filename());
  rule_desc_t rule_desc;
  bool first_rule_desc = true;
  infos_t &pi = *pi_;

  while (!in.eof()) {
    std::vector<token_t> tokens;

    // read line and tokenize it
    cursor.lineno++;
    std::string raw_line;
    std::getline(in, raw_line);
    std::string trim_raw_line = ns2::strip(raw_line);
    if (trim_raw_line.size() == 0 || trim_raw_line[0] == '#') {
      continue;
    }

    // perform variable substitution if we are translating
    cursor.before_after_expand = cursor_t::DuringVariableExpansion;
    if (pi.translate) {
      line = substitute(trim_raw_line, cursor, &pi);
    } else {
      line  = trim_raw_line;
    }
    cursor.before_after_expand = cursor_t::AfterVariableExpansion;
    cursor.source = line;

    // tokenize
    tokenize(tokens, line, cursor);

    // lines endings with '\' were one and the same split into several
    while (!in.eof() && tokens.size() > 0 && tokens.back().text == "\\") {
      // remove trailing '\'
      tokens.pop_back();

      // read next line and tokenize it
      cursor.lineno++;
      std::getline(in, line);

      // perform variable substitution if we are translating
      cursor.before_after_expand = cursor_t::DuringVariableExpansion;
      if (pi.translate) {
        line = substitute(ns2::strip(line), cursor, &pi);
      } else {
        line = ns2::strip(line);
      }
      cursor.before_after_expand = cursor_t::AfterVariableExpansion;
      cursor.source = line;
      if (line.size() == 0) {
        break;
      }
      if (line[0] == '#') {
        continue;
      }
      tokenize(tokens, line, cursor);
    }

    // analyze line prefix [...] and ignore (or not) line according to the OS
    // we run on
    std::string const &s = tokens[0].text;
    bool modifier_is_present = sregex(s, "[?]") || sregex(s, "[?:?]");
    int os = modifier_is_present ? s[1] : '*';
    if (os != 'L' && os != 'W' && os != '*') {
      tokens[0].cursor.col++;
      // L = linux
      // W = windows
      // * = both systems
      die("expected 'L', 'W' or '*'", tokens[0].cursor);
    }
    if (pi.fill_action(sregex(s, "[?:?]") ? s[3] : 'T') == false) {
      tokens[0].cursor.col += 3;
      // P = permissive = translate what it can
      // T = translate and error when cannot
      // R = do not translate, copy as-is
      die("expected 'P', 'T' or 'R'", tokens[0].cursor);
    }
#ifdef NS2_IS_MSVC
    if (os == 'L') {
      continue;
    }
#else
    if (os == 'W') {
      continue;
    }
#endif

    // if the first character is a tab we have a command for a target
    bool cmd = (raw_line[0] == '\t');

    // remove the first token if it is one of the "[?]"'s or "[?:?]"'s
    if (modifier_is_present) {
      tokens.erase(tokens.begin());
    }

    // just a shortcut
    std::string const &head = tokens[0].text;

    // set and ifnot_set
    // We begin by this command because it allows us to skip the rest
    // of the commands when the user just wants to list variables that can
    // be set outside of the build.nsconfig
    if (!cmd && (head == "set" || head == "ifnot_set")) {
      if (tokens.size() == 1 && head == "ifnot_set") {
        die("expected variable description after", tokens[0].cursor);
      }
      size_t i0 = (head == "set" ? 1 : 2);
      if (tokens.size() == i0) {
        die("expected variable name after", tokens[i0 - 1].cursor);
      }
      i0++;
      if (tokens.size() == i0) {
        die("expected '=' after", tokens[i0 - 1].cursor);
      }
      if (tokens[i0].text != "=") {
        die("expected '=' here", tokens[i0].cursor);
      }
      std::string const &key = tokens[i0 - 1].text;
      std::string value(
          ns2::join(tokens.begin() + long(i0 + 1), tokens.end(), " "));

      // handle special requests only when not getting variables
      if (value[0] == '@') {
        if (value == "@source_dir") {
#ifdef NS2_IS_MSVC
          value = ns2::replace(pi.source_dir, '\\', '/');
#else
          value = pi.source_dir;
#endif
        } else if (value == "@build_dir") {
#ifdef NS2_IS_MSVC
          value = ns2::replace(pi.build_dir, '\\', '/');
#else
          value = pi.build_dir;
#endif
        } else if (value == "@make_command") {
          value = pi.make_command;
        } else if (value == "@prefix") {
          value = pi.install_prefix;
        } else if (value == "@obj_ext" || value == "@static_lib_ext" ||
                   value == "@shared_lib_ext" || value == "@shared_link_ext" ||
                   value == "@exe_ext" || value == "@asm_ext") {
          compiler::infos_t ci = compiler::get("cc", &pi);
          value = get_ext(ci.type, value);
        } else if (value == "@ccomp_suite") {
          compiler::infos_t ci = compiler::get("cc", &pi);
          value = compiler::get_type_and_lang_str(ci.type, ci.lang);
        } else if (value == "@ccomp_path") {
          compiler::infos_t ci = compiler::get("cc", &pi);
          value = ci.path;
        } else if (value == "@cppcomp_suite") {
          compiler::infos_t ci = compiler::get("c++", &pi);
          value = compiler::get_type_and_lang_str(ci.type, ci.lang);
        } else if (value == "@cppcomp_path") {
          compiler::infos_t ci = compiler::get("c++", &pi);
          value = ci.path;
        } else {
          std::string suggestions(get_first_suggestions(ns2::levenshtein_sort(
              constants, sizeof(constants) / sizeof(char *), value)));
          if (suggestions.size() > 0) {
            die("unknown constant \"" + value + "\", did you mean " +
                    suggestions + "?",
                tokens[i0 + 1].cursor);
          } else {
            die("unknown statement \"" + value + "\"", tokens[i0 + 1].cursor);
          }
        }
      }
      add_variable(&pi.variables, key, value, true, head == "set");
      if (head == "ifnot_set") {
        infos_t::var_helper_t buf;
        buf.var_name = tokens[2].text;
        buf.helper = tokens[1].text;
        buf.cursor = cursor;
        pi.vars_list.push_back(buf);
      }
      continue;
    }

    // When we are only looking for ifnot_set variables we do not continue
    // parsing the other commands. This prevents the user from seeing errors
    // in the build.config file and I don't know whether it is a good thing
    // or not.
    if (pi.getting_vars_list) {
      continue;
    }

    // end_translate: must be here to avoid the parsing of any other command
    if (!cmd && head == "end_translate") {
      if (!pi.begin_translate) {
        die("unexpected 'end_translate', no corresponding "
            "'begin_translate_if'",
            tokens[0].cursor);
      }
      pi.translate = true;
      pi.begin_translate = false;
      continue;
    }

    // if we are not in a translation zone then continue
    if (pi.translate == false) {
      continue;
    }

    // begin_translate_if
    if (!cmd && head == "begin_translate_if") {
      if (tokens.size() == 1) {
        die("expecting expression after", tokens[0].cursor);
      }
      if (tokens.size() == 2) {
        die("expecting comparison operator after", tokens[1].cursor);
      }
      if (tokens[2].text != "==" && tokens[2].text != "!=") {
        die("comparison operator must be '==' or '!='", tokens[2].cursor);
      }
      if (tokens.size() == 3) {
        die("expecting expression after", tokens[2].cursor);
      }
      if (tokens.size() > 4) {
        die("unexpected token", tokens[4].cursor);
      }
      if ((tokens[2].text == "==" && tokens[1].text == tokens[3].text) ||
          (tokens[2].text == "!=" && tokens[1].text != tokens[3].text)) {
        pi.translate = true;
      } else {
        pi.translate = false;
      }
      pi.translate_cursor = tokens[0].cursor;
      pi.begin_translate = true;
      continue;
    }

    // echo
    if (!cmd && head == "echo") {
      std::cout << "--";
      for (size_t i = 1; i < tokens.size(); i++) {
        std::cout << " " << tokens[i].text;
      }
      std::cout << std::endl;
      continue;
    }

    // package_name
    if (!cmd && head == "package_name") {
      if (tokens.size() == 1) {
        die("expecting package name after", tokens[0].cursor);
      }
      if (tokens.size() > 2) {
        die("extra token here", tokens[2].cursor);
      }
      pi.package_name = tokens[1].text;
      continue;
    }

    // install_file, install_dir
    if (!cmd && (head == "install_file" || head == "install_dir")) {
      if (tokens.size() == 1) {
        die("expected file to install after", tokens[0].cursor);
      } else if (tokens.size() == 2) {
        die("expected path for installation", tokens[1].cursor);
      }
      std::string const &path = tokens.back().text;
      if (head == "install_file") {
        for (size_t i = 1; i < tokens.size() - 1; i++) {
          rules.file_install_paths.push_back(
              ns2::dir_file_t(path, tokens[i].text));
        }
      } else {
        for (size_t i = 1; i < tokens.size() - 1; i++) {
          rules.dir_install_paths.push_back(
              ns2::dir_file_t(path, tokens[i].text));
        }
      }
      continue;
    }

    // disable_all, disable_clean, disable_update
    if (!cmd && (head == "disable_all" || head == "disable_clean" ||
                 head == "disable_update" || head == "disable_install" ||
                 head == "disable_package")) {
      if (tokens.size() > 1) {
        die("unexpected token here", tokens[1].cursor);
      }
      if (head == "disable_all") {
        pi.generate_all = false;
      } else if (head == "disable_clean") {
        pi.generate_clean = false;
      } else if (head == "disable_update") {
        pi.generate_update = false;
      } else if (head == "disable_install") {
        pi.generate_install = false;
      } else if (head == "disable_package") {
        pi.generate_package = false;
      }
      continue;
    }

    // getenv
    if (!cmd && head == "getenv") {
      if (tokens.size() == 1) {
        die("expected variable name after", tokens[0].cursor);
      } else if (tokens.size() == 2) {
        die("expected '='", tokens[1].cursor);
      } else if (tokens[2].text != "=") {
        die("expected '='", tokens[2].cursor);
      } else if (tokens.size() == 3) {
        die("expected environment variable name", tokens[2].cursor);
      } else if (tokens.size() > 4) {
        die("this is unexpected", tokens[4].cursor);
      }

      const char *value = getenv(tokens[3].text.c_str());
#ifdef NS2_IS_MSVC
      add_variable(&pi.variables, tokens[1].text,
                   value == NULL ? std::string()
                                 : ns2::replace(value, '\\', '/'),
                   true, true);
#else
      add_variable(&pi.variables, tokens[1].text,
                   value == NULL ? std::string() : std::string(value), true,
                   true);
#endif
      continue;
    }

    // glob
    if (!cmd && head == "glob") {
      if (tokens.size() == 1) {
        die("expected variable name after", tokens[0].cursor);
      } else if (tokens.size() == 2) {
        die("expected '='", tokens[1].cursor);
      } else if (tokens[2].text != "=") {
        die("expected '='", tokens[2].cursor);
      } else if (tokens.size() == 2) {
        die("expected globbing expression after", tokens[2].cursor);
      }
      std::string const &key = tokens[1].text;
      std::string value;
      for (size_t i = 3; i < tokens.size(); i++) {
        std::vector<std::string> files = ns2::glob(tokens[i].text);
        for (size_t j = 0; j < files.size(); j++) {
          if (value.size() > 0) {
            value += " ";
          }
          value += shell::stringify(files[j]);
        }
      }
      add_variable(&pi.variables, key, value, true, true);
      continue;
    }

    // find_exe, find_header
    if (!cmd && (head == "find_exe" || head == "find_header")) {
      bool optional;
      if (tokens.size() == 1) {
        die("expected variable name or 'optional' keyword after",
            tokens[0].cursor);
      }
      optional = tokens[1].text == "optional";
      size_t i = optional ? 2 : 1;

      if (tokens.size() == i) {
        die("expected variable name after", tokens[i - 1].cursor);
      }
      i++;

      if (tokens.size() == i) {
        die("expected '=' after", tokens[i - 1].cursor);
      }
      i++;

      if (tokens[i - 1].text != "=") {
        die("expected '=' here", tokens[i - 1].cursor);
      }
      if (tokens.size() == i) {
        if (head == "find_exe") {
          die("expected executable name after", tokens[i - 1].cursor);
        } else if (head == "find_header") {
          die("expected header path after", tokens[i - 1].cursor);
        }
      }
      i++;

      std::string &var = tokens[optional ? 2 : 1].text;
      std::string &file = tokens[optional ? 4 : 3].text;
      std::vector<std::string> paths;
      for (; i < tokens.size(); i++) {
        paths.push_back(tokens[i].text);
      }

      if (head == "find_exe") {
        find_exe(&pi.variables, var, file, paths, pi.verbosity, !optional);
      } else if (head == "find_header") {
        find_header(&pi.variables, var, file, paths, pi.verbosity, !optional);
      }
      continue;
    }

    // find_lib
    if (!cmd && head == "find_lib") {
      bool optional = false;
      bool import = false;
      libtype_t libtype = Automatic;
      if (tokens.size() == 1) {
        die("expected variable name, 'optional', 'dynamic' or 'static' after",
            tokens[0].cursor);
      }

      size_t i = 1;
      for (; i < 3 && i < tokens.size(); i++) {
        if (tokens[i].text == "optional") {
          optional = true;
        } else if (tokens[i].text == "dynamic") {
          libtype = Dynamic;
        } else if (tokens[i].text == "static") {
          libtype = Static;
        } else if (tokens[i].text == "import") {
          import = true;
        } else {
          break;
        }
      }

      if (tokens.size() == i) {
        die("expected variable name after", tokens[i - 1].cursor);
      }
      i++;

      if (tokens.size() == i) {
        die("expected '=' after", tokens[i - 1].cursor);
      }
      i++;

      if (tokens[i - 1].text != "=") {
        die("expected '=' here", tokens[i - 1].cursor);
      }
      if (tokens.size() == i) {
        die("expected header path after", tokens[i - 1].cursor);
      }
      i++;

      if (tokens.size() == i) {
        die("expected binary file after", tokens[i - 1].cursor);
      }
      i++;

      std::string &var = tokens[i - 4].text;
      std::string &header = tokens[i - 2].text;
      std::string &binary = tokens[i - 1].text;
      std::vector<std::string> paths;
      for (; i < tokens.size(); i++) {
        paths.push_back(tokens[i].text);
      }

      rule_desc_t import_rd;
      find_lib(&pi.variables, &import_rd, var, header, binary, paths,
               pi.verbosity, libtype, !optional, import);
      // if we must import the shared library, then find_lib will has filled in
      // import_rd with output, dependencies and commands, the rest we have to
      // fill it here
      if (import) {
        import_rd.cursor = tokens[0].cursor;
        import_rd.autodeps = false;
        import_rd.type = rule_desc_t::SingleFile;
        add_target(import_rd, &rules, &pi);
      }
      continue;
    }

    // include
    if (!cmd && head == "include") {
      if (tokens.size() == 1) {
        die("no file given to include", tokens[0].cursor);
      }
      for (size_t i = 1; i < tokens.size(); i++) {
        std::string filename = tokens[i].text;
        if (!ns2::exists(filename)) {
          die("file does not seem to exists", tokens[i].cursor);
        }
        ns2::ifile_t in(tokens[i].text);
        parse_rec(&rules, in, &pi);
      }
      continue;
    }

    // build_files
    if (!cmd && head == "build_files") {
      // add previous rule_desc and target to the list of rules
      if (!first_rule_desc) {
        add_target(rule_desc, &rules, &pi);
      }
      first_rule_desc = false;
      rule_desc.clear();
      rule_desc.type = rule_desc_t::MultipleFiles;

      // sanity check
      if (tokens.size() == 1) {
        die("expected variable name or 'optional' after", tokens[0].cursor);
      }

      bool optional = tokens[1].text == "optional";
      size_t i = optional ? 2 : 1; // i points to the variable prefix
      if (tokens.size() == i) {
        die("no variable name given after", tokens[i - 1].cursor);
      }
      std::string const &var_prefix = tokens[i].text;
      rule_desc.output = tokens[i].text;
      rule_desc.cursor = tokens[i].cursor;

      i++; // i points to foreach
      if (tokens.size() == i) {
        die("expected 'foreach' keyword after", tokens[i - 1].cursor);
      }
      if (tokens[i].text != "foreach") {
        die("expected 'foreach' keyword here", tokens[i].cursor);
      }
      // get index of beginning and end for inputs
      size_t i_begin_input = i + 1;
      size_t i_end_input = i + 1;
      for (; i_end_input < tokens.size() && tokens[i_end_input].text != "as";
           i_end_input++)
        ;
      if (!optional) {
        if (i_begin_input == i_end_input) {
          if (tokens[i].text == "as") {
            die("keyword 'as' unexpected, expected some files/globs here",
                tokens[i].cursor);
          } else {
            die("expected some files/globs here", tokens[i].cursor);
          }
        }
      }

      i = i_end_input; // i points to the keyword as
      if (tokens.size() <= i) {
        die("expected 'as' keyword after", tokens[tokens.size() - 1].cursor);
      }

      i++; // i points to the output format
      if (tokens.size() == i) {
        die("expected filename format after", tokens[i - 1].cursor);
      }
      std::string const &format = tokens[i].text;

      // compute target names and fill rule_desc
      for (size_t j = i_begin_input; j < i_end_input; j++) {
        if (ns2::startswith(tokens[j].text, "glob:")) {
          // glob all requested files
          std::string buf(ns2::sanitize(tokens[j].text.c_str() + 5));
          ns2::dir_file_t df = ns2::split_path(buf);
          size_t i0 = df.first.size() + (df.first.size() > 0 ? 1 : 0);
          std::vector<std::string> temp = ns2::glob(df.first, df.second);
          for (size_t k = 0; k < temp.size(); k++) {
            std::string out = build_files_format(
                format,
                ns2::replace(std::string(temp[k].begin() + long(i0),
                             temp[k].end()), '/', '.'));
            rule_desc.out_ins.push_back(
                std::pair<std::string, std::string>(out, temp[k]));
          }
        } else if (ns2::startswith(tokens[j].text, "popen:")) {
          // add the entries read from a command
          std::string cmd(tokens[j].text.c_str() + 6);
          std::pair<std::string, int> ret = ns2::popen(cmd);
          if (ret.second != 0) {
            die("process failed with code " + ns2::to_string(ret.second),
                tokens[j].cursor);
          }
          std::vector<std::string> lines = ns2::split(ret.first, '\n');
          for (size_t k = 0; k < lines.size(); k++) {
            std::string line = ns2::strip(lines[k]);
            if (line.size() == 0 || line[0] == '#') {
              continue;
            }
            if (line.find(';') != std::string::npos) {
              std::vector<std::string> parts = ns2::split(line, ';');
              if (parts.size() != 2) {
                die("output of command misformed", tokens[k].cursor);
              }
              std::string out =
                  build_files_format(format, ns2::replace(parts[1], '/', '.'));
              std::string in = parts[0] + "/" + parts[1];
              rule_desc.out_ins.push_back(
                  std::pair<std::string, std::string>(out, in));
            } else {
              ns2::dir_file_t df = ns2::split_path(line);
              rule_desc.out_ins.push_back(std::pair<std::string, std::string>(
                  build_files_format(format, df.second), line));
            }
          }
        } else {
          // add the entry
          std::string const &buf = tokens[j].text;
          ns2::dir_file_t df = ns2::split_path(buf);
          rule_desc.out_ins.push_back(std::pair<std::string, std::string>(
              build_files_format(format, df.second), buf));
        }
      }

      // sanity check
      if (rule_desc.out_ins.size() == 0 && !optional) {
        die("cannot find any file for the given globbing(s)",
            tokens[i_begin_input].cursor);
      }

      // if the user specified 'autodeps' then we will generate appropriate
      // flags
      i++; // i points to deps or autodeps
      if (i < tokens.size()) {
        if (tokens[i].text != "deps" && tokens[i].text != "autodeps") {
          die("expected 'deps' or 'autodeps' keyword here", tokens[i].cursor);
        }

        if (tokens[i].text == "autodeps") {
          pi.generate_header_deps_flags = pi.backend_supports_header_deps;
          rule_desc.autodeps = pi.backend_supports_header_deps;
          pi.current_compiler.type = compiler::infos_t::None;
        } else {
          pi.generate_header_deps_flags = false;
          rule_desc.autodeps = false;
        }

        // add dependencies for each input
        for (i++; i < tokens.size(); i++) {
          rule_desc.deps.push_back(tokens[i].text);
        }
      }

      // add variable with all outputs
      std::string buf;
      if (rule_desc.out_ins.size() > 0) {
        buf = rule_desc.out_ins[0].first;
        for (size_t i = 1; i < rule_desc.out_ins.size(); i++) {
          buf += " " + rule_desc.out_ins[i].first;
        }
      }
      add_variable(&pi.variables, var_prefix + ".files", buf, true, true);

      continue;
    }

    // build_file, phony
    if (!cmd && (head == "build_file" || head == "phony")) {

      // add previous rule_desc and target to the list of rules
      if (!first_rule_desc) {
        add_target(rule_desc, &rules, &pi);
      }
      first_rule_desc = false;
      rule_desc.clear();
      if (head == "build_file") {
        rule_desc.type = rule_desc_t::SingleFile;
      } else {
        rule_desc.type = rule_desc_t::Phony;
      }

      // sanity check
      if (tokens.size() == 1) {
        if (head == "build_file") {
          die("no name given to the file to build", tokens[0].cursor);
        } else {
          die("no name given to the phony target", tokens[0].cursor);
        }
      }

      // compute new target name
      rule_desc.output = tokens[1].text;
      if (rule_desc.output.size() == 0) {
        die("cannot have empty rule name", tokens[1].cursor);
      }

      // set rule cursor
      rule_desc.cursor = tokens[1].cursor;

      // either there are no dependencies or there is and we expect
      // the 'deps' or 'autodeps' keywords
      if (tokens.size() >= 3 && tokens[2].text != "deps" &&
          tokens[2].text != "autodeps") {
        die("expected 'deps' keyword here", tokens[2].cursor);
      }

      // if the user specified 'autodeps' then we will generate appropriate
      // flags
      if (tokens.size() >= 3 && tokens[2].text == "autodeps") {
        pi.generate_header_deps_flags = pi.backend_supports_header_deps;
        rule_desc.autodeps_file = rule_desc.output + ".d";
        rule_desc.autodeps = pi.backend_supports_header_deps;
        pi.current_compiler.type = compiler::infos_t::None;
      } else {
        pi.generate_header_deps_flags = false;
        rule_desc.autodeps = false;
      }

      // compute all dependencies
      for (size_t i = 3; i < tokens.size(); i++) {
        rule_desc.deps.push_back(tokens[i].text);
      }

      continue;
    }

    // we have a command
    if (cmd) {
      if (tokens.size() == 0) {
        die("no command given after", tokens[0].cursor);
      }
      rule_desc.cmds.push_back(shell::translate(tokens, &pi));
      continue;
    }

    // unknown statement
    std::string suggestions(get_first_suggestions(ns2::levenshtein_sort(
        statements, sizeof(statements) / sizeof(char *), tokens[0].text)));
    if (suggestions.size() > 0) {
      die("unknown statement \"" + tokens[0].text + "\", did you mean " +
              suggestions + "?",
          tokens[0].cursor);
    } else {
      die("unknown statement \"" + tokens[0].text + "\"", tokens[0].cursor);
    }
  }

  // add last rule_desc and target to the list of rules
  if (!first_rule_desc) {
    add_target(rule_desc, &rules, &pi);
  }
}

// ----------------------------------------------------------------------------

static void resolve_force_rule_deps(rules_t *rules, infos_t const &pi) {
  for (rules_t::iterator it = rules->begin(); it != rules->end(); it++) {
    // We consider only "force" rules
    if (it->second.force) {
      std::vector<std::string> deps = it->second.deps;
      it->second.deps.clear();
      for (size_t i = 0; i < deps.size(); i++) {
        if (deps[i] != pi.output_file) {
          rule_desc_t *rd = rules->find_force_by_output(deps[i]);
          if (rd != NULL) {
            it->second.deps.push_back(rd->target);
          }
        } else {
          it->second.deps.push_back(pi.output_file);
        }
      }
    }
  }
}

// ----------------------------------------------------------------------------

static void add_phony_target(infos_t *pi_, rules_t *rules_,
                             std::string const &name,
                             std::vector<std::string> const &cmds,
                             std::vector<std::string> const &deps,
                             bool is_update = false) {
  infos_t &pi = *pi_;
  rules_t &rules = *rules_;
  rule_desc_t rd;
  rd.output = name;
  rd.force = false;
  rd.autodeps = false;
  rd.type = rule_desc_t::Phony;
  rd.deps = deps;
  rd.cmds = cmds;
  bool temp = pi.generate_self;
  if (is_update) {
    pi.generate_self = false; // avoid the target to depend on itself
  }
  add_target(rd, &rules, &pi);
  if (is_update) {
    pi.generate_self = temp; // restore the value
  }
}

// ----------------------------------------------------------------------------

void push_back_cmds_deps_for_install(std::vector<std::string> *cmds,
                                     std::vector<std::string> *deps,
                                     rules_t const &rules,
                                     std::string const &prefix) {
  std::set<std::string> dirs;
  for (size_t i = 0; i < rules.file_install_paths.size(); i++) {
    std::string dest_dir = ns2::sanitize(
        ns2::join_path(prefix, rules.file_install_paths[i].first));
    if (dirs.find(dest_dir) == dirs.end()) {
      cmds->push_back(shell::mkdir_p(dest_dir));
      dirs.insert(dest_dir);
    }
    cmds->push_back(
        shell::cp(false, ns2::sanitize(rules.file_install_paths[i].second),
                  dest_dir));
    deps->push_back(rules.file_install_paths[i].second);
  }
  for (size_t i = 0; i < rules.dir_install_paths.size(); i++) {
    std::string dest_dir = ns2::sanitize(
        ns2::join_path(prefix, rules.dir_install_paths[i].first));
    if (dirs.find(dest_dir) == dirs.end()) {
      cmds->push_back(shell::mkdir_p(dest_dir));
      dirs.insert(dest_dir);
    }
    cmds->push_back(
        shell::cp(true, ns2::sanitize(rules.dir_install_paths[i].second),
                  dest_dir));
  }
}

// ----------------------------------------------------------------------------

rules_t parse(ns2::ifile_t &in, infos_t *pi_) {
  rules_t ret;
  infos_t &pi = *pi_;

  // Do the real parsing
  pi.getting_vars_list = false;
  parse_rec(&ret, in, &pi);

  // Check for non closed begin_translate_if
  if (pi.translate == false) {
    NS2_THROW(std::runtime_error,
              "unfinished begin_translate_if on line " +
                  ns2::to_string(pi.translate_cursor.lineno));
  }

  // Check for unused variables
  std::vector<std::string> used_vars;
  std::vector<std::string> unused_vars;
  for (variables_t::const_iterator it = pi.variables.begin();
       it != pi.variables.end(); it++) {
    if (it->second.first) {
      used_vars.push_back(it->first);
    } else {
      unused_vars.push_back(it->first);
    }
  }
  if (unused_vars.size() > 0) {
    std::ostringstream os;
    for (size_t i = 0; i < unused_vars.size(); i++) {
      os << "variable \"" << unused_vars[i] << "\" defined but not used";
      std::string suggestions(get_first_suggestions(
          ns2::levenshtein_sort(used_vars, unused_vars[i])));
      if (suggestions.size() > 0) {
        os << ", did you mean " << suggestions << "?";
      }
      os << std::endl;
    }
    NS2_THROW(std::runtime_error, os.str());
  }

  if (ret.size() == 0) {
    OUTPUT << "No rule given, nothing to do" << std::endl;
    exit(EXIT_SUCCESS);
  }

  // Add target 'clean' if need be
  if (pi.generate_clean && ret.find_by_target("clean") == NULL) {
    std::vector<std::string> cmds;
    for (size_t i = 0; i < pi.outputs.size(); i++) {
      cmds.push_back(shell::rm(false, pi.outputs[i]));
    }
    add_phony_target(&pi, &ret, "clean", cmds, std::vector<std::string>());
  }

  // Add target 'all' if need be
  if (pi.generate_all && ret.find_by_target("all") == NULL) {
    std::vector<std::string> deps;
#ifdef NS2_IS_MSVC
    for (size_t i = 0; i < pi.outputs.size(); i++) {
      deps.push_back(ns2::sanitize(pi.outputs[i]));
    }
#else
    deps = pi.outputs;
#endif
    add_phony_target(&pi, &ret, "all", std::vector<std::string>(), deps);
  }

  // Add target 'install' if need be
  if (pi.generate_install && ret.find_by_target("install") == NULL) {
    std::vector<std::string> deps;
    std::vector<std::string> cmds;
    push_back_cmds_deps_for_install(&cmds, &deps, ret, pi.install_prefix);
    add_phony_target(&pi, &ret, "install", cmds, deps);
  }

  // Add target 'package' if need be
  if (pi.generate_package && ret.find_by_target("package") == NULL) {
    std::vector<std::string> deps;
    std::vector<std::string> cmds;
    cmds.push_back(shell::rm(true, pi.package_name));
#ifdef NS2_IS_MSVC
    cmds.push_back(shell::rm(false, pi.package_name + ".zip"));
#else
    cmds.push_back(shell::rm(false, pi.package_name + ".tar.bz2"));
#endif
    push_back_cmds_deps_for_install(&cmds, &deps, ret, pi.package_name);
    cmds.push_back(shell::zip(pi.package_name));
    add_phony_target(&pi, &ret, "package", cmds, deps);
  }

  // Add target 'update' if need be
  if (pi.generate_update && ret.find_by_target("update") == NULL) {
    std::vector<std::string> cmds(1, pi.cmdline);
    add_phony_target(&pi, &ret, "update", cmds, std::vector<std::string>(),
                     true);
  }

  // Add target for Makefile or build.ninja
  if (pi.generate_self && ret.find_by_target(pi.output_file) == NULL) {
    rule_desc_t rd;
    rd.output = pi.output_file;
    rd.type = rule_desc_t::SelfGenerate;
    rd.force = false;
    rd.autodeps = false;
    rd.deps.push_back(pi.build_nsconfig);
    rd.cmds.push_back(pi.cmdline);
    if (!pi.backend_supports_self_generation) {
      rd.cmds.push_back("@echo x");
      rd.cmds.push_back("@echo x x x");
      rd.cmds.push_back("@echo x x x x x");
      rd.cmds.push_back("@echo x x x x x x . . . RERUN " + pi.make_command);
      rd.cmds.push_back("@echo x x x x x");
      rd.cmds.push_back("@echo x x x");
      rd.cmds.push_back("@echo x");
      rd.cmds.push_back("@exit 99");
    }
    pi.generate_self = false; // avoid the target to depend on itself
    add_target(rd, &ret, &pi);
    pi.generate_self = true; // restore the value
  }

  // Resolve all dependencies (change the outputs by their corresponding
  // targets)
  resolve_force_rule_deps(&ret, pi);

  return ret;
}

// ----------------------------------------------------------------------------

void die(std::string const &msg, cursor_t const &cursor) {
  int i0 = maximum(0, cursor.col - 30);
  int i1 = minimum(int(cursor.source.size()), cursor.col + 30);
  std::string source(cursor.source.begin() + i0, cursor.source.begin() + i1);
  int col = cursor.col - i0;
  std::ostringstream os;

  if (source.size() > 0) {
    std::string when;
    switch (cursor.before_after_expand) {
    case cursor_t::BeforeVariableExpansion:
      when = "before variable expansion";
      break;
    case cursor_t::DuringVariableExpansion:
      when = "during variable expansion";
      break;
    case cursor_t::AfterVariableExpansion:
      when = "after variable expansion";
      break;
    }
    os << cursor.filename << ":" << cursor.lineno << ": " << when << std::endl
       << std::endl;
    if (i0 > 0) {
      os << "... ";
      col += 4;
    }
    os << source;
    if (i1 < int(cursor.source.size())) {
      os << "... ";
    }
    os << std::endl
       << std::string(size_t(col), ' ') << "^~~~~ " << msg << std::endl
       << std::endl;
  } else {
    os << cursor.filename << ":" << cursor.lineno << ": " << msg << std::endl;
  }
  NS2_THROW(std::runtime_error, os.str());
}

// ----------------------------------------------------------------------------

void list_variables(ns2::ifile_t &in, infos_t *pi_) {
  infos_t &pi = *pi_;
  pi.getting_vars_list = true;
  rules_t rules;
  parse_rec(&rules, in, &pi);
}

// ----------------------------------------------------------------------------

} // namespace parser
