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

#include "find_exe_lib_header.hpp"
#include "shell.hpp"
#include <cstdlib>
#include <ns2/fs.hpp>
#include <string>
#include <vector>

// ----------------------------------------------------------------------------

static std::string sanitize_name_for_define(std::string const &str) {
  std::string ret;
  for (size_t i = 0; i < str.size(); i++) {
    if (str[i] >= 'a' && str[i] <= 'z') {
      ret += (char)(str[i] + ('A' - 'a'));
    } else if ((str[i] >= 'A' && str[i] <= 'Z') || str[i] == '_') {
      ret += str[i];
    } else if (str[i] >= '0' && str[i] <= '9') {
      ret += str[i];
    } else {
      ret += '_';
    }
  }
  return ret;
}

// ----------------------------------------------------------------------------

std::string lib_basename(std::string const &str) {
#ifdef NS2_IS_MSVC
  std::string temp = ns2::lower(str);
#else
  std::string const &temp = str;
#endif
  size_t i0 = ns2::startswith(temp, "lib") ? 3 : 0;
  size_t i1 = str.size();
  if (ns2::endswith(temp, ".so")) {
    i1 -= 3;
  } else if (ns2::endswith(temp, ".a")) {
    i1 -= 2;
  } else if (ns2::endswith(temp, ".lib")) {
    i1 -= 4;
  } else if (ns2::endswith(temp, ".dll")) {
    i1 -= 4;
  }
  return std::string(str.begin() + i0, str.begin() + i1);
}

// ----------------------------------------------------------------------------

void find_exe(parser::variables_t *variables, std::string const &var,
              std::string const &prog_name,
              std::vector<std::string> const &paths, int verbosity,
              bool die_on_not_found) {
  const char *env[] = {"PATH"};
#ifdef NS2_IS_MSVC
  std::vector<std::string> exe(1, prog_name + ".exe");
#else
  std::vector<std::string> exe(1, prog_name);
#endif
  ns2::dir_file_t df = ns2::find(paths, exe, env, 1, true);
  if (ns2::found(df)) {
    if (verbosity >= VERBOSITY_NORMAL) {
      OUTPUT << "Program '" << prog_name << "' found" << std::endl;
    } else if (verbosity >= VERBOSITY_DEBUG) {
      OUTPUT << "Program '" << prog_name << "' found in '" << df.first << "'"
             << std::endl;
    }
  } else {
    if (verbosity >= VERBOSITY_NORMAL) {
      OUTPUT << "Program '" << prog_name << "' not found" << std::endl;
    }
    if (die_on_not_found) {
      exit(EXIT_FAILURE);
    }
  }
  parser::add_variable(variables, var + ".dir", df.first, true, true);
  parser::add_variable(variables, var, ns2::join_path(df.first, df.second),
                       true, true);
}

// ----------------------------------------------------------------------------

void find_header(parser::variables_t *variables, std::string const &var,
                 std::string const &header_path,
                 std::vector<std::string> const &paths, int verbosity,
                 bool die_on_not_found) {
  std::vector<std::string> header(1, header_path);
  ns2::dir_file_t df = ns2::find(paths, header);
  if (ns2::found(df)) {
    if (verbosity >= VERBOSITY_NORMAL) {
      OUTPUT << "Header '" << header_path << "' found" << std::endl;
    } else if (verbosity >= VERBOSITY_DEBUG) {
      OUTPUT << "Header '" << header_path << "' found in '" << df.first << "'"
             << std::endl;
    }
    parser::add_variable(variables, var + ".dir", df.first, true, true);
    std::string flags("-DHAS_" + sanitize_name_for_define(header_path) +
                      " -I" + df.first);
    parser::add_variable(variables, var + ".flags", flags, true, true);
    parser::add_variable(variables, var + ".cflags", flags, true, true);
  } else {
    if (verbosity >= VERBOSITY_NORMAL) {
      OUTPUT << "Header '" << header_path << "' not found" << std::endl;
    }
    if (die_on_not_found) {
      exit(EXIT_FAILURE);
    }
    parser::add_variable(variables, var + ".dir", "", true, true);
    parser::add_variable(variables, var + ".flags", "", true, true);
    parser::add_variable(variables, var + ".cflags", "", true, true);
  }
}

// ----------------------------------------------------------------------------

void find_lib(parser::variables_t *variables, rule_desc_t *rd,
              std::string const &var, std::string const &header_path,
              std::string const &binary_path,
              std::vector<std::string> const &paths, int verbosity,
              libtype_t libtype, bool die_on_not_found, bool import_lib) {
  // look for the header first
  std::vector<std::string> header(1, header_path);
  std::vector<std::string> hpaths(paths);
  for (size_t i = 0; i < paths.size(); i++) {
    hpaths.push_back(paths[i] + "/include");
  }
  ns2::dir_file_t hdf = ns2::find(hpaths, header);

  // look for the binary file
  std::vector<std::string> binaries;
  ns2::dir_file_t df = ns2::split_path(binary_path);
  std::string lib_name(lib_basename(df.second));
#ifdef NS2_IS_MSVC
  (void)libtype;
  binaries.push_back(lib_name + ".lib");
  binaries.push_back("lib" + lib_name + ".lib");
  binaries.push_back(lib_name + ".a");
  binaries.push_back("lib" + lib_name + ".a");
#else
  switch (libtype) {
  case Dynamic:
    binaries.push_back("lib" + lib_name + ".so");
    break;
  case Static:
    binaries.push_back("lib" + lib_name + ".a");
    break;
  case Automatic:
    binaries.push_back("lib" + lib_name + ".so");
    binaries.push_back("lib" + lib_name + ".a");
    break;
  }
#endif
  std::vector<std::string> lpaths(paths);
  for (size_t i = 0; i < paths.size(); i++) {
    lpaths.push_back(paths[i] + "/lib");
    lpaths.push_back(paths[i] + "/lib64");
  }
  ns2::dir_file_t ldf = ns2::find(lpaths, binaries);

  if (ns2::found(hdf) && ns2::found(ldf)) {
    if (verbosity >= VERBOSITY_NORMAL) {
      OUTPUT << "Library '" << lib_name << "' found" << std::endl;
    } else if (verbosity >= VERBOSITY_DEBUG) {
      OUTPUT << "Header '" << lib_name << "' found, header in '" << hdf.first
             << "' and binary in '" << ldf.first << "'" << std::endl;
    }
    parser::add_variable(variables, var + ".header_dir", hdf.first, true,
                         true);
    std::string lib_filename(ns2::join_path(ldf.first, ldf.second));
    std::string cflags, ldflags;
    if (import_lib) {
      if (verbosity >= VERBOSITY_NORMAL) {
        OUTPUT << "Importing library: " << lib_filename << std::endl;
      }
      ns2::copyfile(lib_filename, ldf.second);
      std::pair<std::string, std::string> lbe = ns2::splitext(lib_filename);
      std::pair<std::string, std::string> llbe = ns2::splitext(ldf.second);
      if (ns2::lower(lbe.second) == "dll") {
        ns2::copyfile(lbe.first + ".lib", llbe.first + ".lib");
      }
      parser::add_variable(variables, var + ".lib_dir", ".", true, true);
      parser::add_variable(variables, var + ".deps", ldf.second, true, true);
      cflags =
          "-DHAS_" + sanitize_name_for_define(lib_name) + " -I" + hdf.first;
      ldflags = "-L$ORIGIN" + (ns2::startswith(ns2::lower(ldf.second), "lib")
                                   ? " -l" + lib_name
                                   : " -l:" + ldf.second);
      rd->target = ldf.second;
      rd->output = ldf.second;
      rd->deps.push_back(lib_filename);
      rd->cmds.push_back(shell::cp(false, lib_filename, ldf.second));
      if (ns2::lower(lbe.second) == "dll") {
        rd->cmds.push_back(
            shell::cp(false, lbe.first + ".lib", llbe.first + ".lib"));
      }
    } else {
      parser::add_variable(variables, var + ".lib_dir", ldf.first, true, true);
      parser::add_variable(variables, var + ".deps", lib_filename, true, true);
      cflags =
          "-DHAS_" + sanitize_name_for_define(lib_name) + " -I" + hdf.first;
      ldflags = " -L" + ldf.first +
                (ns2::startswith(ns2::lower(ldf.second), "lib")
                     ? " -l" + lib_name
                     : " -l:" + ldf.second);
    }
    parser::add_variable(variables, var + ".flags", cflags + " " + ldflags,
                         true, true);
    parser::add_variable(variables, var + ".cflags", cflags, true, true);
    parser::add_variable(variables, var + ".ldflags", ldflags, true, true);
  } else {
    if (verbosity >= VERBOSITY_NORMAL) {
      OUTPUT << "Library '" << lib_name << "' not found" << std::endl;
    }
    if (die_on_not_found) {
      exit(EXIT_FAILURE);
    }
    parser::add_variable(variables, var + ".header_dir", "", true, true);
    parser::add_variable(variables, var + ".lib_dir", "", true, true);
    parser::add_variable(variables, var + ".flags", "", true, true);
    parser::add_variable(variables, var + ".cflags", "", true, true);
    parser::add_variable(variables, var + ".ldflags", "", true, true);
    parser::add_variable(variables, var + ".deps", "", true, true);
  }
}
