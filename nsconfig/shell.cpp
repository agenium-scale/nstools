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

#include "shell.hpp"
#include "find_exe_lib_header.hpp"
#include "parser.hpp"
#include <algorithm>
#include <cassert>
#include <ns2/fs.hpp>
#include <ns2/string.hpp>
#include <string>
#include <vector>

namespace shell {

// ----------------------------------------------------------------------------

static std::vector<std::string> fcc(std::string const &,
                                    std::vector<parser::token_t> const &,
                                    compiler::infos_t const &,
                                    parser::infos_t const &,
                                    parser::infos_t::action_t);

static std::vector<std::string>
emscripten(std::string const &, std::vector<parser::token_t> const &,
           compiler::infos_t const &, parser::infos_t const &,
           parser::infos_t::action_t);

static std::vector<std::string> gcc_clang(std::string const &,
                                          std::vector<parser::token_t> const &,
                                          compiler::infos_t const &,
                                          parser::infos_t const &,
                                          parser::infos_t::action_t);

static std::vector<std::string>
hipcc_hcc_dpcpp(std::string const &, std::vector<parser::token_t> const &,
                compiler::infos_t const &, parser::infos_t const &,
                parser::infos_t::action_t);

static std::vector<std::string> icc(std::string const &,
                                    std::vector<parser::token_t> const &,
                                    compiler::infos_t const &,
                                    parser::infos_t const &,
                                    parser::infos_t::action_t);

static std::vector<std::string> xlc(std::string const &,
                                    std::vector<parser::token_t> const &,
                                    compiler::infos_t const &,
                                    parser::infos_t const &,
                                    parser::infos_t::action_t);

static std::vector<std::string> msvc(std::vector<parser::token_t> const &,
                                     compiler::infos_t const &,
                                     parser::infos_t const &,
                                     parser::infos_t::action_t);

static std::vector<std::string>
nvcc(compiler::infos_t const &, std::vector<parser::token_t> const &,
     compiler::infos_t const &, parser::infos_t *, parser::infos_t::action_t);

// ----------------------------------------------------------------------------

std::string stringify(std::string const &str) {
  if (str.find(' ') != std::string::npos ||
      str.find('\t') != std::string::npos) {
    return "\"" + str + "\"";
  } else {
    return str;
  }
}

// ----------------------------------------------------------------------------

std::string ify(std::string const &str) {
  return stringify(ns2::sanitize(str));
}

// ----------------------------------------------------------------------------

std::vector<std::string> ify(std::vector<std::string> const &vec) {
  std::vector<std::string> ret(vec.size());
  for (size_t i = 0; i < vec.size(); i++) {
    ret[i] = stringify(ns2::sanitize(vec[i]));
  }
  return ret;
}

// ----------------------------------------------------------------------------

static std::vector<std::string> uniq(std::vector<std::string> const &vec) {
  std::vector<std::string> ret;
  for (size_t i = 0; i < vec.size(); i++) {
    if (std::find(ret.begin(), ret.end(), vec[i]) == ret.end()) {
      ret.push_back(vec[i]);
    }
  }
  return ret;
}

// ----------------------------------------------------------------------------

static std::string raw(std::vector<parser::token_t> const &tokens) {
  std::string ret(ns2::sanitize(stringify(tokens[0].text)));
  for (size_t i = 1; i < tokens.size(); i++) {
    ret += " " + ns2::sanitize(stringify(tokens[i].text));
  }
  return ret;
}

// ----------------------------------------------------------------------------

static std::string append_filenames(std::vector<parser::token_t> const &tokens,
                                    size_t i0) {
  std::string ret;
  for (size_t i = i0; i < tokens.size(); i++) {
    ret += " " + stringify(ns2::sanitize(tokens[i].text));
  }
  return ret;
}

// ----------------------------------------------------------------------------

static std::string touch(std::vector<parser::token_t> const &tokens) {
  if (tokens.size() != 2) {
    die("touch must have only one argument", tokens[0].cursor);
  }
#ifdef NS2_IS_MSVC
  return "echo >" + stringify(ns2::sanitize(tokens[1].text));
#else
  return "touch " + stringify(tokens[1].text);
#endif
}

// ----------------------------------------------------------------------------

static std::string cd(std::vector<parser::token_t> const &tokens) {
  if (tokens.size() != 2) {
    die("cd must have only one directory", tokens[0].cursor);
  }
  return "cd" + stringify(ns2::sanitize(tokens[1].text));
}

// ----------------------------------------------------------------------------

static std::string rm(std::vector<parser::token_t> const &tokens) {
  if (tokens.size() == 1) {
    die("expected at least one file to delete", tokens[0].cursor);
  }
  std::string ret;
  if (tokens[1].text == "-r") {
#ifdef NS2_IS_MSVC
    ret = "rd /S /Q";
#else
    ret = "rm -rf";
#endif
    return ret + append_filenames(tokens, 2);
  } else {
#ifdef NS2_IS_MSVC
    ret = "del /F /Q";
#else
    ret = "rm -f";
#endif
    return ret + append_filenames(tokens, 1);
  }
}

// ----------------------------------------------------------------------------

std::string rm(bool rf, std::string const &filename) {
#ifdef NS2_IS_MSVC
  if (rf) {
    return "del /S /Q " + stringify(ns2::sanitize(filename));
  } else {
    return "del /F /Q " + stringify(ns2::sanitize(filename));
  }
#else
  if (rf) {
    return "rm -rf " + stringify(filename);
  } else {
    return "rm -f " + stringify(filename);
  }
#endif
}

// ----------------------------------------------------------------------------
// xcopy does not behave like cp when copying one directory into another

#ifdef NS2_IS_MSVC
static std::string xcopy_helper(std::string const &from,
                                std::string const &to) {
  size_t slash_ = from.rfind('/');
  size_t backslash_ = from.rfind('\\');
  int slash = (slash_ == std::string::npos ? -1 : int(slash_));
  int backslash = (backslash_ == std::string::npos ? -1 : int(backslash_));
  int i0 = (std::max)(slash, backslash);
  if (i0 == -1) {
    return stringify(from) + " " + stringify(to + "\\" + from);
  } else {
    return stringify(from) + " " + stringify(to + "\\" +
               from.substr(size_t(i0 + 1)));
  }
}
#endif

// ----------------------------------------------------------------------------

static std::string cp(std::vector<parser::token_t> const &tokens) {
  std::string ret;
  if (tokens[1].text == "-r") {
    if (tokens.size() != 4) {
      die("expected exactly one source and one destination", tokens[0].cursor);
    }
#ifdef NS2_IS_MSVC
    return "xcopy " + xcopy_helper(tokens[2].text, tokens[3].text) +
           " /E /C /I /Q /G /H /R /Y";
#else
    return "cp -rf" + append_filenames(tokens, 2);
#endif
  } else {
    if (tokens.size() != 3) {
      die("expected exactly one source and one destination", tokens[0].cursor);
    }
#ifdef NS2_IS_MSVC
    return "xcopy " + append_filenames(tokens, 1) + " /C /I /Q /G /H /Y";
#else
    return "cp -f" + append_filenames(tokens, 1);
#endif
  }
}

// ----------------------------------------------------------------------------

std::string cp(bool r, std::string const &from, std::string const &to) {
#ifdef NS2_IS_MSVC
  if (r) {
    return "xcopy " + xcopy_helper(from, to) + " /E /C /I /Q /G /H /R /Y";
  } else {
    return "xcopy " + stringify(ns2::sanitize(from)) + " " +
           stringify(ns2::sanitize(to)) + " /C /I /Q /G /H /Y";
  }
#else
  if (r) {
    return "cp -rf " + stringify(from) + " " + stringify(to);
  } else {
    return "cp -f " + stringify(from) + " " + stringify(to);
  }
#endif
}

// ----------------------------------------------------------------------------

static std::string mkdir(std::vector<parser::token_t> const &tokens) {
  if (tokens.size() == 1) {
    die("expected one folder to create after", tokens[0].cursor);
  } else if (tokens.size() > 2) {
    die("can only deal with one folder at a time", tokens[2].cursor);
  }
  std::string folder(shell::ify(tokens[1].text));
#ifdef NS2_IS_MSVC
  return "if not exist " + folder + " md " + folder;
#else
  return "mkdir -p " + folder;
#endif
}

// ----------------------------------------------------------------------------

std::string mkdir_p(std::string const &path) {
#ifdef NS2_IS_MSVC
  return "if not exist " + stringify(path) + " md " + stringify(path);
#else
  return "mkdir -p " + stringify(path);
#endif
}

// ----------------------------------------------------------------------------

static std::string cat(std::vector<parser::token_t> const &tokens) {
  if (tokens.size() == 1) {
    die("expected at least one file to dump", tokens[0].cursor);
  }
#ifdef NS2_IS_MSVC
  std::string ret("type");
#else
  std::string ret("cat");
#endif
  return ret + append_filenames(tokens, 1);
}

// ----------------------------------------------------------------------------

static std::string echo(std::vector<parser::token_t> const &tokens) {
#ifdef NS2_IS_MSVC
  if (tokens.size() == 1 || tokens[1].text == ">" || tokens[1].text == ">>") {
    return std::string("echo.");
  }
#endif
  return "echo" + append_filenames(tokens, 1);
}

// ----------------------------------------------------------------------------

static std::string mv(std::vector<parser::token_t> const &tokens) {
  if (tokens.size() != 3) {
    die("mv must have only two arguments", tokens[0].cursor);
  }
#ifdef NS2_IS_MSVC
  std::string ret("move /Y");
#else
  std::string ret("mv");
#endif
  return ret + append_filenames(tokens, 1);
}

// ----------------------------------------------------------------------------

static std::string ar(std::vector<parser::token_t> const &tokens) {
  if (tokens.size() < 4) {
    die("ar must have at least two arguments", tokens[0].cursor);
  }
  if (tokens[1].text != "rcs") {
    die("only accepted argument is 'rcs'", tokens[1].cursor);
  }
#ifdef NS2_IS_MSVC
  return "lib /nologo /out:" + tokens[2].text + append_filenames(tokens, 3);
#else
  return "ar rcs" + append_filenames(tokens, 2);
#endif
}

// ----------------------------------------------------------------------------

static size_t
find_corresponding_paren(std::vector<parser::token_t> const &tokens,
                         size_t i0) {
  int paren = 0;
  size_t i = i0;
  for (; i < tokens.size(); i++) {
    if (tokens[i].text == "(") {
      paren++;
    }
    if (tokens[i].text == ")") {
      paren--;
      if (paren == 0) {
        break;
      }
    }
  }
  return i;
}

// ----------------------------------------------------------------------------

static std::string if_(std::vector<parser::token_t> const &tokens,
                       parser::infos_t::action_t action,
                       parser::infos_t *pi_, autodeps_t *autodeps_) {
  parser::infos_t &pi = *pi_;
  switch (tokens.size()) {
  case 2:
    die("expected comparison operator after", tokens[1].cursor);
    break;
  case 3:
    if (tokens[2].text != "==" && tokens[2].text != "!=") {
      die("expected valid comparison operator '==' or '!='", tokens[2].cursor);
    }
    die("expected second operand for comparison after", tokens[2].cursor);
    break;
  case 4:
    die("expected '(' after", tokens[3].cursor);
    break;
  }
  if (tokens[4].text != "(") {
    die("expected '(' here", tokens[4].cursor);
  }

  std::string ret;
  size_t i;

#ifdef NS2_IS_MSVC
  if (tokens[2].text == "==") {
    ret = "if ";
  } else {
    ret = "if not ";
  }
  ret += "\"" + tokens[1].text + "\" == \"" + tokens[3].text + "\" ( ";
#else
  ret = "if [ \"" + tokens[1].text + "\" ";
  ret += (tokens[2].text == "==" ? "=" : "!=");
  ret += " \"" + tokens[3].text + "\" ]; then ";
#endif

  // extract the 'then' part
  i = find_corresponding_paren(tokens, 4);
  if (i >= tokens.size()) {
    die("cannot find corresponding ')'", tokens[4].cursor);
  }

  // parse the then part
  ret += translate(std::vector<parser::token_t>(tokens.begin() + 5,
                                                tokens.begin() + long(i)),
                   action, &pi, autodeps_);

  // is there an 'else' part or not?
  if (i >= tokens.size() - 1) {
#ifdef NS2_IS_MSVC
    ret += " )";
#else
    ret += " ; fi";
#endif
    return ret;
  } else {
#ifdef NS2_IS_MSVC
    ret += " ) else ( ";
#else
    ret += " ; else ";
#endif
  }

  // we have an else part, we expect 'else' here
  i++;
  if (tokens[i].text != "else") {
    die("expected 'else' keyword here", tokens[i].cursor);
  }
  i++;
  if (i >= tokens.size()) {
    die("expected '(' after", tokens[i - 1].cursor);
  } else if (tokens[i].text != "(") {
    die("expected '(' here", tokens[i].cursor);
  }
  size_t i0_else = i + 1;

  // find corresponding ')'
  i = find_corresponding_paren(tokens, i);
  if (i >= tokens.size()) {
    die("cannot find corresponding ')'", tokens[4].cursor);
  }

  // parse the else part
  ret += translate(std::vector<parser::token_t>(tokens.begin() + long(i0_else),
                                                tokens.begin() + long(i)),
                   action, &pi, autodeps_);

#ifdef NS2_IS_MSVC
  ret += " )";
#else
  ret += " ; fi";
#endif

  return ret;
}

// ----------------------------------------------------------------------------

static std::vector<std::string>
comp(compiler::infos_t const &ci, std::vector<parser::token_t> const &tokens,
     parser::infos_t *pi_, parser::infos_t::action_t action,
     autodeps_t *autodeps_) {
  parser::infos_t &pi = *pi_;
  std::vector<std::string> ret;
  switch (ci.type) {
  case compiler::infos_t::GCC:
  case compiler::infos_t::Clang:
  case compiler::infos_t::ARMClang:
    ret = gcc_clang(ci.path, tokens, ci, pi, action);
    break;
  case compiler::infos_t::FCC_trad_mode:
  case compiler::infos_t::FCC_clang_mode:
    ret = fcc(ci.path, tokens, ci, pi, action);
    break;
  case compiler::infos_t::Xlc:
    ret = xlc(ci.path, tokens, ci, pi, action);
    break;
  case compiler::infos_t::Emscripten:
    ret = emscripten(ci.path, tokens, ci, pi, action);
    break;
  case compiler::infos_t::MSVC:
    ret = msvc(tokens, ci, pi, action);
    break;
  case compiler::infos_t::ICC:
    ret = icc(ci.path, tokens, ci, pi, action);
    break;
  case compiler::infos_t::NVCC: {
    compiler::infos_t host_ci;
    if (ci.name == "c++" ||
        pi.compilers.find("cuda-host-c++") != pi.compilers.end()) {
      host_ci = compiler::get("cuda-host-c++", &pi);
    } else {
      host_ci = compiler::get("c++", &pi);
    }
    ret = nvcc(ci, tokens, host_ci, &pi, action);
    break;
  }
  case compiler::infos_t::HIPCC:
  case compiler::infos_t::HCC:
  case compiler::infos_t::DPCpp:
    ret = hipcc_hcc_dpcpp(ci.path, tokens, ci, pi, action);
    break;
  case compiler::infos_t::None:
    NS2_THROW(std::runtime_error, "Invalid compiler");
  }

  // There are basically only two  ways compilers handle autodeps. There is
  // MSVC on one side that needs the "/showIncludes" switch that cannot be
  // placed anywhere in the command-line. On the other side there all other
  // compilers we have encounter so far that do as GCC does. So we handle the
  // autodeps stuff here.
  if (autodeps_ != NULL) {
    if (ci.type == compiler::infos_t::MSVC) {
      autodeps_->by = compiler::infos_t::MSVC;
      autodeps_->cmd = ns2::join(ret.begin(), ret.begin() + 4, " ") +
                       " /showIncludes " +
                       ns2::join(ret.begin() + 4, ret.end(), " ");
    } else {
      autodeps_->by = compiler::infos_t::GCC;
      autodeps_->cmd =
          ns2::join(ret, " ") + " -MMD -MF " + shell::ify(autodeps_->file);
    }
  }
  return ret;
}

// ----------------------------------------------------------------------------
// On BSDs (and therefore on MacOS) the syntax for rpath is of course different
// from Linux. More precisely GNU ld (found on most Linux) uses "-rpath=dir"
// while BSD ld uses "-path dir".

static std::string get_rpath_argument(std::string const &directory,
                                      compiler::infos_t const &ci) {
  switch (ci.type) {
  case compiler::infos_t::None:
    NS2_THROW(std::runtime_error, "Invalid compiler");
  case compiler::infos_t::GCC:
  case compiler::infos_t::Clang:
  case compiler::infos_t::ARMClang:
  case compiler::infos_t::FCC_trad_mode:
  case compiler::infos_t::FCC_clang_mode:
  case compiler::infos_t::Xlc:
  case compiler::infos_t::ICC:
  case compiler::infos_t::HIPCC:
  case compiler::infos_t::NVCC:
  case compiler::infos_t::HCC:
  case compiler::infos_t::DPCpp:
#if defined(NS2_IS_BSD) || defined(NS2_IS_MACOS)
    if (directory == ".") {
      return "-rpath,$ORIGIN";
    } else if (directory[0] == '.' && directory[1] == '/') {
      return "-rpath,$ORIGIN" + directory.substr(2);
    } else {
      return "-rpath," + directory;
    }
#else
    if (directory == ".") {
      return "-rpath=$ORIGIN";
    } else if (directory[0] == '.' && directory[1] == '/') {
      return "-rpath=$ORIGIN" + directory.substr(2);
    } else {
      return "-rpath=" + directory;
    }
#endif
  case compiler::infos_t::MSVC:
  case compiler::infos_t::Emscripten:
    return std::string();
  }
  return std::string();
}

// ----------------------------------------------------------------------------

static std::vector<std::string>
translate_single_arg(std::string const &compiler,
                     std::map<std::string, std::string> const &args,
                     compiler::infos_t const &ci, parser::token_t const &token,
                     parser::infos_t const &pi,
                     parser::infos_t::action_t action) {
  std::vector<std::string> ret;
  std::string const &arg = token.text;
  if (arg == "-lpthread" || arg == "-lm") {
    ret.push_back(arg);
    return ret;
  } else if (arg == "-L.") {
    ret.push_back("-L.");
    std::string rpath_arg(get_rpath_argument(".", ci));
    if (rpath_arg.size() > 0) {
      ret.push_back("'-Wl," + rpath_arg + "'");
    }
    return ret;
  }
  if (arg[0] == '-') {
    if (arg[1] == 'l' && arg[2] == ':') {
      if (arg.size() == 3) {
        die("no file/directory given here", token.cursor);
      }
      ret.push_back("-l:" + shell::ify(&arg[3]));
    } else if (arg[1] == 'I' || arg[1] == 'L' || arg[1] == 'l') {
      if (arg.size() == 2) {
        die("no file/directory given here", token.cursor);
      }
      if (arg[1] == 'l') {
        ret.push_back("-l" + shell::ify(lib_basename(&arg[2])));
      } else if (arg[1] == 'L') {
        std::string path(shell::ify(&arg[2]));
        ret.push_back("-L" + path);
        std::string rpath_arg(get_rpath_argument(path, ci));
        if (rpath_arg.size() > 0) {
          ret.push_back("'-Wl," + rpath_arg + "'");
        }
      } else {
        ret.push_back(std::string("-") + arg[1] + shell::ify(&arg[2]));
      }
    } else if (arg[1] == 'D') {
      if (arg.size() == 2) {
        die("no macro name given here", token.cursor);
      }
      ret.push_back(arg);
    } else {
      std::map<std::string, std::string>::const_iterator it = args.find(arg);
      if (it != args.end()) {
        if (it->second.size() > 0) {
          ret.push_back(it->second);
        } else if (pi.verbosity >= VERBOSITY_DEBUG) {
          WARNING << "Option " << arg << " is not supported or known by "
                  << compiler << ", ignoring it" << std::endl;
        }
      } else if (action == parser::infos_t::Permissive) {
        ret.push_back(arg);
      } else {
        die("unknown compiler option", token.cursor);
      }
    }
  } else {
    ret.push_back(stringify(ns2::sanitize(arg)));
  }
  return ret;
}

// ----------------------------------------------------------------------------

static std::vector<std::string>
gcc_clang(std::string const &compiler,
          std::vector<parser::token_t> const &tokens,
          compiler::infos_t const &ci, parser::infos_t const &pi,
          parser::infos_t::action_t action) {
  std::vector<std::string> ret;
  ret.push_back(compiler);
  std::map<std::string, std::string> args;
  args["-std=c89"] = "-std=c89 -pedantic";
  args["-std=c99"] = "-std=c99 -pedantic";
  args["-std=c11"] = "-std=c11 -pedantic";
  args["-std=c++98"] = "-std=c++98 -pedantic";
  args["-std=c++03"] = "-std=c++03 -pedantic";
  if (ci.type == compiler::infos_t::GCC && ci.version < 40801) {
    args["-std=c++11"] = "-std=c++0x";
  } else {
    args["-std=c++11"] = "-std=c++11 -pedantic";
  }
  if (ci.type == compiler::infos_t::GCC && ci.version < 50000) {
    args["-std=c++14"] = "-std=c++1y";
  } else {
    args["-std=c++14"] = "-std=c++14 -pedantic";
  }
  args["-std=c++17"] = "-std=c++17 -pedantic";
  args["-std=c++20"] = "-std=c++20 -pedantic";
  args["-O0"] = "-O0";
  args["-O1"] = "-O1";
  args["-O2"] = "-O2";
  args["-O3"] = "-O3";
  args["-ffast-math"] = "-ffast-math";
  args["-g"] = "-g";
  args["-S"] = "-S";
  args["-c"] = "-c";
  args["-x"] = "-x";
  args["-o"] = "-o";
  if (ci.type == compiler::infos_t::GCC && ci.version < 40500) {
    args["-Wall"] = "-Wall -Wextra -Wconversion -Wsign-conversion";
  } else {
    args["-Wall"] =
        "-Wall -Wextra -Wdouble-promotion -Wconversion -Wsign-conversion";
  }
  args["-fPIC"] = "-fPIC";
  args["-static-libstdc++"] = "-static-libstdc++ -static-libgcc";
  args["-msse"] = "-msse";
  args["-msse2"] = "-msse2";
  args["-msse3"] = "-msse3";
  args["-mssse3"] = "-mssse3";
  args["-msse41"] = "-msse4.1";
  args["-msse42"] = "-msse4.2";
  if (ci.type == compiler::infos_t::GCC) {
    // GCC translates 256-bits loads/stores into two 128-bits loads/stores
    // because on some old CPUs it is faster. We assume that these CPUs are
    // no longer in use so we tell GCC to translate 256-bits loads/stores into
    // 256-bits loads/stores.
    // Cf. https://stackoverflow.com/questions/52626726/
    //       why-doesnt-gcc-resolve-mm256-loadu-pd-as-single-vmovupd
    args["-mavx"] = "-mavx -mno-avx256-split-unaligned-load"
                    " -mno-avx256-split-unaligned-store";
    args["-mavx2"] = "-mavx2 -mno-avx256-split-unaligned-load"
                     " -mno-avx256-split-unaligned-store";
  } else {
    args["-mavx"] = "-mavx";
    args["-mavx2"] = "-mavx2";
  }
  args["-mavx512_knl"] = "-mavx512f -mavx512pf -mavx512er -mavx512cd";
  args["-mavx512_skylake"] =
      "-mavx512f -mavx512dq -mavx512cd -mavx512bw -mavx512vl";
  if (ci.arch == compiler::infos_t::ARMEL) {
    args["-mneon64"] = "-mfloat-abi=softfp -mfpu=neon";
    args["-mneon128"] = "-mfloat-abi=softfp -mfpu=neon";
  } else {
    args["-mneon64"] = "-mfpu=neon";
    args["-mneon128"] = "-mfpu=neon";
  }
  args["-maarch64"] = "";
  args["-msve"] = "-march=armv8.2-a+sve";
  args["-msve128"] = "-march=armv8.2-a+sve -msve-vector-bits=128";
  args["-msve256"] = "-march=armv8.2-a+sve -msve-vector-bits=256";
  args["-msve512"] = "-march=armv8.2-a+sve -msve-vector-bits=512";
  args["-msve1024"] = "-march=armv8.2-a+sve -msve-vector-bits=1024";
  args["-msve2048"] = "-march=armv8.2-a+sve -msve-vector-bits=2048";
  args["-mvmx"] = "-mcpu=powerpc64le -maltivec";
  args["-mvsx"] = "-mcpu=powerpc64le -mvsx";
  if (ci.type == compiler::infos_t::GCC) {
    args["-msm_35"] = "";
    args["-msm_50"] = "";
    args["-msm_53"] = "";
    args["-msm_60"] = "";
    args["-msm_61"] = "";
    args["-msm_62"] = "";
    args["-msm_70"] = "";
    args["-msm_72"] = "";
    args["-msm_75"] = "";
    args["-mwasm_simd128"] = "";
  } else {
    args["-msm_35"] = "--cuda-gpu-arch=sm_35";
    args["-msm_50"] = "--cuda-gpu-arch=sm_50";
    args["-msm_53"] = "--cuda-gpu-arch=sm_53";
    args["-msm_60"] = "--cuda-gpu-arch=sm_60";
    args["-msm_61"] = "--cuda-gpu-arch=sm_61";
    args["-msm_62"] = "--cuda-gpu-arch=sm_62";
    args["-msm_70"] = "--cuda-gpu-arch=sm_70";
    args["-msm_72"] = "--cuda-gpu-arch=sm_72";
    args["-msm_75"] = "--cuda-gpu-arch=sm_75";
    args["-mwasm_simd128"] = "-msimd128";
  }

  if (ci.arch == compiler::infos_t::Intel) {
    args["-mfma"] = "-mfma";
    args["-mfp16"] = "-mf16c";
  } else if (ci.arch == compiler::infos_t::ARMEL) {
    args["-mfma"] = "";
    args["-mfp16"] = "";
  } else if (ci.arch == compiler::infos_t::ARMHF) {
    args["-mfma"] = "";
    args["-mfp16"] = "";
  } else if (ci.arch == compiler::infos_t::AARCH64) {
    args["-mfma"] = "";
    args["-mfp16"] = "-mfp16-format=ieee -march=armv8.2-a+fp16";
  } else if (ci.arch == compiler::infos_t::PPC64EL) {
    args["-mfma"] = "";
    args["-mfp16"] = "";
  }
  args["-fopenmp"] = "-fopenmp";
  args["-shared"] = "-shared";
  args["-fdiagnostics-color=always"] = "-fdiagnostics-color=always";

  if (ci.type == compiler::infos_t::GCC) {
    args["-fno-omit-frame-pointer"] = "-fno-omit-frame-pointer";
  } else {
    /* https://stackoverflow.com/questions/43864882
        /fno-omit-frame-pointer-equivalent-compiler-option-for-clang */
    args["-fno-omit-frame-pointer"] = "-fno-omit-frame-pointer "
                                      "-mno-omit-leaf-frame-pointer";
  }

  if (ci.type == compiler::infos_t::GCC) {
    if (ci.version <= 40900) {
      args["-vec-report"] = "-ftree-vectorizer-verbose=7";
    } else {
      args["-vec-report"] = "-fopt-info-vec-all";
    }
  } else {
    args["-vec-report"] = "-Rpass=loop-vectorize "
                          "-Rpass-missed=loop-vectorize "
                          "-Rpass-analysis=loop-vectorize";
  }

  for (size_t i = 1; i < tokens.size(); i++) {
    std::string const &arg = tokens[i].text;
    // The effect of --version on the command line is that the compiler
    // performs no action but displaying some infos, so no need to pass
    // other flags that will be ignored
    if (arg == "--version") {
      ret.clear();
      ret.push_back(compiler);
      ret.push_back("--version");
      return ret;
    }

    // For GCC and Clang there is no need to link against m (math library)
    // when compiling in C++. It is still needed when compiling C. Worse! it
    // makes clang++ emit a warning of non used command line parameter.
    if (arg == "-lm") {
      if (ci.lang == compiler::infos_t::C) {
        ret.push_back("-lm");
      }
      continue;
    }

    std::vector<std::string> buf =
        translate_single_arg(compiler, args, ci, tokens[i], pi, action);
    ret.insert(ret.end(), buf.begin(), buf.end());
  }

  return uniq(ret);
}

// ----------------------------------------------------------------------------

static std::vector<std::string>
msvc(std::vector<parser::token_t> const &tokens, compiler::infos_t const &ci,
     parser::infos_t const &pi, parser::infos_t::action_t action) {
  enum stop_stage_t { Assemble, Compile, CompileLink };
  stop_stage_t stop_stage = CompileLink;
  bool debug_infos = false;
  bool ffast_math = false;
  std::string output;
  std::vector<std::string> ret;
  ret.push_back("cl");
  ret.push_back("/nologo");
  ret.push_back("/EHsc");
  ret.push_back("/D_CRT_SECURE_NO_WARNINGS");

  std::map<std::string, std::string> args;
  args["-std=c89"] = "";
  args["-std=c99"] = "";
  args["-std=c11"] = "";
  args["-std=c++98"] = "";
  args["-std=c++03"] = "";
  args["-std=c++11"] = "";
  if (ci.version >= 1911) {
    args["-std=c++14"] = "/std:c++14";
    args["-std=c++17"] = "/std:c++17";
    args["-std=c++20"] = "/std:c++latest";
  } else if (ci.version >= 1900) {
    args["-std=c++14"] = "/std:c++14";
    args["-std=c++17"] = "/std:c++latest";
    args["-std=c++20"] = "/std:c++latest";
  } else {
    args["-std=c++14"] = "";
    args["-std=c++17"] = "";
    args["-std=c++20"] = "";
  }
  if (ci.version > 1900) { // MSVC 2017
    args["-std=c++98"] += " /Zc:__cplusplus";
    args["-std=c++03"] += " /Zc:__cplusplus";
    args["-std=c++11"] += " /Zc:__cplusplus";
    args["-std=c++14"] += " /Zc:__cplusplus";
    args["-std=c++17"] += " /Zc:__cplusplus";
    args["-std=c++20"] += " /Zc:__cplusplus";
  }
  args["-O0"] = "/Od";
  args["-O1"] = "/O2";
  args["-O2"] = "/Ox";
  args["-O3"] = "/Ox";
  args["-g"] = "/Zi";
  args["-S"] = "/FA";
  args["-c"] = "/c";
  args["-o"] = ""; // dealt with later because it's complicated for MSVC
  args["-Wall"] = "/W3";
  args["-fPIC"] = "";
  args["-static-libstdc++"] = "/MT";
  if (ci.nbits == 32) {
    args["-msse"] = "/arch:SSE";
    args["-msse2"] = "/arch:SSE2";
  } else {
    args["-msse"] = "";
    args["-msse2"] = "";
  }
  args["-msse3"] = "";
  args["-mssse3"] = "";
  args["-msse3"] = "";
  args["-mssse3"] = "";
  args["-msse41"] = "";
  args["-msse42"] = "";
  args["-mavx"] = "/arch:AVX";
  args["-mavx2"] = "/arch:AVX2";
  args["-mavx512_knl"] = "/arch:AVX512";
  args["-mavx512_skylake"] = "/arch:AVX512";
  if (ci.arch == compiler::infos_t::ARMEL) {
    args["-mfma"] = "/arch:VFPv4";
  } else {
    args["-mfma"] = "";
  }
  args["-mneon64"] = "";
  args["-mneon128"] = "";
  args["-maarch64"] = "";
  args["-msve"] = "";
  args["-msve128"] = "";
  args["-msve256"] = "";
  args["-msve512"] = "";
  args["-msve1024"] = "";
  args["-msve2048"] = "";
  args["-mfp16"] = "";
  args["-fopenmp"] = "/openmp";
  args["-shared"] = "/LD";
  args["-fdiagnostics-color=always"] = "";
  args["-mvmx"] = "";
  args["-mvsx"] = "";
  args["-mwasm_simd128"] = "";
  args["-msm_35"] = "";
  args["-msm_50"] = "";
  args["-msm_53"] = "";
  args["-msm_60"] = "";
  args["-msm_61"] = "";
  args["-msm_62"] = "";
  args["-msm_70"] = "";
  args["-msm_72"] = "";
  args["-msm_75"] = "";
  if (ci.version >= 1800) { // Visual Studio 2013
    args["-vec-report"] = "/Qvec-report:2";
  } else {
    args["-vec-report"] = "";
  }

  /* Omit frame pointer only available in 32-bits mode:
     https://docs.microsoft.com/en-us/cpp/build/reference
         /oy-frame-pointer-omission?view=msvc-160 */
  if (ci.nbits == 32) {
    args["-fno-omit-frame-pointer"] = "/Oy-";
  } else {
    args["-fno-omit-frame-pointer"] = "";
  }

  std::vector<std::string> linker_args;
  for (size_t i = 1; i < tokens.size(); i++) {
    std::string const &arg = tokens[i].text;
    // The equivalent of --version in MSVC is to call the compiler with
    // no arguments
    if (arg == "--version") {
      ret.clear();
      ret.push_back("cl");
      return ret;
    } else if (arg == "-lpthread") {
      ret.push_back("/MT");
      continue;
    } else if (arg == "-lm") {
      continue;
    } else if (arg == "-L.") {
      continue;
    } else if (arg == "-ffast-math") {
      ffast_math = true;
      continue;
    }
    if (arg[0] == '-') {
      if ((arg[1] == 'I' || arg[1] == 'l' || arg[1] == 'L') &&
          arg.size() == 2) {
        parser::die("no file/directory given here", tokens[i].cursor);
      }
      if (arg[1] == 'D' && arg.size() == 2) {
        parser::die("no macro name given here", tokens[i].cursor);
      }
      if (arg[1] == 'l' && arg[2] == ':' && arg.size() == 3) {
        parser::die("no file given here", tokens[i].cursor);
      }
      if (arg[1] == 'I') {
        ret.push_back("/I" + shell::ify(arg.c_str() + 2));
      } else if (arg[1] == 'D') {
        ret.push_back("/D" + stringify(arg.c_str() + 2));
      } else if (arg[1] == 'L') {
        linker_args.push_back("/LIBPATH:" + shell::ify(arg.c_str() + 2));
      } else if (arg[1] == 'l' && arg[2] == ':') {
        ret.push_back(shell::ify(&arg[3]));
      } else if (arg[1] == 'l') {
        ret.push_back(
            shell::ify("lib" + std::string(arg.c_str() + 2) + ".lib"));
      } else if (arg == "-o") {
        if (i == tokens.size() - 1) {
          parser::die("no filename given after -o", tokens[i].text);
        }
        output = tokens[i + 1].text;
        i++;
      } else if (arg == "-x") {
        if (i == tokens.size() - 1) {
          die("no language given after", tokens[i].cursor);
        }
        i++;
        if (tokens[i].text == "c") {
          ret.push_back("/TC");
        } else if (tokens[i].text == "c++") {
          ret.push_back("/TP");
        }
      } else {
        std::map<std::string, std::string>::const_iterator it = args.find(arg);
        if (it != args.end()) {
          if (arg == "-c") {
            stop_stage = Compile;
          } else if (arg == "-S") {
            stop_stage = Assemble;
          } else if (arg == "-g") {
            debug_infos = true;
          }
          if (it->second.size() > 0) {
            ret.push_back(it->second);
          } else if (pi.verbosity >= VERBOSITY_DEBUG) {
            WARNING << "Option " << arg << " is not supported or known by msvc"
                    << ", ignoring it" << std::endl;
          }
        } else if (action == parser::infos_t::Permissive) {
          ret.push_back(arg);
        } else {
          parser::die("unknown compiler option", tokens[i].cursor);
        }
      }
    } else {
      ret.push_back(shell::ify(arg));
    }
  }

  // Construct resources directory path. This is necessary when facing the
  // following situation: MSVC does always produce object files and leave
  // them. Their name is crafted wrt the source file only. Thus compiling in
  // parallel the same source file (with different compilation flags) leads to
  // two different cl.exe processes writing to the same object file and causes
  // "Access denied" errors and failed compilation. Thus for each compilation
  // nsconfig creates a directory whose name is chosen wrt to the output name
  // given by the user only and tells MSVC to store in this directory all
  // "side" files.
  std::string resdir("cl.exe-side-files\\");
  if (output.size() > 0) {
    resdir += shell::ify(ns2::sanitize(output) + "-side-files\\");
  }

  // Generate output flag wrt stop_stage. This is rather different from
  // GCC/Clang where -c, -S or -E flags stop the compiler at this stage and
  // the output is put into the file specified by the -o option. But for MSVC
  // those options does not all stop the compiler but rather produce different
  // files whose filenames can be choosen but with different compiler options.
  // While GCC/Clang have -o, MSVC has "/Fe" for the final executable, "/Fd"
  // for the debug infos file, "/Fa" for the assembly output and "/Fo" for
  // the object files.
  if (output.size() > 0) {
    switch (stop_stage) {
    case Assemble:
      ret.push_back("/Fa" + shell::ify(output));
      ret.push_back("/Fe" + resdir);
      ret.push_back("/Fo" + resdir);
      break;
    case Compile:
      ret.push_back("/Fo" + shell::ify(output));
      break;
    case CompileLink:
      ret.push_back("/Fe" + shell::ify(output));
      ret.push_back("/Fo" + resdir);
      break;
    }
  }

  // we always put debug infos in the resdir directory
  if (debug_infos) {
    ret.push_back("/Fd" + resdir);
  }

  // add ffast-math compiler flag
  if (ffast_math) {
    ret.push_back("/fp:fast");
  } else {
    ret.push_back("/fp:strict");
  }

  // finally add linker flags
  ret.push_back("/link");
  ret.push_back("/INCREMENTAL:NO");
  if (linker_args.size() > 0) {
    ret.insert(ret.end(), linker_args.begin(), linker_args.end());
  }

  if (debug_infos || stop_stage != Assemble) {
    ret[0] = "cmd /c ( if not exist " + resdir + " md " + resdir + " ) & cl";
  }
  return uniq(ret);
}

// ----------------------------------------------------------------------------

static std::vector<std::string> icc(std::string const &compiler,
                                    std::vector<parser::token_t> const &tokens,
                                    compiler::infos_t const &ci,
                                    parser::infos_t const &pi,
                                    parser::infos_t::action_t action) {
  std::vector<std::string> ret;
  ret.push_back(compiler);
  std::map<std::string, std::string> args;
  bool ffast_math = false;

  args["-std=c89"] = "-std=c89 -pedantic";
  args["-std=c99"] = "-std=c99 -pedantic";
  args["-std=c11"] = "-std=c11 -pedantic";
  args["-std=c++98"] = "-std=c++98 -pedantic";
  args["-std=c++03"] = ""; // -std=c++03 is not supported by icc
  args["-std=c++11"] = "-std=c++11 -pedantic";
  args["-std=c++14"] = "-std=c++14 -pedantic";
  args["-std=c++17"] = "-std=c++17 -pedantic";
  args["-std=c++20"] = "-std=c++20 -pedantic";
  args["-O0"] = "-O0";
  args["-O1"] = "-O1";
  args["-O2"] = "-O2";
  args["-O3"] = "-O3";
  args["-g"] = "-g";
  args["-S"] = "-S";
  args["-c"] = "-c";
  args["-o"] = "-o";
  args["-x"] = "-x";
  args["-Wall"] = "-Wall -Wextra -Wconversion -Wsign-conversion";
  // -Wdouble-promotion is not supported by icc
  args["-fPIC"] = "-fPIC";
  args["-static-libstdc++"] = "-static-libstdc++ -static-libgcc";
  args["-msse"] = "-msse";
  args["-msse2"] = "-msse2";
  args["-msse3"] = "-msse3";
  args["-mssse3"] = "-mssse3";
  args["-msse41"] = "-msse4.1";
  args["-msse42"] = "-msse4.2";
  args["-mavx"] = "-mavx";
  args["-mavx2"] = "-mavx2";
  args["-mavx512_knl"] = "-mavx512f -mavx512pf -mavx512er -mavx512cd";
  args["-mavx512_skylake"] = "-mavx512f -mavx512dq -mavx512cd -mavx512bw "
                             "-mavx512vl -march=skylake-avx512";
  args["-mneon64"] = "";
  args["-mneon128"] = "";
  args["-maarch64"] = "";
  args["-msve"] = "";
  args["-msve128"] = "";
  args["-msve256"] = "";
  args["-msve512"] = "";
  args["-msve1024"] = "";
  args["-msve2048"] = "";
  args["-mfma"] = "-mfma";
  args["-mfp16"] = "-mf16c";
  args["-fopenmp"] = "-fopenmp";
  args["-shared"] = "-shared";
  args["-fdiagnostics-color=always"] = "";
  args["-mvmx"] = "";
  args["-mvsx"] = "";
  args["-mwasm_simd128"] = "";
  args["-msm_35"] = "";
  args["-msm_50"] = "";
  args["-msm_53"] = "";
  args["-msm_60"] = "";
  args["-msm_61"] = "";
  args["-msm_62"] = "";
  args["-msm_70"] = "";
  args["-msm_72"] = "";
  args["-msm_75"] = "";
  args["-fno-omit-frame-pointer"] = "-fno-omit-frame-pointer "
                                    "-mno-omit-leaf_frame-pointer";
  args["-vec-report"] = "-qopt-report -qopt-report-phase=vec "
                        "-qopt-report-file=stdout";

  for (size_t i = 1; i < tokens.size(); i++) {
    std::string const &arg = tokens[i].text;
    // The effect of --version on the command line is that the compiler
    // performs no action but displaying some infos, so no need to pass
    // other flags that will be ignored
    if (arg == "--version") {
      ret.clear();
      ret.push_back(compiler);
      ret.push_back("--version");
      return ret;
    } else if (arg == "-ffast-math") {
      ffast_math = true;
      continue;
    }

    std::vector<std::string> buf =
        translate_single_arg(compiler, args, ci, tokens[i], pi, action);
    ret.insert(ret.end(), buf.begin(), buf.end());
  }

  if (!ffast_math) {
    // Cf. https://scicomp.stackexchange.com/questions/20815
    //     /performance-of-icc-main-cpp-g-ffast-math-main-cpp
    ret.push_back("-fp-model strict");
  }

  return uniq(ret);
}

// ----------------------------------------------------------------------------

static std::vector<std::string>
nvcc(compiler::infos_t const &ci, std::vector<parser::token_t> const &tokens,
     compiler::infos_t const &host_ci, parser::infos_t *pi_,
     parser::infos_t::action_t action) {
  // NVCC does not support much options, almost all of them are to be passed
  // to the host compiler. The logic here is different than for other
  // compilers: we remove from `tokens` arguments that are nvcc specific,
  // pass the rest of them for host compiler translation and get back the list
  // that we include in the command line. We make no effort to keep the order
  // of arguments as nvcc reorders them before passing them to the host
  // compiler.

  parser::infos_t &pi = *pi_;
  std::vector<std::string> ret;
  std::vector<parser::token_t> host_tokens(1);
  ret.push_back(ci.path);
  ret.push_back("-ccbin " + host_ci.path);
  ret.push_back("-m" + ns2::to_string(ci.nbits));
  size_t x_cu_id = ret.size() - 1;
  bool only_cpp_as_input = true;
  std::map<std::string, std::string> args;
  args["-msm_35"] = "-arch=sm_35";
  args["-msm_50"] = "-arch=sm_50";
  args["-msm_53"] = "-arch=sm_53";
  args["-msm_60"] = "-arch=sm_60";
  args["-msm_61"] = "-arch=sm_61";
  args["-msm_62"] = "-arch=sm_62";
  args["-msm_70"] = "-arch=sm_70";
  args["-msm_72"] = "-arch=sm_72";
  args["-msm_75"] = "-arch=sm_75";
  args["-c"] = "-c";
  args["-o"] = "-o";
  args["-shared"] = "-shared";
  args["-S"] = "--ptx";
  args["-g"] = "-g -G -lineinfo";
  std::string x_cu_language;
  bool next_arg_is_output = false;
  for (size_t i = 1; i < tokens.size(); i++) {
    std::string const &arg = tokens[i].text;
    // The effect of --version on the command line is that the compiler
    // performs no action but displaying some infos, so no need to pass
    // other flags that will be ignored
    if (arg == "--version") {
      ret.clear();
      ret.push_back(ci.path);
      ret.push_back("--version");
      return ret;
    }
    if (arg == "-x") {
      if (i == tokens.size() - 1) {
        die("no language given after", tokens[i].cursor);
      }
      i++;
      x_cu_language = tokens[i].text;
    }
    if (arg == "-o") {
      next_arg_is_output = true;
    }
    // If we do not find the argument in args, we do not emit an error, we
    // pass the argument to the host compiler.
    std::map<std::string, std::string>::const_iterator it = args.find(arg);
    if (it != args.end()) {
      ret.push_back(it->second);
      continue;
    }
    if (arg == "-lm" || arg == "-lpthread") {
      host_tokens.push_back(tokens[i]);
      continue;
    }
    if (arg == "-std=c++03" || arg == "-std=c++11" || arg == "-std=c++14" ||
        arg == "-std=c++17" || arg == "-std=c++20") {
      ret.push_back("-std c++" + arg.substr(8));
      host_tokens.push_back(tokens[i]);
      continue;
    }
    if (arg == "-fast-math") {
      ret.push_back("--use_fast_math");
      host_tokens.push_back(tokens[i]);
      continue;
    }
    if (arg[0] == '-') {
      if (arg[1] == 'l' && arg[2] == ':') {
        if (arg.size() == 3) {
          die("no file/directory given here", tokens[i].cursor);
        }
        ret.push_back("-l:" + shell::ify(&arg[3]));
      } else if (arg[1] == 'I' || arg[1] == 'L' || arg[1] == 'l') {
        if (arg.size() == 2) {
          die("no file/directory given here", tokens[i].cursor);
        }
        if (arg[1] == 'l') {
          ret.push_back("-l" + shell::ify(lib_basename(&arg[2])));
        } else if (arg[1] == 'L') {
          std::string path(shell::ify(&arg[2]));
          ret.push_back("-L" + path);
          std::string rpath("'" + get_rpath_argument(path, host_ci) + "'");
          if (rpath.size() > 0) {
            ret.push_back("-Xlinker " + rpath);
          }
        } else {
          ret.push_back(std::string("-") + arg[1] + shell::ify(&arg[2]));
        }
      } else if (arg[1] == 'D') {
        if (arg.size() == 2) {
          die("no macro name given here", tokens[i].cursor);
        }
        host_tokens.push_back(tokens[i]);
        ret.push_back(arg);
      } else {
        host_tokens.push_back(tokens[i]);
      }
    } else {
      // By default nvcc treats C++ files as not containing any device code
      // which is different from hipcc, dpcpp and SYCL compilers. As we want
      // the "same" behavior for nvcc if the user only provides C++ files to
      // nvcc then we tell nvcc that they contain CUDA code. If they really
      // don't then it should not make a difference as the resulting code is
      // passed to the host compiler. If we see other file extensions then
      // we let nvcc decide on its own.
      if (!next_arg_is_output) {
        if (arg.size() >= 4) {
          std::string lo_arg(ns2::lower(arg));
          if (!ns2::endswith(lo_arg, ".cc") &&
              !ns2::endswith(lo_arg, ".cpp") &&
              !ns2::endswith(lo_arg, ".cxx")) {
            only_cpp_as_input = false;
          }
        } else {
          only_cpp_as_input = false;
        }
      }
      ret.push_back(stringify(ns2::sanitize(arg)));
      next_arg_is_output = false;
    }
  }

  // Do not forget the -x option if need be. If x_cu_language is not "" then
  // -x has been given by the user and we pass it as-is to nvcc. Otherwise
  // we only put a '-x cu' if only C++ source files were given.
  if (x_cu_language.size() > 0) {
    ret.insert(ret.begin() + int(x_cu_id), "-x " + x_cu_language);
  } else {
    if (only_cpp_as_input) {
      ret.insert(ret.begin() + int(x_cu_id), "-x cu");
    }
  }

  // Translate arguments for host compiler, in any case we do not want header
  // dependencies flags as nvcc handles them
  std::vector<std::string> host_ret =
      comp(host_ci, host_tokens, &pi, action, NULL);

  // Remove first argument as it is the name of the executable. Note that
  // for MSVC is it the name of the executable + the making of the directory
  // for the unwanted-badly-named side files generated by cl.exe.
  host_ret.erase(host_ret.begin());

  // NVCC does NOT produce C++ standard code before passing it to the host
  // compiler. Therefore compiling with -pedantic is a bad idea as almost
  // every line of code makes GCC emits a warning about GCC specific syntax.
  // Apparently, there is no way to tell NVCC to produce correct code so we
  // get rid of all -pedantic flags.
  for (size_t i = 0; i < host_ret.size(); i++) {
    host_ret[i] = ns2::replace(host_ret[i], " -pedantic", "");
  }

  if (host_ret.size() > 0) {
    std::string host_args;
    for (size_t i = 0; i < host_ret.size(); i++) {
      if (i > 0) {
        host_args += ",";
      }
      if (host_ret[i].find(" ") != std::string::npos) {
        host_args += "\"" + host_ret[i] + "\"";
      } else {
        host_args += host_ret[i];
      }
    }
    ret.push_back("-Xcompiler " + host_args);
  }

  return ret;
}

// ----------------------------------------------------------------------------

static std::vector<std::string>
hipcc_hcc_dpcpp(std::string const &compiler,
                std::vector<parser::token_t> const &tokens,
                compiler::infos_t const &ci, parser::infos_t const &pi,
                parser::infos_t::action_t action) {
  std::vector<std::string> ret;
  ret.push_back(compiler);
  std::map<std::string, std::string> args;
  args["-std=c89"] = "-std=c89 -pedantic";
  args["-std=c99"] = "-std=c99 -pedantic";
  args["-std=c11"] = "-std=c11 -pedantic";
  args["-std=c++98"] = "-std=c++98 -pedantic";
  args["-std=c++03"] = "-std=c++03 -pedantic";
  args["-std=c++11"] = "-std=c++11 -pedantic";
  args["-std=c++14"] = "-std=c++14 -pedantic";
  args["-std=c++17"] = "-std=c++17 -pedantic";
  args["-std=c++20"] = "-std=c++20 -pedantic";
  args["-O0"] = "-O0";
  args["-O1"] = "-O1";
  args["-O2"] = "-O2";
  args["-O3"] = "-O3";
  args["-ffast-math"] = "-ffast-math";
  args["-g"] = "-g";
  args["-S"] = "-S";
  args["-c"] = "-c";
  args["-o"] = "-o";
  args["-x"] = "-x";
  args["-Wall"] =
      "-Wall -Wextra -Wdouble-promotion -Wconversion -Wsign-conversion";
  args["-fPIC"] = "-fPIC";
  args["-static-libstdc++"] = "-static-libstdc++ -static-libgcc";
  args["-msse"] = "";
  args["-msse2"] = "";
  args["-msse3"] = "";
  args["-mssse3"] = "";
  args["-msse41"] = "";
  args["-msse42"] = "";
  args["-mavx"] = "";
  args["-mavx2"] = "";
  args["-mavx512_knl"] = "";
  args["-mavx512_skylake"] = "";
  args["-mneon64"] = "";
  args["-mneon128"] = "";
  args["-maarch64"] = "";
  args["-msve"] = "";
  args["-msve128"] = "";
  args["-msve256"] = "";
  args["-msve512"] = "";
  args["-msve1024"] = "";
  args["-msve2048"] = "";
  args["-mvmx"] = "";
  args["-mvsx"] = "";
  args["-mwasm_simd128"] = "";
  args["-msm_35"] = "--cuda-gpu-arch=sm_35";
  args["-msm_50"] = "--cuda-gpu-arch=sm_50";
  args["-msm_53"] = "--cuda-gpu-arch=sm_53";
  args["-msm_60"] = "--cuda-gpu-arch=sm_60";
  args["-msm_61"] = "--cuda-gpu-arch=sm_61";
  args["-msm_62"] = "--cuda-gpu-arch=sm_62";
  args["-msm_70"] = "--cuda-gpu-arch=sm_70";
  args["-msm_72"] = "--cuda-gpu-arch=sm_72";
  args["-msm_75"] = "--cuda-gpu-arch=sm_75";
  args["-mfma"] = "";
  args["-mfp16"] = "";
  args["-fopenmp"] = "-fopenmp";
  args["-shared"] = "-shared";
  args["-fdiagnostics-color=always"] = "-fdiagnostics-color=always";
  /* https://stackoverflow.com/questions/43864881
      /fno-omit-frame-pointer-equivalent-compiler-option-for-clang */
  args["-fno-omit-frame-pointer"] = "-fno-omit-frame-pointer "
                                    "-mno-omit-leaf-frame-pointer";
  args["-vec-report"] = "-Rpass=loop-vectorize "
                        "-Rpass-missed=loop-vectorize "
                        "-Rpass-analysis=loop-vectorize";

  for (size_t i = 1; i < tokens.size(); i++) {
    std::string const &arg = tokens[i].text;
    // The effect of --version on the command line is that the compiler
    // performs no action but displaying some infos, so no need to pass
    // other flags that will be ignored
    if (arg == "--version") {
      ret.clear();
      ret.push_back(compiler);
      ret.push_back("--version");
      return ret;
    }
    std::vector<std::string> buf =
        translate_single_arg(compiler, args, ci, tokens[i], pi, action);
    ret.insert(ret.end(), buf.begin(), buf.end());
  }

  return uniq(ret);
}

// ----------------------------------------------------------------------------

static std::vector<std::string>
emscripten(std::string const &compiler,
           std::vector<parser::token_t> const &tokens,
           compiler::infos_t const &ci, parser::infos_t const &pi,
           parser::infos_t::action_t action) {
  std::vector<std::string> ret;
  ret.push_back(compiler);
  ret.push_back("-Wno-version-check");
  ret.push_back("-Wno-emcc");
  std::map<std::string, std::string> args;

  args["-std=c89"] = "-std=c89 -pedantic";
  args["-std=c99"] = "-std=c99 -pedantic";
  args["-std=c11"] = "-std=c11 -pedantic";
  args["-std=c++98"] = "-std=c++98 -pedantic";
  args["-std=c++03"] = "-std=c++03 -pedantic";
  args["-std=c++11"] = "-std=c++11 -pedantic";
  args["-std=c++14"] = "-std=c++14 -pedantic";
  args["-std=c++17"] = "-std=c++17 -pedantic";
  args["-std=c++20"] = "-std=c++20 -pedantic";
  args["-O0"] = "-O0";
  args["-O1"] = "-O1";
  args["-O2"] = "-O2";
  args["-O3"] = "-O3";
  args["-g"] = "-g";
  args["-S"] = "-S";
  args["-c"] = "-c";
  args["-o"] = "-o";
  args["-x"] = "-x";
  args["-Wall"] =
      "-Wall -Wextra -Wdouble-promotion -Wconversion -Wsign-conversion";
  args["-fPIC"] = "";
  args["-static-libstdc++"] = "";
  args["-msse"] = "";
  args["-msse2"] = "";
  args["-msse3"] = "";
  args["-mssse3"] = "";
  args["-msse41"] = "";
  args["-msse42"] = "";
  args["-mavx"] = "";
  args["-mavx2"] = "";
  args["-mavx512_knl"] = "";
  args["-mavx512_skylake"] = "";
  args["-mneon64"] = "";
  args["-mneon128"] = "";
  args["-maarch64"] = "";
  args["-msve"] = "";
  args["-msve128"] = "";
  args["-msve256"] = "";
  args["-msve512"] = "";
  args["-msve1024"] = "";
  args["-msve2048"] = "";
  args["-mfma"] = "";
  args["-mfp16"] = "";
  args["-fopenmp"] = "";
  args["-shared"] = "-shared";
  args["-fdiagnostics-color=always"] = "";
  args["-mvmx"] = "";
  args["-mvsx"] = "";
  args["-mwasm_simd128"] = "-msimd128";
  args["-msm_35"] = "";
  args["-msm_50"] = "";
  args["-msm_53"] = "";
  args["-msm_60"] = "";
  args["-msm_61"] = "";
  args["-msm_62"] = "";
  args["-msm_70"] = "";
  args["-msm_72"] = "";
  args["-msm_75"] = "";
  args["-fno-omit-frame-pointer"] = "";
  args["-vec-report"] = "-Rpass=loop-vectorize "
                        "-Rpass-missed=loop-vectorize "
                        "-Rpass-analysis=loop-vectorize";

  for (size_t i = 1; i < tokens.size(); i++) {
    std::string const &arg = tokens[i].text;
    // The effect of --version on the command line is that the compiler
    // performs no action but displaying some infos, so no need to pass
    // other flags that will be ignored
    if (arg == "--version") {
      ret.clear();
      ret.push_back(compiler);
      ret.push_back("--version");
      return ret;
    }

    std::vector<std::string> buf =
        translate_single_arg(compiler, args, ci, tokens[i], pi, action);
    ret.insert(ret.end(), buf.begin(), buf.end());
  }

  return uniq(ret);
}

// ----------------------------------------------------------------------------

static std::vector<std::string> xlc(std::string const &compiler,
                                    std::vector<parser::token_t> const &tokens,
                                    compiler::infos_t const &ci,
                                    parser::infos_t const &pi,
                                    parser::infos_t::action_t action) {
  std::vector<std::string> ret;
  ret.push_back(compiler);
  std::map<std::string, std::string> args;

  args["-std=c89"] = "-qlanglvl=stdc89";
  args["-std=c99"] = "-qlanglvl=stdc99";
  args["-std=c11"] = "-qlanglvl=stdc11";
  args["-std=c++98"] = "";
  args["-std=c++03"] = "";
  args["-std=c++11"] = "-qlanglvl=extended0x";
  args["-std=c++14"] = "-qlanglvl=extended1y";
  args["-std=c++17"] = "-qlanglvl=extended1y";
  args["-std=c++20"] = "-qlanglvl=extended1y";
  args["-O0"] = "";
  args["-O1"] = "-qoptimize=0";
  args["-O2"] = "-qoptimize=2";
  args["-O3"] = "-qoptimize=4";
  args["-g"] = "-g";
  args["-S"] = "-S";
  args["-c"] = "-c";
  args["-o"] = "-o";
  args["-x"] = "-x";
  args["-Wall"] = "";
  args["-fPIC"] = "-qpic";
  args["-static-libstdc++"] = "";
  args["-msse"] = "";
  args["-msse2"] = "";
  args["-msse3"] = "";
  args["-mssse3"] = "";
  args["-msse41"] = "";
  args["-msse42"] = "";
  args["-mavx"] = "";
  args["-mavx2"] = "";
  args["-mavx512_knl"] = "";
  args["-mavx512_skylake"] = "";
  args["-mneon64"] = "";
  args["-mneon128"] = "";
  args["-maarch64"] = "";
  args["-msve"] = "";
  args["-msve128"] = "";
  args["-msve256"] = "";
  args["-msve512"] = "";
  args["-msve1024"] = "";
  args["-msve2048"] = "";
  args["-mfma"] = "";
  args["-mfp16"] = "";
  args["-fopenmp"] = "-qsmp=omp";
  args["-shared"] = "-qmkshrobj";
  args["-fdiagnostics-color=always"] = "";
  args["-mvmx"] = "";
  args["-mvsx"] = "";
  args["-mwasm_simd128"] = "";
  args["-msm_35"] = "";
  args["-msm_50"] = "";
  args["-msm_53"] = "";
  args["-msm_60"] = "";
  args["-msm_61"] = "";
  args["-msm_62"] = "";
  args["-msm_70"] = "";
  args["-msm_72"] = "";
  args["-msm_75"] = "";
  args["-fno-omit-frame-pointer"] = "";
  args["-vec-report"] = "";

  for (size_t i = 1; i < tokens.size(); i++) {
    std::string const &arg = tokens[i].text;
    // The effect of --version on the command line is that the compiler
    // performs no action but displaying some infos, so no need to pass
    // other flags that will be ignored
    if (arg == "--version") {
      ret.clear();
      ret.push_back(compiler);
      ret.push_back("-qversion");
      return ret;
    }
    std::vector<std::string> buf =
        translate_single_arg(compiler, args, ci, tokens[i], pi, action);
    ret.insert(ret.end(), buf.begin(), buf.end());
  }

  return uniq(ret);
}


// ----------------------------------------------------------------------------

static std::vector<std::string> fcc(std::string const &compiler,
                                    std::vector<parser::token_t> const &tokens,
                                    compiler::infos_t const &ci,
                                    parser::infos_t const &pi,
                                    parser::infos_t::action_t action) {
  std::vector<std::string> ret;
  ret.push_back(compiler);
  if (ci.type == compiler::infos_t::FCC_trad_mode) {
    ret.push_back("-Nnoclang");
  } else {
    ret.push_back("-Nclang");
  }
  std::map<std::string, std::string> args;
  bool msve = false;

  args["-std=c89"] = "-std=c89";
  args["-std=c99"] = "-std=c99";
  args["-std=c11"] = "-std=c11";
  args["-std=c++98"] = "-std=c++03";
  args["-std=c++03"] = "-std=c++03";
  args["-std=c++11"] = "-std=c++11";
  args["-std=c++14"] = "-std=c++14";
  args["-std=c++17"] = "-std=c++17";
  args["-std=c++20"] = "-std=c++17";
  args["-O0"] = "-O0";
  args["-O1"] = "-O1";
  args["-O2"] = "-O2";
  args["-O3"] = "-O3";
  args["-g"] = "-g";
  args["-S"] = "-S";
  args["-c"] = "-c";
  args["-o"] = "-o";
  //args["-x"] = "-x"; // must be handled in the main loop
  args["-Wall"] = "";
  args["-fPIC"] = "-fPIC";
  args["-static-libstdc++"] = "";
  args["-msse"] = "";
  args["-msse2"] = "";
  args["-msse3"] = "";
  args["-mssse3"] = "";
  args["-msse41"] = "";
  args["-msse42"] = "";
  args["-mavx"] = "";
  args["-mavx2"] = "";
  args["-mavx512_knl"] = "";
  args["-mavx512_skylake"] = "";
  args["-mneon64"] = "";
  args["-mneon128"] = "";
  args["-maarch64"] = "";
  if (ci.type == compiler::infos_t::FCC_trad_mode) {
    args["-msve"] = "-KSVE -Ksimd_reg_size=agnostic";
    args["-msve128"] = "-KSVE -Ksimd_reg_size=128";
    args["-msve256"] = "-KSVE -Ksimd_reg_size=256";
    args["-msve512"] = "-KSVE -Ksimd_reg_size=512";
  } else {
    args["-msve"] = "-mcpu=generic+sve+fp16";
    args["-msve128"] = "";
    args["-msve256"] = "";
    args["-msve512"] = "";
  }
  args["-msve1024"] = "";
  args["-msve2048"] = "";
  args["-mfma"] = "";
  args["-mfp16"] = "";
  args["-fopenmp"] = "-fopenmp";
  args["-shared"] = "-shared";
  args["-fdiagnostics-color=always"] = "";
  args["-mvmx"] = "";
  args["-mvsx"] = "";
  args["-mwasm_simd128"] = "";
  args["-msm_35"] = "";
  args["-msm_50"] = "";
  args["-msm_53"] = "";
  args["-msm_60"] = "";
  args["-msm_61"] = "";
  args["-msm_62"] = "";
  args["-msm_70"] = "";
  args["-msm_72"] = "";
  args["-msm_75"] = "";
  args["-fno-omit-frame-pointer"] = "-fno-omit-frame-pointer";
  args["-vec-report"] = "";

  for (size_t i = 1; i < tokens.size(); i++) {
    std::string const &arg = tokens[i].text;
    // The effect of --version on the command line is that the compiler
    // performs no action but displaying some infos, so no need to pass
    // other flags that will be ignored
    if (arg == "--version") {
      ret.clear();
      ret.push_back(compiler);
      ret.push_back("--version");
      return ret;
    } else if (arg == "-x") {
      if (i + 1 >= tokens.size()) {
        die("argument -x must be followed by the language", tokens[i].cursor);
      }
      i++;
      continue;
    }

    if (arg == "-msve" || arg == "-msve128" || arg == "-msve256" ||
        arg == "-msve512") {
      msve = true;
    }
    std::vector<std::string> buf =
        translate_single_arg(compiler, args, ci, tokens[i], pi, action);
    ret.insert(ret.end(), buf.begin(), buf.end());
  }

  if (msve == false) {
    if (ci.type == compiler::infos_t::FCC_trad_mode) {
      ret.push_back("-KNOSVE");
    } else {
      ret.push_back("-mcpu=generic+nosve");
    }
  }

  return uniq(ret);
}

// ----------------------------------------------------------------------------

bool command_is_compiler(std::string const &cmd) {
  if (cmd == "cc" || cmd == "c++" || cmd == "msvc" || cmd == "gcc" ||
      cmd == "g++" || cmd == "clang" || cmd == "clang++" || cmd == "mingw" ||
      cmd == "armclang" || cmd == "armclang++" || cmd == "icc" ||
      cmd == "nvcc" || cmd == "hipcc" || cmd == "hcc" || cmd == "dpcpp" ||
      cmd == "cuda-host-c++" || cmd == "fcc_trad_mode" ||
      cmd == "FCC_trad_mode" || cmd == "fcc_clang_mode" ||
      cmd == "FCC_clang_mode" || cmd == "emcc" || cmd == "em++" ||
      cmd == "xlc" || cmd == "xlc++") {
    return true;
  }
  return false;
}

// ----------------------------------------------------------------------------

static std::string single_command(std::vector<parser::token_t> const &tokens,
                                  parser::infos_t::action_t action,
                                  parser::infos_t *pi_,
                                  autodeps_t *autodeps_) {
  parser::infos_t &pi = *pi_;
  std::string const &cmd = tokens[0].text;
  if (cmd == "touch") {
    return touch(tokens);
  } else if (cmd == "cd") {
    return cd(tokens);
  } else if (cmd == "rm") {
    return rm(tokens);
  } else if (cmd == "cp") {
    return cp(tokens);
  } else if (cmd == "mkdir") {
    return mkdir(tokens);
  } else if (cmd == "cat") {
    return cat(tokens);
  } else if (cmd == "echo") {
    return echo(tokens);
  } else if (cmd == "mv") {
    return mv(tokens);
  } else if (cmd == "if") {
    return if_(tokens, action, &pi, autodeps_);
  } else if (cmd == "ar") {
    return ar(tokens);
  } else if (command_is_compiler(cmd)) {
    compiler::infos_t ci = compiler::get(cmd, &pi);
    return ns2::join(comp(ci, tokens, &pi, action, autodeps_), " ");
  } else if (action == parser::infos_t::Raw ||
             action == parser::infos_t::Permissive) {
    return raw(tokens);
  } else {
    die("unknown command", tokens[0].cursor);
  }
  return std::string(); // should never be reached
}

// ----------------------------------------------------------------------------

std::string translate(std::string const &cmd, parser::infos_t::action_t,
                      parser::infos_t *, autodeps_t *autodeps_) {
  if (autodeps_ != NULL) {
    autodeps_->cmd = cmd;
  }
  return cmd;
}

// ----------------------------------------------------------------------------

std::string translate(std::vector<parser::token_t> const &tokens,
                      parser::infos_t::action_t action, parser::infos_t *pi_,
                      autodeps_t *autodeps_) {
  parser::infos_t &pi = *pi_;
  if (action == parser::infos_t::Raw) {
    return raw(tokens);
    if (autodeps_ != NULL) {
      autodeps_->cmd.clear();
      autodeps_->by = compiler::infos_t::None;
    }
  } else {
    std::string ret;
    for (size_t i0 = 0; i0 < tokens.size();) {
      size_t i1;

      // goto the next '&&', '||', '|', ';', '>', '>>' or '<'
      for (i1 = i0 + 1; i1 < tokens.size() && tokens[i1].text != "&&" &&
                        tokens[i1].text != "||" && tokens[i1].text != "|" &&
                        tokens[i1].text != ";" && tokens[i1].text != ">" &&
                        tokens[i1].text != ">>" && tokens[i1].text != "<";
           i1++)
        ;

      // translate previous command
      if (autodeps_ != NULL) {
        autodeps_t buf;
        buf.by = autodeps_->by; // in case there is no compilation
        buf.file = autodeps_->file;
        ret += single_command(
            std::vector<parser::token_t>(tokens.begin() + long(i0),
                                         tokens.begin() + long(i1)),
            action, &pi, &buf);
        autodeps_->by = buf.by;
        autodeps_->cmd += buf.cmd;
      } else {
        ret += single_command(
            std::vector<parser::token_t>(tokens.begin() + long(i0),
                                         tokens.begin() + long(i1)),
            action, &pi, NULL);
      }

      if (i1 >= tokens.size()) {
        break;
      }

      // add separator
      if (tokens[i1].text == ">" || tokens[i1].text == ">>" ||
          tokens[i1].text == "<") {
        i1++;
        if (i1 >= tokens.size()) {
          die("expected file after", tokens[i1 - 1].cursor);
        }
        ret += " " + tokens[i1 - 1].text + tokens[i1].text;
        if (autodeps_ != NULL) {
          autodeps_->cmd += " " + tokens[i1 - 1].text + tokens[i1].text;
        }
      } else if (tokens[i1].text == ";") {
#ifdef NS2_IS_MSVC
        ret += " & ";
        if (autodeps_ != NULL) {
          autodeps_->cmd += " & ";
        }
#else
        ret += " ; ";
        if (autodeps_ != NULL) {
          autodeps_->cmd += " ; ";
        }
#endif
      } else {
        ret += " " + tokens[i1].text + " ";
        if (autodeps_ != NULL) {
          autodeps_->cmd += " " + tokens[i1].text + " ";
        }
      }

      // update i0
      i0 = i1 + 1;
    }
    return ret;
  }
}

// ----------------------------------------------------------------------------

std::string zip(std::string const &dirname) {
#ifdef NS2_IS_MSVC
  return "powershell -Command Compress-Archive -Force -Path " +
         stringify(ns2::sanitize(dirname)) + " -DestinationPath " +
         stringify(ns2::sanitize(dirname + ".zip"));
#else
  return "tar -cvjSf " + stringify(dirname + ".tar.bz2") + " " +
         stringify(dirname);
#endif
}

// ----------------------------------------------------------------------------

} // namespace shell
