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

#include "compiler.hpp"
#include "nsconfig.hpp"
#include "shell.hpp"
#include <cstdlib>
#include <map>
#include <ns2/process.hpp>
#include <ns2/fs.hpp>
#include <string>

namespace compiler {

// ----------------------------------------------------------------------------

std::ostream &operator<<(std::ostream &os, const infos_t &ci) {
  os << "\"" << ci.path << "\" version " << ns2::to_string(ci.version)
     << " for " << (ci.arch == compiler::infos_t::Intel ? "Intel" : "ARM")
     << " " << ns2::to_string(ci.nbits) << " bits";
  return os;
}

// ----------------------------------------------------------------------------

std::string get_correpsonding_cpp_comp(std::string const &c_comp) {
  if (c_comp == "gcc") {
    return "g++";
  } else if (c_comp == "cl") {
    return "cl";
  } else if (c_comp == "clang") {
    return "clang++";
  } else if (c_comp == "armclang") {
    return "armclang++";
  } else if (c_comp == "icc") {
    return "icc";
  } else {
    return "";
  }
}

// ----------------------------------------------------------------------------

int get_type(infos_t *ci, std::string const &str) {
  if (str == "gcc" || str == "g++") {
    ci->type = compiler::infos_t::GCC;
    return 0;
  } else if (str == "clang" || str == "clang++") {
    ci->type = compiler::infos_t::Clang;
    return 0;
  } else if (str == "armclang" || str == "armclang++") {
    ci->type = compiler::infos_t::ARMClang;
    return 0;
  } else if (str == "cl") {
    ci->type = compiler::infos_t::MSVC;
    return 0;
  } else if (str == "icc") {
    ci->type = compiler::infos_t::ICC;
    return 0;
  } else if (str == "nvcc") {
    ci->type = compiler::infos_t::NVCC;
    return 0;
  }
  return -1;
}

// ----------------------------------------------------------------------------

static bool is_version_number(std::string const &str) {
  if (str.size() == 0) {
    return false;
  }
  for (size_t i = 0; i < str.size(); i++) {
    if (str[i] == '.' || (str[i] >= '0' && str[i] <= '9')) {
      continue;
    }
    return false;
  }
  return true;
}

// ----------------------------------------------------------------------------

static std::vector<std::string> get_version_digits(std::string const &str) {
  std::vector<std::string> words = ns2::split(
      ns2::replace(ns2::replace(ns2::replace(str, 'V', ' '), '+', ' '), '-',
                   ' '),
      ' ');
  for (size_t i = 0; i < words.size(); i++) {
    if (is_version_number(words[i])) {
      return ns2::split(words[i], '.');
    }
  }
  return std::vector<std::string>();
}

// ----------------------------------------------------------------------------

static bool can_exec(std::string const &str) {
#ifdef NS2_IS_MSVC
  std::string buf(str + " 1>nul 2>nul");
#else
  std::string buf(str + " 1>/dev/null 2>/dev/null");
#endif
  return system(buf.c_str()) == 0;
}

// ----------------------------------------------------------------------------

static void automatic_detection(infos_t *ci, std::string const &name,
                                int verbosity) {
  if (name == "cc") {
    ci->lang = compiler::infos_t::C;
    if (verbosity >= VERBOSITY_NORMAL) {
      OUTPUT << "Automatic C compiler detection" << std::endl;
    }
#ifdef NS2_IS_MSVC
    // On Windows we start by MSVC, then Clang, then mingw
    if (can_exec("cl")) {
      ci->path = "cl";
      ci->type = infos_t::MSVC;
    } else if (can_exec("clang --version")) {
      ci->path = "clang";
      ci->type = infos_t::Clang;
    } else if (can_exec("gcc --version")) {
      ci->path = "gcc";
      ci->type = compiler::infos_t::GCC;
    } else {
      goto lbl_error_compiler;
    }
#else
    // On Linux we start by Clang, then GCC
    if (can_exec("clang --version")) {
      ci->path = "clang";
      ci->type = infos_t::Clang;
    } else if (can_exec("gcc --version")) {
      ci->path = "gcc";
      ci->type = compiler::infos_t::GCC;
    } else {
      goto lbl_error_compiler;
    }
#endif
  } else if (name == "c++") {
    ci->lang = compiler::infos_t::CPP;
    if (verbosity >= VERBOSITY_NORMAL) {
      OUTPUT << "Automatic C++ compiler detection" << std::endl;
    }
#ifdef NS2_IS_MSVC
    // On Windows we start by MSVC, then Clang, then mingw
    if (can_exec("cl")) {
      ci->path = "cl";
      ci->type = infos_t::MSVC;
    } else if (can_exec("clang++ --version")) {
      ci->path = "clang++";
      ci->type = infos_t::Clang;
    } else if (can_exec("g++ --version")) {
      ci->path = "g++";
      ci->type = compiler::infos_t::GCC;
    } else {
      goto lbl_error_compiler;
    }
#else
    // On Linux we start by Clang, then GCC
    if (can_exec("clang++ --version")) {
      ci->path = "clang++";
      ci->type = infos_t::Clang;
    } else if (can_exec("g++ --version")) {
      ci->path = "g++";
      ci->type = compiler::infos_t::GCC;
    } else {
      goto lbl_error_compiler;
    }
#endif
  } else {
    ci->lang = compiler::infos_t::Unknown;
    ci->path = name;
    get_type(ci, name);
  }
  return;

lbl_error_compiler:
  OUTPUT << "Cannot find a viable compiler" << std::endl;
  exit(EXIT_FAILURE);
}

// ----------------------------------------------------------------------------

static int get_host_nbits() {
#ifdef _MSC_VER
  // Returns 32 or 64 depending on the machine where nsconfig is running.
  // We do not use sizeof(void*) or other compile time tricks as we can
  // imagine a 32-bits nsconfig running on a 64-bits OS in which case this
  // function has to return 64. On Windows this is easy: it suffices to look
  // into the PROCESSOR_ARCHITECTURE environment variable. But this environment
  // variable depends on the processus. For 32-bits processus running on a
  // 64-bits system (WOW64 process) this variable is not set to correspondond
  // to the 64-bits system. Moreover using the Win32 API to determine system
  // informations via GetNativeSystemInfo or IsWow64Process does not work
  // properly on ARM. The only Win32 API that does work properly is
  // IsWow64Process2 but it is only available on Windows 10 and later. Another
  // way is to run the native cmd.exe and echo %PROCESSOR_ARCHITECTURE%.
  // But for WOW64 processes the system does file system redirections and
  // %WINDIR%\system32 is in fact %WINDIR%\SysWOW64 and one has to use
  // %WINDIR%\Sysnative. But the latter is only available for WOW64 processes.
  // So the logic is the following:
  // - If we have been compiled in 64 bits then we can only run on a 64 bits
  //   system, so we return 64
  // - Getting here means that we have been compiled in 32 bits mode in which
  //   case %WINDIR%\SysWOW64 exists if and only if we are running on a 64 bits
  //   system so we use GetSystemWow64DirectoryA.
#ifdef _WIN64
  return 64;
#else
  char buf[MAX_PATH];
  unsigned int ret = GetSystemWow64DirectoryA(buf, MAX_PATH - 1);
  if (ret == 0 && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED) {
    return 32;
  } else {
    return 64;
  }
#endif
#else
  // On Linux the situation is more simplier. We run the uname command and
  // determine the system we are running on.
  std::pair<std::string, int> ret = ns2::popen("uname -m");
  if (ns2::startswith(ret.first, "x86_64") ||
      ns2::startswith(ret.first, "aarch64") ||
      ns2::startswith(ret.first, "arm64") ||
      ns2::startswith(ret.first, "ppc64le")) {
    return 64;
  } else {
    return 32;
  }
#endif
}

// ----------------------------------------------------------------------------

static int get_version(std::vector<std::string> const &digits,
                       int nb_significant_digits) {
  int ret = 0;
  int power = 1;
  for (size_t i = size_t(nb_significant_digits) - 1; i != 0; i--) {
    if (i < digits.size()) {
      int d = ::atoi(digits[i].c_str());
      ret += (d > 9 ? 9 : d) * power;
    }
    power *= 10;
  }
  return ret + power * ::atoi(digits[0].c_str());
}

// ----------------------------------------------------------------------------

static void set_version_arch(infos_t *ci, parser::infos_t *pi_) {
  parser::infos_t &pi = *pi_;
  // variables must be declared here because of all the goto's
  ns2::ifile_t in;
  std::vector<std::string> digits;
  std::string line;
  infos_t host_ci;

  // filename that will receive compiler infos
  std::string filename(ns2::sanitize(
      pi.build_dir + "/" + ns2::replace(ci->path, '/', '.') + "-version.txt"));

  // craft command line for filling filename and execute
  std::string cmd;
  if (ci->type == compiler::infos_t::MSVC) {
    cmd = ci->path + " 1>" + shell::stringify(filename) + " 2>&1";
  } else if (ci->type == compiler::infos_t::NVCC) {
    cmd = ci->path + " --version 1>" + shell::stringify(filename) + " 2>&1";
  } else {
    cmd = ci->path + " -v 1>" + shell::stringify(filename) + " 2>&1";
  }
  if (system(cmd.c_str()) != 0) {
    OUTPUT << "Command \"" << cmd << "\" fails" << std::endl;
    goto lbl_error_version;
  }

  // determine compiler version

  // search for the line that begins with "version" or "Version"
  in.open(filename);
  while (std::getline(in, line)) {
    if (line.find("version") != std::string::npos ||
        line.find("Version") != std::string::npos ||
        (ci->type == compiler::infos_t::ICC &&
         line.find("icc (ICC) ") != std::string::npos) ||
        (ci->type == compiler::infos_t::NVCC &&
         line.find("release") != std::string::npos)) {
      digits = get_version_digits(line);
    }
  }

  // compute compiler version
  switch (ci->type) {
  case compiler::infos_t::GCC:
    if (digits.size() < 2) {
      goto lbl_error_version;
    }
    ci->version = get_version(digits, 4);
    break;
  case compiler::infos_t::Clang:
  case compiler::infos_t::ARMClang:
    if (digits.size() < 2) {
      goto lbl_error_version;
    }
    ci->version = get_version(digits, 3);
    break;
  case compiler::infos_t::MSVC:
    if (digits.size() < 2) {
      goto lbl_error_version;
    }
    ci->version = ::atoi(digits[0].c_str()) * 100 + ::atoi(digits[1].c_str());
    break;
  case compiler::infos_t::ICC:
    if (digits.size() < 2) {
      goto lbl_error_version;
    }
    ci->version = get_version(digits, 3);
    break;
  case compiler::infos_t::NVCC:
    ci->version = get_version(digits, 2);
    break;
  case compiler::infos_t::None:
    NS2_THROW(std::runtime_error, "Invalid compiler");
    break;
  }

  // compute compiler target architecture
  in.clear();
  in.seekg(0);
  ci->nbits = 0;
  switch (ci->type) {
  case infos_t::MSVC: {
    std::getline(in, line);
    std::vector<std::string> words(ns2::split(line, ' '));
    std::string archi(ns2::strip(words.back())); // remove trailing \r on WIN
    if (archi == "x64") {
      ci->arch = infos_t::Intel;
      ci->nbits = 64;
    } else if (archi == "x86") {
      ci->arch = infos_t::Intel;
      ci->nbits = 32;
    } else if (archi == "ARM") {
      ci->arch = infos_t::ARM;
      ci->nbits = 32;
    } else if (archi == "ARM64") {
      ci->arch = infos_t::ARM;
      ci->nbits = 64;
    } else {
      goto lbl_error_march;
    }
    break;
  }
  case infos_t::GCC:
  case infos_t::Clang:
  case infos_t::ARMClang:
    while (!in.eof()) {
      std::getline(in, line);
      if (ns2::startswith(line, "Target:") ||
          ns2::startswith(line, "Cible :")) {
        if (line.find("aarch64") != std::string::npos) {
          ci->arch = infos_t::ARM;
          ci->nbits = 64;
        } else if (line.find("arm") != std::string::npos) {
          ci->arch = infos_t::ARM;
          ci->nbits = 32;
        } else if (line.find("x86_64") != std::string::npos) {
          ci->arch = infos_t::Intel;
          ci->nbits = 64;
        } else {
          ci->arch = infos_t::Intel;
          ci->nbits = 32;
        }
      }
    }
    if (ci->nbits == 0) {
      goto lbl_error_march;
    }
    break;
  case infos_t::ICC:
    ci->arch = infos_t::Intel;
    ci->nbits = get_host_nbits(); // For ICC default is like host
    break;
  case infos_t::NVCC:
    // For NVCC it depends on the host compiler and by default the
    // host compiler we choose for nvcc is c++, the one that can be given at
    // nsconfig command line.
    host_ci = get("c++", &pi);
    ci->arch = host_ci.arch;
    ci->nbits = host_ci.nbits;
    break;
  case infos_t::None:
    NS2_THROW(std::runtime_error, "Invalid compiler");
    break;
  }

  if (pi.verbosity >= VERBOSITY_NORMAL) {
    OUTPUT << "Compiler: " << *ci << std::endl;
  }
  return;

lbl_error_march:
  if (pi.verbosity >= VERBOSITY_NORMAL) {
    OUTPUT << "Cannot determine compiler target architecture" << std::endl;
  }
  exit(EXIT_FAILURE);

lbl_error_version:
  if (pi.verbosity >= VERBOSITY_NORMAL) {
    OUTPUT << "Cannot determine compiler version" << std::endl;
  }
  exit(EXIT_FAILURE);
}

// ----------------------------------------------------------------------------

infos_t get(std::string name, parser::infos_t *pi_) {
  parser::infos_t &pi = *pi_;
  infos_t ci;

  // do we already have encountered this compiler?
  list_t::iterator it = pi.compilers.find(name);
  if (it == pi.compilers.end()) {
    ci.name = name;
    ci.fully_filled = false;
    automatic_detection(&ci, name, pi.verbosity);
  } else {
    ci = it->second;
  }

  // if ci was not fully filled then fill it
  if (!ci.fully_filled) {
    set_version_arch(&ci, &pi);
    ci.fully_filled = true;
    pi.compilers[name] = ci;
  }

  // if the compiler was not known yet, then put it into the map
  if (it == pi.compilers.end()) {
    pi.compilers[name] = ci;
  }

  return ci;
}

// ----------------------------------------------------------------------------

std::string get_type_str(compiler::infos_t::type_t const compiler_type) {
  switch (compiler_type) {
  case compiler::infos_t::GCC:
    return "gcc";
  case compiler::infos_t::Clang:
    return "clang";
  case compiler::infos_t::ARMClang:
    return "clang";
  case compiler::infos_t::MSVC:
    return "msvc";
  case compiler::infos_t::ICC:
    return "icc";
  case compiler::infos_t::NVCC:
    return "nvcc";
  case compiler::infos_t::None:
    return "none";
  }
  return "none";
}

std::string
get_type_and_lang_str(compiler::infos_t::type_t const compiler_type,
                      compiler::infos_t::lang_t const compiler_lang) {
  if (compiler_lang == compiler::infos_t::CPP) {
    if (compiler_type == compiler::infos_t::GCC) {
      return "g++";
    } else if (compiler_type == compiler::infos_t::Clang) {
      return "clang++";
    }
  }
  return get_type_str(compiler_type);
}

// ----------------------------------------------------------------------------

} // namespace compiler
