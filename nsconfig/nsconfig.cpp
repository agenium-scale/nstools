// MIT License
//
// Copyright (c) 2020 Agenium Scale
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
#include "compiler.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <ns2/fs.hpp>
#include <stdexcept>

// ----------------------------------------------------------------------------

static void help(FILE *out) {
  // clang-format off
  fputs(
  "usage: nsconfig [OPTIONS]... DIRECTORY\n"
  "Configure project for compilation.\n"
  "\n"
  "  -v              verbose mode, useful for debugging\n"
  "  -nodev          Build system will never call nsconfig\n"
  "  -DVAR=VALUE     Set value of variable VAR to VALUE\n"
  "  -list-vars      List project specific variable\n"
  "  -GBUILD_SYSTEM  Produce files for build system BUILD_SYSTEM\n"
  "                  Supported BUILD_SYSTEM:\n"
  "                    make       POSIX Makefile\n"
  "                    gnumake    GNU Makefile\n"
  "                    nmake      Microsot Visual Studio NMake Makefile\n"
  "                    ninja      Ninja build file (this is the default)\n"
  "                    list-vars  List project specific variables\n"
  "  -oOUTPUT        Output to OUTPUT instead of default\n"
  "  -suite=SUITE    Use compilers from SUITE as default ones\n"
  "                  Supported SUITE:\n"
  "                    gcc       The GNU compiler collection\n"
  "                    msvc      Microsoft C and C++ compiler\n"
  "                    llvm      The LLVM compiler infrastructure\n"
  "                    armclang  Arm suite of compilers based on LLVM\n"
  "                    fcc_trad_mode\n"
  "                              Fujitsu compiler in traditional mode\n"
  "                    fcc_clang_mode\n"
  "                              Fujitsu compiler in clang mode\n"
  "                    icc       Intel C amd C++ compiler\n"
  "                    rocm      Radeon Open Compute compilers\n"
  "                    cuda, cuda+gcc, cuda+clang, cuda+msvc\n"
  "                              Nvidia CUDA C++ compiler\n"
  "  -comp=COMMAND,COMPILER[,PATH[,VERSION[,ARCHI]]]\n"
  "                  Use COMPILER when COMMAND is invoked for compilation\n"
  "                  If VERSION and/or ARCHI are not given, nsconfig will\n"
  "                  try to determine those. This is useful for cross\n"
  "                  compiling and/or setting the CUDA host compiler.\n"
  "                  COMMAND must be in { cc, c++, gcc, g++, cl, icc, nvcc,\n"
  "                  hipcc, hcc, clang, clang++, armclang, armclang++,\n"
  "                  cuda-host-c++ } ;\n"
  "                  VERSION is compiler dependant. Note that VERSION\n"
  "                  can be set to only major number(s) in which case\n"
  "                  nsconfig fill missing numbers with zeros.\n"
  "                  Supported ARCHI:\n"
  "                    x86      Intel 32-bits ISA\n"
  "                    x86_64   Intel/AMD 64-bits ISA\n"
  "                    armel    ARMv5 and ARMv6 32-bits ISA\n"
  "                    armhf    ARMv7 32-bits ISA\n"
  "                    aarch64  ARM 64-bits ISA\n"
  "                  Supported COMPILER:\n"
  "                    gcc, g++              GNU Compiler Collection\n"
  "                    clang, clang++        LLVM Compiler Infrastructure\n"
  "                    msvc, cl              Microsoft Visual C++\n"
  "                    armclang, armclang++  ARM Compiler\n"
  "                    icc                   Intel C/C++ Compiler\n"
  "                    dpcpp                 Intel DPC++ Compiler\n"
  "                    nvcc                  Nvidia CUDA compiler\n"
  "                    hipcc                 ROCm HIP compiler\n"
  "                    fcc_trad_mode, FCC_trad_mode\n"
  "                                          Fujitsu C and C++ traditionnal\n"
  "                                          compiler\n"
  "                    fcc_clang_mode, FCC_clang_mode\n"
  "                                          Fujitsu C and C++ traditionnal\n"
  "                                          compiler\n"
  "  -prefix=PREFIX  Set path for installation to PREFIX\n"
  "  -h, --help      Print the current help\n"
  "\n"
  "NOTE: Nvidia CUDA compiler (nvcc) needs a host compiler. Usually on\n"
  "      Linux systems it is GCC while on Windows systems it is MSVC.\n"
  "      If nvcc is chosen as the default C++ compiler via the -suite\n"
  "      switch, then its host compiler can be invoked in compilation\n"
  "      commands with 'cuda-host-c++'. The latter defaults to GCC on Linux\n"
  "      systems and MSVC on Windows systems. The user can of course choose\n"
  "      a specific version and path of this host compiler via the\n"
  "      '-comp=cuda-hostc++,... parameters. If nvcc is not chosen as the\n"
  "      default C++ compiler but is used for compilation then its default\n"
  "      C++ host compiler is 'c++'. The latter can also be customized via\n"
  "      the '-comp=c++,...' command line switch.\n"
  , out);
  // clang-format on
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
    if (!memcmp(argv[i], "-suite=", 7)) {
      std::string suite(argv[i] + 7);
      if (suite != "gcc" && suite != "msvc" && suite != "llvm" &&
          suite != "armclang" && suite != "icc" && suite != "rocm" &&
          suite != "cuda" && suite != "cuda+gcc" && suite != "cuda+clang" &&
          suite != "cuda+msvc" && suite != "fcc_trad_mode" &&
          suite != "fcc_clang_mode") {
        NS2_THROW(std::runtime_error,
                  "unknown suite given at command line: " + suite);
      }
      OUTPUT << "Using compiler from suite: " << suite << '\n';
      std::string c_comp(compiler::get_corresponding_c_comp(suite));
      if (c_comp.size() == 0) {
        OUTPUT << "No C compiler found in suite\n";
      } else {
        OUTPUT << "C compiler found in suite: " << c_comp << '\n';
        compiler::infos_t ci;
        ci.name = c_comp;
        compiler::get_path_type_and_lang(&ci, c_comp);
        ci.fully_filled = false;
        pi.compilers["cc"] = ci;
      }
      std::string cpp_comp(compiler::get_corresponding_cpp_comp(suite));
      if (cpp_comp.size() == 0) {
        OUTPUT << "No C++ compiler found in suite\n";
      } else {
        OUTPUT << "C++ compiler found in suite: " << cpp_comp << '\n';
        compiler::infos_t ci;
        ci.name = cpp_comp;
        compiler::get_path_type_and_lang(&ci, cpp_comp);
        ci.fully_filled = false;
        pi.compilers["c++"] = ci;
      }
      // If suite is cuda+XXX then we have to set the host compiler to XXX
      if (ns2::startswith(suite, "cuda")) {
        compiler::infos_t ci;
        if (suite == "cuda") {
#ifdef NS2_IS_MSVC
          ci.name = "cl";
#else
          ci.name = "g++";
#endif
        } else if (suite == "cuda+gcc") {
          ci.name = "g++";
        } else if (suite == "cuda+clang") {
          ci.name = "clang++";
        } else if (suite == "cuda+msvc") {
          ci.name = "cl";
        }
        OUTPUT << "CUDA host C++ compiler will be: " << ci.name << '\n';
        compiler::get_path_type_and_lang(&ci, ci.name);
        ci.fully_filled = false;
        pi.compilers["cuda-host-c++"] = ci;
      }
      continue;
    }
    if (!memcmp(argv[i], "-comp=", 6)) {
      std::vector<std::string> words(ns2::split(argv[i] + 6, ','));
      if (words.size() <= 1) {
        NS2_THROW(std::runtime_error, "misformed -comp argument");
      }
      // In order: COMMAND, COMPILER, PATH, VERSION, ARCHI
      //              0         1      2       3       4
      if (!shell::command_is_compiler(words[0])) {
        NS2_THROW(std::runtime_error,
                  "unknown compilation command given at -comp");
      }
      compiler::infos_t ci;
      ci.fully_filled = false;
      if (compiler::get_path_type_and_lang(&ci, words[1]) == -1) {
        NS2_THROW(std::runtime_error, "unknown compiler given at -comp");
      }
      if (words.size() >= 3) {
        ci.path = words[2];
      }
      if (words.size() >= 4) {
        compiler::get_version_from_string(&ci, ns2::split(words[3], '.'));
      }
      if (words.size() >= 5) {
        compiler::get_archi_from_string(&ci, words[4]);
        ci.fully_filled = true;
      }
      pi.compilers[words[0]] = ci;
      continue;
    }
    if (!memcmp(argv[i], "-G", 2)) {
      if (generator) {
        NS2_THROW(std::runtime_error,
                  "generator already given at command line");
      }
      generator = &argv[i][2];
      if (strcmp(generator, "make") && strcmp(generator, "nmake") &&
          strcmp(generator, "ninja") && strcmp(generator, "gnumake") &&
          strcmp(generator, "list-vars")) {
        NS2_THROW(std::runtime_error,
                  "unknown generator given at command line");
      }
      continue;
    }
    if (!memcmp(argv[i], "-o", 2)) {
      if (output) {
        NS2_THROW(std::runtime_error,
                  "output file already given at command line");
      }
      output = &argv[i][2];
      pi.build_dir = ns2::absolute(ns2::split_path(output).first);
      continue;
    }
    if (!memcmp(argv[i], "-D", 2)) {
      const char *arg = &argv[i][2];
      const char *equal = strchr(arg, '=');
      if (equal == NULL) {
        NS2_THROW(std::runtime_error,
                  "cannot parse define directive at command line");
      }
      std::string key(arg, equal);
      std::string val(equal + 1);
      parser::add_variable(&pi.variables, key, val, false, false);
      continue;
    }
    if (!ns2::exists(argv[i])) {
      NS2_THROW(std::runtime_error,
                "cannot access source dir at command line");
    } else {
      pi.source_dir = ns2::absolute(argv[i]);
      continue;
    }
    NS2_THROW(std::runtime_error,
              std::string("unknown argument given at command line: ") +
                  argv[i]);
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
