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

#include "backend_make.hpp"
#include "backend_ninja.hpp"
#include "parser.hpp"
#include "shell.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <ns2/fs.hpp>
#include <stdexcept>

// ----------------------------------------------------------------------------

static std::string add_comp(parser::infos_t *pi, std::string const &type,
                            std::string const &path, std::string const &name,
                            std::string const &version = std::string(),
                            std::string const &archi = std::string()) {
  // This is à la C, do not want to have another function just for that...
  if (pi == NULL) {
    return "Supported compilers:\n"
           "  gcc, g++                GNU Compiler Collection\n"
           "  clang, clang++          LLVM Compiler Infrastructure\n"
           "  msvc                    Microsoft Visual C++\n"
           "  armclang, armclang++    ARM Compiler\n"
           "  icc                     Intel C/C++ Compiler\n"
           "  dpcpp                   Intel DPC++ Compiler\n"
           "  nvcc                    Nvidia CUDA compiler\n"
           "  hipcc                   ROCm HIP compiler\n";
  }

  // Getting here means we want the "normal" behavior of this function
  compiler::infos_t ci;
  if (compiler::get_type(&ci, type) == -1) {
    return "unknown compiler type: \"" + type + "\"";
  }
  ci.name = name;
  ci.path = path;
  if (version.size() > 0 && archi.size()) {
    compiler::get_version_from_string(&ci, ns2::split(version, '.'));
    compiler::get_archi_from_string(&ci, archi);
    ci.fully_filled = true;
  } else {
    ci.fully_filled = false;
  }
  pi->compilers[name] = ci;
  return std::string();
}

// ----------------------------------------------------------------------------

static void help(FILE *out) {
#define P(msg) fputs(msg "\n", out)
  P("usage: nsconfig [OPTIONS]... DIRECTORY");
  P("Configure project for compilation.");
  P("");
  P("  -v                   verbose mode");
  P("  -nodev               Build system will never call nsconfig");
  P("  -DVAR=VALUE          Set value of variable VAR to VALUE");
  P("  -list-vars           List project specific variable");
  P("  -GBUILD_SYSTEM       Produce files for build system BUILD_SYSTEM");
  P("  -Ghelp               List supported build systems");
  P("  -oOUTPUT             Output to OUTPUT instead of default");
  P("  -comp=SUITE          Use compiler SUITE for both C and C++");
  P("  -ccomp=SUITE,PATH[,VERSION[,ARCHI]]");
  P("                       Use PATH as default C compiler from suite SUITE");
  P("                       If VERSION and/or ARCHI are given, nsconfig");
  P("                       try to determine those. This is useful for");
  P("                       cross compiling, ARCHI must be in { x86, x86_64,");
  P("                       arm, aarch64 }");
  P("  -ccomp=help          List supported C compilers");
  P("  -cppcomp=SUITE,PATH[,VERSION[,ARCHI]]");
  P("                       Use PATH as default C++ compiler from suite "
    "SUITE");
  P("                       If VERSION and/or ARCHI are given, nsconfig");
  P("                       try to determine those. This is useful for");
  P("                       cross compiling, ARCHI must be in { x86, x86_64,");
  P("                       arm, aarch64 }");
  P("  -cppcomp=help        List supported C++ compilers");
  P("  -prefix=PREFIX       Set prefix for installation to PREFIX");
  P("  -h, --help           Print the current text");
#undef P
}

// ----------------------------------------------------------------------------

int main2(int argc, char **argv) {
  const char *generator = NULL;
  const char *output = NULL;
  parser::infos_t pi;
  pi.source_dir = ns2::absolute("");
  pi.verbosity = VERBOSITY_NORMAL;
  pi.generate_self = true;
  pi.build_dir = ns2::absolute("");
#ifdef NS2_IS_MSVC
  pi.install_prefix = "C:/Program Files";
#else
  pi.install_prefix = "/opt/local";
#endif
  pi.package_name.clear();

  // parse arguments
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
      help(stdout);
      fflush(stdout);
      return 0;
    }
    if (!strcmp(argv[i], "-list-vars")) {
      generator = "list-vars";
      continue;
    }
    if (!strcmp(argv[i], "-v")) {
      pi.verbosity = VERBOSITY_DEBUG;
      continue;
    }
    if (!strcmp(argv[i], "-nodev")) {
      pi.generate_self = false;
      continue;
    }
    if (!memcmp(argv[i], "-prefix=", 8)) {
      pi.install_prefix = std::string(argv[i] + 8);
      continue;
    }
    if (!memcmp(argv[i], "-comp=", 6)) {
      std::string c_comp(argv[i] + 6);
      std::string cpp_comp(compiler::get_correpsonding_cpp_comp(c_comp));
      if (cpp_comp.size() == 0) {
        std::cerr << argv[0]
                  << ": ERROR: cannot get corresponding C++ compiler of \""
                  << c_comp << "\"" << std::endl;
        exit(EXIT_FAILURE);
      }
      std::string code = add_comp(&pi, c_comp, c_comp, "cc");
      if (code.size() > 0) {
        std::cerr << argv[0] << ": ERROR: " << code << std::endl;
        exit(EXIT_FAILURE);
      }
      code = add_comp(&pi, cpp_comp, cpp_comp, "c++");
      if (code.size() > 0) {
        std::cerr << argv[0] << ": ERROR: " << code << std::endl;
        exit(EXIT_FAILURE);
      }
      continue;
    }
    if (!memcmp(argv[i], "-ccomp=", 7)) {
      std::vector<std::string> words(ns2::split(argv[i] + 7, ','));
      if (words.size() == 1 && words[0] == "help") {
        std::cout << argv[0] << ": "
                  << add_comp(NULL, std::string(), std::string(),
                              std::string());
        exit(EXIT_SUCCESS);
      }
      std::string code;
      if (words.size() != 2 && words.size() != 4) {
        std::cerr << argv[0] << ": ERROR: wrong format for C compiler"
                  << std::endl;
        exit(EXIT_FAILURE);
      } else if (words.size() == 2) {
        code = add_comp(&pi, words[0], words[1], "cc");
      } else if (words.size() == 4) {
        code = add_comp(&pi, words[0], words[1], "cc", words[2], words[3]);
      }
      if (code.size() > 0) {
        std::cerr << argv[0] << ": ERROR: " << code << std::endl;
        exit(EXIT_FAILURE);
      }
      continue;
    }
    if (!memcmp(argv[i], "-cppcomp=", 9)) {
      std::vector<std::string> words(ns2::split(argv[i] + 9, ','));
      if (words.size() == 1 && words[0] == "help") {
        std::cout << argv[0] << ": "
                  << add_comp(NULL, std::string(), std::string(),
                              std::string());
        exit(EXIT_SUCCESS);
      }
      std::string code;
      if (words.size() != 2 && words.size() != 4) {
        std::cerr << argv[0] << ": ERROR: wrong format for C++ compiler"
                  << std::endl;
        exit(EXIT_FAILURE);
      } else if (words.size() == 2) {
        code = add_comp(&pi, words[0], words[1], "c++");
      } else if (words.size() == 4) {
        code = add_comp(&pi, words[0], words[1], "c++", words[2], words[3]);
      }
      if (code.size() > 0) {
        std::cerr << argv[0] << ": ERROR: " << code << std::endl;
        exit(EXIT_FAILURE);
      }
      continue;
    }
    if (!strcmp(argv[i], "-Ghelp")) {
      std::cout << argv[0] << ": Supported generators:\n"
                << "  make       POSIX Makefile\n"
                << "  gnumake    GNU Makefile\n"
                << "  nmake      Microsot Visual Studio NMake Makefile\n"
                << "  ninja      Ninja build file (this is the default)\n"
                << "  list-vars  List project specific variables\n"
                << std::endl;
      return 0;
    }
    if (!memcmp(argv[i], "-G", 2)) {
      if (generator) {
        std::cerr << argv[0] << ": ERROR: generator already given"
                  << std::endl;
        return -1;
      }
      generator = &argv[i][2];
      if (strcmp(generator, "make") && strcmp(generator, "nmake") &&
          strcmp(generator, "ninja") && strcmp(generator, "gnumake") &&
          strcmp(generator, "list-vars")) {
        std::cerr << argv[0] << ": ERROR: unknown generator: " << generator
                  << std::endl;
        return -1;
      }
      continue;
    }
    if (!memcmp(argv[i], "-o", 2)) {
      if (output) {
        std::cerr << argv[0] << ": ERROR: output already given" << std::endl;
        return -1;
      }
      output = &argv[i][2];
      pi.build_dir = ns2::absolute(ns2::split_path(output).first);
      continue;
    }
    if (!memcmp(argv[i], "-D", 2)) {
      const char *arg = &argv[i][2];
      const char *equal = strchr(arg, '=');
      if (equal == NULL) {
        std::cerr << argv[0] << ": ERROR: cannot parse define directive"
                  << std::endl;
        return -1;
      }
      std::string key(arg, equal);
      std::string val(equal + 1);
      parser::add_variable(&pi.variables, key, val, false, false);
      continue;
    }
    if (!ns2::exists(argv[i])) {
      std::cerr << argv[0] << ": ERROR: cannot access source dir: " << argv[i]
                << std::endl;
      return -1;
    } else {
      pi.source_dir = ns2::absolute(argv[i]);
      continue;
    }
    std::cerr << argv[0] << ": ERROR: unknown argument: " << argv[i]
              << std::endl;
    return -1;
  }

  // reconstruct command line
  std::string cmdline(argv[0]);
  for (int i = 1; i < argc; i++) {
    cmdline += " " + shell::ify(argv[i]);
  }

  // open nsconfig file and work on it
  ns2::ifile_t in(pi.source_dir + "/build.nsconfig");
  pi.package_name = "package";
  pi.generate_all = true;
  pi.generate_clean = true;
  pi.generate_update = true;
  pi.generate_install = true;
  pi.generate_update = true;
  pi.generate_package = true;
  pi.translate = true;
  pi.begin_translate = false;
  pi.build_nsconfig = shell::ify(pi.source_dir + "/build.nsconfig");
  pi.cmdline = cmdline;

  if (generator == NULL || !strcmp(generator, "ninja")) {

    std::string temp(output ? output : "build.ninja");
    pi.output_file = shell::ify(temp);
    pi.make_command = "ninja";
    pi.backend_supports_self_generation = true;
    pi.backend_supports_header_deps = true;
    rules_t rules = parser::parse(in, &pi);
    std::string msvc_path;
    for (compiler::list_t::const_iterator it = pi.compilers.begin();
         it != pi.compilers.end(); ++it) {
      if ((it->second).type == compiler::infos_t::MSVC) {
        msvc_path = (it->second).path;
        break;
      }
    }
    backend::ninja(rules, temp, cmdline, msvc_path);

  } else if (!strcmp(generator, "list-vars")) {

    if (pi.verbosity == VERBOSITY_NORMAL) {
      pi.verbosity = VERBOSITY_QUIET;
    }
    parser::list_variables(in, &pi);
    parser::infos_t::vars_list_t &vars = pi.vars_list;
    if (vars.size() == 0) {
      std::cout << "Project variables list: (none)" << std::endl;
    } else {
      std::cout << "Project variables list:" << std::endl;
      size_t max_len_var_name = 0;
      size_t max_len_var_desc = 0;
      for (size_t i = 0; i < vars.size(); i++) {
        max_len_var_name = maximum(max_len_var_name, vars[i].var_name.size());
        max_len_var_desc = maximum(max_len_var_desc, vars[i].helper.size());
      }
      // In what follows, 4 = sizeof("name") - 1 = 5 - 1
      // In what follows, 11 = sizeof("description") - 1 = 12 - 1
      max_len_var_name = maximum(max_len_var_name, size_t(4)) + 1;
      max_len_var_desc = maximum(max_len_var_desc, size_t(11)) + 1;
      std::cout << "name" << std::string(max_len_var_name - 4, ' ')
                << "| description" << std::endl;
      std::cout << std::string(max_len_var_name, '-') << "|"
                << std::string(max_len_var_desc, '-') << std::endl;
      for (size_t i = 0; i < vars.size(); i++) {
        std::cout << vars[i].var_name
                  << std::string(max_len_var_name - vars[i].var_name.size(),
                                 ' ')
                  << "| " << vars[i].helper << std::endl;
      }
    }

  } else {

    std::string temp(output ? output : "Makefile");
    pi.output_file = shell::ify(temp);
    pi.backend_supports_self_generation = false;
    pi.backend_supports_header_deps = false;
    pi.make_command = strcmp(generator, "nmake") ? "make" : "nmake";
    rules_t rules = parser::parse(in, &pi);

    if (!strcmp(generator, "make")) {
      backend::make(rules, temp, backend::make_t::POSIX, cmdline);
    } else if (!strcmp(generator, "gnumake")) {
      backend::make(rules, temp, backend::make_t::GNU, cmdline);
    } else if (!strcmp(generator, "nmake")) {
      backend::make(rules, temp, backend::make_t::MSVC, cmdline);
    }
  }

  return 0;
}

// ----------------------------------------------------------------------------

int main(int argc, char **argv) {
  try {
    return main2(argc, argv);
  } catch (std::exception const &e) {
    std::cerr << argv[0] << ": error: " << e.what() << std::endl;
    exit(EXIT_FAILURE);
  }
}
