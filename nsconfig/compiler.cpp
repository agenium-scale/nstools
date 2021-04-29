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
#include <ns2/exception.hpp>
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

std::string get_corresponding_cpp_comp(std::string const &suite) {
  if (suite == "gcc") {
    return "g++";
  } else if (suite == "msvc") {
    return "cl";
  } else if (suite == "llvm") {
    return "clang++";
  } else if (suite == "armclang") {
    return "armclang++";
  } else if (suite == "fcc_trad_mode") {
    return "FCC_trad_mode";
  } else if (suite == "fcc_clang_mode") {
    return "FCC_clang_mode";
  } else if (suite == "emscripten") {
    return "em++";
  } else if (suite == "icc") {
    return "icc";
  } else if (suite == "oneapi") {
    return "dpcpp";
  } else if (suite == "rocm") {
    return "hipcc";
  } else if (suite == "cuda" || suite == "cuda+gcc" || suite == "cuda+clang" ||
             suite == "cuda+msvc") {
    return "nvcc";
  } else {
    return "";
  }
}

// ----------------------------------------------------------------------------

std::string get_corresponding_c_comp(std::string const &suite) {
  if (suite == "gcc") {
    return "gcc";
  } else if (suite == "msvc") {
    return "cl";
  } else if (suite == "llvm") {
    return "clang";
  } else if (suite == "armclang") {
    return "armclang";
  } else if (suite == "fcc_trad_mode") {
    return "fcc_trad_mode";
  } else if (suite == "fcc_clang_mode") {
    return "fcc_clang_mode";
  } else if (suite == "emscripten") {
    return "emcc";
  } else if (suite == "icc") {
    return "icc";
  } else {
    return "";
  }
}

// ----------------------------------------------------------------------------

static int compute_version(std::vector<std::string> const &str,
                           int multiplier0, int multiplier1, int multiplier2) {
  if (str.size() == 0) {
    return 0;
  }
  int ret = 0;
  if (str.size() > 0 && multiplier0 > 0) {
    ret += ::atoi(str[0].c_str()) * multiplier0;
  }
  if (str.size() > 1 && multiplier1 > 0) {
    ret += ::atoi(str[1].c_str()) * multiplier1;
  }
  if (str.size() > 2 && multiplier2 > 0) {
    ret += ::atoi(str[2].c_str()) * multiplier2;
  }
  return ret;
}

// ----------------------------------------------------------------------------

void get_version_from_string(compiler::infos_t *ci_,
                             std::vector<std::string> const &str) {
  compiler::infos_t &ci = *ci_;
  switch (ci.type) {
  case compiler::infos_t::GCC:
  case compiler::infos_t::Clang:
  case compiler::infos_t::ARMClang:
  case compiler::infos_t::NVCC:
    ci.version = compute_version(str, 10000, 100, 1);
    break;
  case compiler::infos_t::FCC_trad_mode:
  case compiler::infos_t::FCC_clang_mode:
    ci.version = compute_version(str, 100, 10, 1);
    break;
  case compiler::infos_t::MSVC:
  case compiler::infos_t::ICC:
    ci.version = compute_version(str, 100, 1, 0);
    break;
  case compiler::infos_t::HIPCC:
  case compiler::infos_t::HCC:
  case compiler::infos_t::Emscripten:
    ci.version = compute_version(str, 10000, 100, 0);
    break;
  case compiler::infos_t::DPCpp:
    ci.version = compute_version(str, 1, 0, 0);
    break;
  case compiler::infos_t::None:
    NS2_THROW(std::runtime_error, "Invalid compiler");
    break;
  }
}

// ----------------------------------------------------------------------------

void get_archi_from_string(compiler::infos_t *ci_, std::string const &str) {
  infos_t &ci = *ci_;
  if (str == "armel" || str == "arm") {
    ci.arch = compiler::infos_t::ARMEL;
    ci.nbits = 32;
  } else if (str == "armhf") {
    ci.arch = compiler::infos_t::ARMHF;
    ci.nbits = 32;
  } else if (str == "aarch64") {
    ci.arch = compiler::infos_t::AARCH64;
    ci.nbits = 64;
  } else if (str == "ppc64el") {
    ci.arch = compiler::infos_t::PPC64EL;
    ci.nbits = 64;
  } else if (str == "x86") {
    ci.arch = compiler::infos_t::Intel;
    ci.nbits = 32;
  } else if (str == "x86_64") {
    ci.arch = compiler::infos_t::Intel;
    ci.nbits = 64;
  } else if (str == "wasm32") {
    ci.arch = compiler::infos_t::WASM;
    ci.nbits = 32;
  } else if (str == "wasm64") {
    ci.arch = compiler::infos_t::WASM;
    ci.nbits = 64;
  } else {
    NS2_THROW(std::runtime_error, "Invalid architecture");
  }
}

// ----------------------------------------------------------------------------

int get_path_type_and_lang(infos_t *ci, std::string const &str) {
  if (str == "gcc" || str == "g++") {
    ci->type = compiler::infos_t::GCC;
    ci->lang = (str == "gcc" ? compiler::infos_t::C : compiler::infos_t::CPP);
    ci->path = str;
    return 0;
  } else if (str == "clang" || str == "clang++") {
    ci->type = compiler::infos_t::Clang;
    ci->lang =
        (str == "clang" ? compiler::infos_t::C : compiler::infos_t::CPP);
    ci->path = str;
    return 0;
  } else if (str == "armclang" || str == "armclang++") {
    ci->type = compiler::infos_t::ARMClang;
    ci->lang =
        (str == "armclang" ? compiler::infos_t::C : compiler::infos_t::CPP);
    ci->path = str;
    return 0;
  } else if (str == "fcc_trad_mode" || str == "FCC_trad_mode") {
    ci->type = compiler::infos_t::FCC_trad_mode;
    ci->lang = (str == "fcc_trad_mode" ? compiler::infos_t::C
                                       : compiler::infos_t::CPP);
    if (str == "fcc_trad_mode") {
      ci->path = "fcc";
    } else {
      ci->path = "FCC";
    }
    return 0;
  } else if (str == "fcc_clang_mode" || str == "FCC_clang_mode") {
    ci->type = compiler::infos_t::FCC_clang_mode;
    ci->lang = (str == "fcc_clang_mode" ? compiler::infos_t::C
                                        : compiler::infos_t::CPP);
    if (str == "fcc_clang_mode") {
      ci->path = "fcc";
    } else {
      ci->path = "FCC";
    }
    return 0;
  } else if (str == "em++" || str == "emcc") {
    ci->type = compiler::infos_t::Emscripten;
    ci->lang = (str == "emcc" ? compiler::infos_t::C : compiler::infos_t::CPP);
    ci->path = str;
    return 0;
  } else if (str == "cl" || str == "msvc") {
    ci->type = compiler::infos_t::MSVC;
    ci->lang = compiler::infos_t::Unknown;
    ci->path = "cl";
    return 0;
  } else if (str == "icc") {
    ci->type = compiler::infos_t::ICC;
    ci->lang = compiler::infos_t::Unknown;
    ci->path = "icc";
    return 0;
  } else if (str == "nvcc") {
    ci->type = compiler::infos_t::NVCC;
    ci->lang = compiler::infos_t::CPP;
    ci->path = "nvcc";
    return 0;
  } else if (str == "hipcc") {
    ci->type = compiler::infos_t::HIPCC;
    ci->lang = compiler::infos_t::CPP;
    ci->path = "hipcc";
    return 0;
  } else if (str == "hcc") {
    ci->type = compiler::infos_t::HCC;
    ci->lang = compiler::infos_t::CPP;
    ci->path = "hcc";
    return 0;
  } else if (str == "dpc++" || str == "dpcpp") {
    ci->type = compiler::infos_t::DPCpp;
    ci->lang = compiler::infos_t::CPP;
    ci->path = "dpcpp";
    return 0;
  }
  return -1;
}

// ----------------------------------------------------------------------------

static bool can_exec(std::string const &str, int verbosity) {
#ifdef NS2_IS_MSVC
  std::string buf(str + " 1>nul 2>nul");
#else
  std::string buf(str + " 1>/dev/null 2>/dev/null");
#endif
  int code = system(buf.c_str());
  if (verbosity >= VERBOSITY_DEBUG) {
    OUTPUT << "Executing command: " << buf << std::endl;
    OUTPUT << "Return code is " << code << std::endl;
  }
  return code == 0;
}

// ----------------------------------------------------------------------------

static void automatic_detection(infos_t *ci, std::string const &name,
                                int verbosity) {
  if (verbosity >= VERBOSITY_DEBUG) {
    OUTPUT << "Attempting automatic detection for '" << name << "'"
           << std::endl;
  }
  if (name == "cc") {
    ci->lang = compiler::infos_t::C;
    if (verbosity >= VERBOSITY_NORMAL) {
      OUTPUT << "Automatic C compiler detection" << std::endl;
    }
#ifdef NS2_IS_MSVC
    // On Windows we start by MSVC, then Clang, then mingw
    if (can_exec("cl", verbosity)) {
      ci->path = "cl";
      ci->type = infos_t::MSVC;
    } else if (can_exec("clang --version", verbosity)) {
      ci->path = "clang";
      ci->type = infos_t::Clang;
    } else if (can_exec("gcc --version", verbosity)) {
      ci->path = "gcc";
      ci->type = compiler::infos_t::GCC;
    } else {
      goto lbl_error_compiler;
    }
#else
    // On Linux we start by GCC, then Clang
    if (can_exec("gcc --version", verbosity)) {
      ci->path = "gcc";
      ci->type = infos_t::GCC;
    } else if (can_exec("clang --version", verbosity)) {
      ci->path = "clang";
      ci->type = compiler::infos_t::Clang;
    } else {
      goto lbl_error_compiler;
    }
#endif
  } else if (name == "c++" || name == "cuda-host-c++") {
    ci->lang = compiler::infos_t::CPP;
    if (verbosity >= VERBOSITY_NORMAL) {
      OUTPUT << "Automatic C++ compiler detection" << std::endl;
    }
#ifdef NS2_IS_MSVC
    // On Windows we start by MSVC, then Clang, then mingw
    if (can_exec("cl", verbosity)) {
      ci->path = "cl";
      ci->type = infos_t::MSVC;
    } else if (can_exec("clang++ --version", verbosity)) {
      ci->path = "clang++";
      ci->type = infos_t::Clang;
    } else if (can_exec("g++ --version", verbosity)) {
      ci->path = "g++";
      ci->type = compiler::infos_t::GCC;
    } else {
      goto lbl_error_compiler;
    }
#else
    // On Linux we start by GCC, then Clang
    if (can_exec("g++ --version", verbosity)) {
      ci->path = "g++";
      ci->type = infos_t::GCC;
    } else if (can_exec("clang++ --version", verbosity)) {
      ci->path = "clang++";
      ci->type = compiler::infos_t::Clang;
    } else {
      goto lbl_error_compiler;
    }
#endif
  } else {
    get_path_type_and_lang(ci, name);
  }
  return;

lbl_error_compiler:
  NS2_THROW(std::runtime_error, "cannot find a viable compiler");
}

// ----------------------------------------------------------------------------

/* For later use
static int get_host_nbits() {
#ifdef _MSC_VER
  // Returns 32 or 64 depending on the machine where nsconfig is running.
  // We do not use sizeof(void*) or other compile time tricks as we can
  // imagine a 32-bits nsconfig running on a 64-bits OS in which case this
  // function has to return 64. On Windows this is easy: it suffices to look
  // into the PROCESSOR_ARCHITECTURE environment variable. But this environment
  // variable depends on the processus. For 32-bits processus running on a
  // 64-bits system (WOW64 process) this variable is not set to correspond
  // to the 64-bits system. Moreover using the Win32 API to determine system
  // informations via GetNativeSystemInfo or IsWow64Process does not work
  // properly on ARM. The only Win32 API that does work properly is
  // IsWow64Process2 but it is only available on Windows 10 and later. Another
  // way is to run the native cmd.exe and echo %PROCESSOR_ARCHITECTURE%.
  // But for WOW64 processes this environment variable is set to 32 even on
  // a 64 bits system. Moreover for WOW64 processes the system does file
  // system redirections and %WINDIR%\system32 is in fact %WINDIR%\SysWOW64
  // and one has to use %WINDIR%\Sysnative. But the latter is only available
  // for WOW64 processes. So the logic is the following:
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
*/

// ----------------------------------------------------------------------------

static std::pair<std::string, int> popen_src(compiler::infos_t const &ci,
                                             std::string const &dirname,
                                             std::string const &prefix,
                                             std::string const &src,
                                             int verbosity) {

  // Compute file extension and proper include file
  std::string extension, include_stdio;
  if (ci.type == compiler::infos_t::NVCC) {
    extension = ".cu";
    include_stdio = "cstdio";
  } else if (ci.lang == compiler::infos_t::C) {
    extension = ".c";
    include_stdio = "stdio.h";
  } else {
    extension = ".cpp";
    include_stdio = "cstdio";
  }

  // Dump code for computing version
  std::string src_filename(prefix + extension);
  std::string src_pathname(ns2::join_path(dirname, src_filename));
  if (verbosity >= VERBOSITY_DEBUG) {
    OUTPUT << "Content of '" << src_pathname << "':" << std::endl;
  }
  std::string file_content("#include <" + include_stdio + ">\nint main() {\n" +
                           src + "\nfflush(stdout); return 0; }");
  if (verbosity >= VERBOSITY_DEBUG) {
    output_multiline(file_content);
  }
  ns2::write_file(src_pathname, file_content);

  // Try and compile the code
  std::string aout_filename(prefix + extension + ".exe");
  std::string cmdline("cd \"" + ns2::sanitize(dirname) + "\" && ");
  if (ci.type == compiler::infos_t::MSVC) {
    cmdline += ci.path + " " + src_filename + " /Fe" + aout_filename + " 2>&1";
  } else {
    cmdline += ci.path + " " + src_filename + " -o " + aout_filename + " 2>&1";
  }
  if (verbosity >= VERBOSITY_DEBUG) {
    OUTPUT << "Compilation line: " << cmdline << std::endl;
  }
  std::pair<std::string, int> ret = ns2::popen(cmdline);
  if (verbosity >= VERBOSITY_DEBUG) {
    OUTPUT << "Output of compilation:" << std::endl;
    output_multiline(ret.first);
    OUTPUT << "Return code is " << ret.second << std::endl;
  }
  if (ret.second != 0) {
    return std::pair<std::string, int>(std::string(), 1);
  }

  // Execute binary and finally get output
  std::string aout_pathname(
      ns2::sanitize(ns2::join_path(dirname, aout_filename)));
  ret = ns2::popen(aout_pathname);
  if (verbosity >= VERBOSITY_DEBUG) {
    OUTPUT << "Output of '" << aout_pathname << "'" << std::endl;
    output_multiline(ret.first);
    OUTPUT << "Return code is " << ret.second << std::endl;
  }
  return std::pair<std::string, int>(ret.first, (ret.second ? 2 : 0));
}

// ----------------------------------------------------------------------------

static void set_version_arch(infos_t *ci_, parser::infos_t *pi_) {
  parser::infos_t &pi = *pi_;
  compiler::infos_t &ci = *ci_;
  int verbosity = pi.verbosity;

  // Getting the version of the compiler is easy. We compile and run a piece
  // of code that computes the version. Thanks to sjubertie for the idea. It
  // avoids the mess of parsing the compiler banner/version which compiler,
  // compiler version, system locale or not,.. dependant...
  std::string version_formula;
  switch (ci.type) {
  case compiler::infos_t::GCC:
    version_formula = "(long)(__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + "
                      "__GNUC_PATCHLEVEL__)";
    break;
  case compiler::infos_t::Clang:
  case compiler::infos_t::ARMClang:
    version_formula = "(long)(__clang_major__ * 10000 + __clang_minor__ * 100 "
                      "+ __clang_patchlevel__)";
    break;
  case compiler::infos_t::FCC_trad_mode:
  case compiler::infos_t::FCC_clang_mode:
    version_formula = "(long)(__FCC_major__ * 100 + __FCC_minor__ * 10 + "
                      "__FCC_patchlevel__)";
    break;
  case compiler::infos_t::Emscripten:
    version_formula = "(long)(__EMSCRIPTEN_major__ * 10000 + "
                      "__EMSCRIPTEN_minor__ * 100 + __EMSCRIPTEN_tiny__)";
    break;
  case compiler::infos_t::MSVC:
    version_formula = "(long)(_MSC_VER)";
    break;
  case compiler::infos_t::ICC:
    version_formula = "(long)(__INTEL_COMPILER)";
    break;
  case compiler::infos_t::NVCC:
    version_formula = "(long)(__CUDACC_VER_MAJOR__ * 10000 + "
                      "__CUDACC_VER_MINOR__ * 100 + "
                      "__CUDACC_VER_BUILD__)";
    break;
  case compiler::infos_t::HIPCC:
  case compiler::infos_t::HCC:
    version_formula = "(long)(__hcc_major__ * 10000 + __hcc_minor__ * 100)";
    break;
  case compiler::infos_t::DPCpp:
    version_formula = "(long)__INTEL_CLANG_COMPILER";
    break;
  case compiler::infos_t::None:
    NS2_THROW(std::runtime_error, "Invalid compiler");
    break;
  }

  // First of, create directory for all this mess
  ns2::mkdir(COMPILER_INFOS_DIR);

  // Get compiler version
  if (verbosity >= VERBOSITY_DEBUG) {
    OUTPUT << "Attempting to find compiler version" << std::endl;
  }
  std::pair<std::string, int> code =
      popen_src(ci, COMPILER_INFOS_DIR, "version",
                "printf(\"%li\", " + version_formula + ");", verbosity);
  if (code.second) {
    NS2_THROW(std::runtime_error,
              "Cannot find " + get_type_str(ci.type) + " version");
  }
  ci.version = atoi(code.first.c_str());

  // Get compiler target architecture
  if (verbosity >= VERBOSITY_DEBUG) {
    OUTPUT << "Attempting to find compiler target architecture" << std::endl;
  }
  switch(ci.type) {
  case infos_t::NVCC: {
    // For NVCC it depends on the host compiler and by default the
    // host compiler we choose for nvcc is cuda-host-c++ if nvcc is c++,
    // otherwise it is c++.
    compiler::infos_t host_ci;
    if (ci.name == "c++" ||
        pi.compilers.find("cuda-host-c++") != pi.compilers.end()) {
      host_ci = get("cuda-host-c++", &pi);
    } else {
      host_ci = get("c++", &pi);
    }
    ci.arch = host_ci.arch;
    ci.nbits = host_ci.nbits;
    break;
  }
  case infos_t::Clang:
  case infos_t::ARMClang:
  case infos_t::FCC_clang_mode:
  case infos_t::FCC_trad_mode:
  case infos_t::Emscripten:
  case infos_t::GCC:
  case infos_t::MSVC:
  case infos_t::HCC:
  case infos_t::ICC:
  case infos_t::HIPCC:
  case infos_t::DPCpp:
    code = popen_src(
        ci, COMPILER_INFOS_DIR, "target",
        "#if defined(_M_ARM_ARMV7VE) || defined(_M_ARM) || "
        "(__ARM_ARCH > 0 && __ARM_ARCH <= 7) || "
        "defined(__ARMEL__) || defined(__ARM_32BIT_STATE)\n"
        "#if _MSC_VER > 0 || (defined(__ARM_ARCH) && __ARM_ARCH <= 6)\n"
        "printf(\"armel\");\n"
        "#else\n"
        "printf(\"armhf\");\n"
        "#endif\n"
        "#elif defined(__arm64) || defined(_M_ARM64) || defined(__aarch64__) "
        "|| defined(__AARCH64EL__) || defined(__ARM_64BIT_STATE) || "
        "defined(__ARM_FEATURE_SVE)\n"
        "printf(\"aarch64\");\n"
        "#elif defined(_M_IX86) || defined(_X86_) || defined(__INTEL__) || "
        "defined(__I86__) || defined(__i386__) || defined(__i486__) ||"
        "defined(__i586__) || defined(__i686__)\n"
        "printf(\"x86\");\n"
        "#elif defined(__x86_64) || defined(__x86_64__) || defined(__amd64__) "
        "|| defined(__amd64) || defined(_M_X64)\n"
        "printf(\"x86_64\");\n"
        "#elif defined(__wasm32) || defined(__wasm32__)\n"
        "printf(\"wasm32\");\n"
        "#elif defined(__wasm64) || defined(__wasm64__)\n"
        "printf(\"wasm64\");\n"
        "#elif (defined(__PPC64__) || defined(_ARCH_PPC64)) && "
        "(defined(_LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN__))\n"
        "printf(\"ppc64el\");\n"
        "#endif", verbosity);
    if (code.second) {
      NS2_THROW(std::runtime_error, "Cannot find " + get_type_str(ci.type) +
                                        " target architecture");
    }
    get_archi_from_string(&ci, code.first);
    break;
  case infos_t::None:
    NS2_THROW(std::runtime_error, "Invalid compiler");
    break;
  }

  if (pi.verbosity >= VERBOSITY_NORMAL) {
    OUTPUT << "Compiler: " << ci << std::endl;
  }
  return;
}

// ----------------------------------------------------------------------------

infos_t get(std::string name, parser::infos_t *pi_) {
  parser::infos_t &pi = *pi_;
  infos_t ci;
  int verbosity = pi.verbosity;

  // do we already have encountered this compiler?
  list_t::iterator it = pi.compilers.find(name);
  if (it == pi.compilers.end()) {
    if (verbosity >= VERBOSITY_DEBUG) {
      OUTPUT << "Requesting new compiler: '" << name << "'" << std::endl;
    }
    ci.name = name;
    ci.fully_filled = false;
    automatic_detection(&ci, name, verbosity);
  } else {
    ci = it->second;
  }

  // if ci was not fully filled then fill it
  if (!ci.fully_filled) {
    if (verbosity >= VERBOSITY_DEBUG) {
      OUTPUT << "Compiler: '" << name << "' is not fully known" << std::endl;
    }
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
  case compiler::infos_t::FCC_trad_mode:
    return "fcc_trad_mode";
  case compiler::infos_t::FCC_clang_mode:
    return "fcc_clang_mode";
  case compiler::infos_t::Emscripten:
    return "emscripten";
  case compiler::infos_t::MSVC:
    return "msvc";
  case compiler::infos_t::ICC:
    return "icc";
  case compiler::infos_t::NVCC:
    return "nvcc";
  case compiler::infos_t::HIPCC:
    return "hipcc";
  case compiler::infos_t::HCC:
    return "hcc";
  case compiler::infos_t::DPCpp:
    return "dpcpp";
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
