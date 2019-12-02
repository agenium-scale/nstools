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
  return "echo >" + stringify(ns2::sanitize(tokens[0].text));
#else
  return "touch" + stringify(tokens[0].text);
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
                       parser::infos_t *pi_) {
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
  ret += translate(
      std::vector<parser::token_t>(tokens.begin() + 5, tokens.begin() + i),
      &pi);

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
  ret += translate(std::vector<parser::token_t>(tokens.begin() + i0_else,
                                                tokens.begin() + i),
                   &pi);

#ifdef NS2_IS_MSVC
  ret += " )";
#else
  ret += " ; fi";
#endif

  return ret;
}

// ----------------------------------------------------------------------------

static std::string gcc_clang(std::string const &compiler,
                             std::vector<parser::token_t> const &tokens,
                             compiler::infos_t const &ci,
                             parser::infos_t *pi_) {
  std::vector<std::string> temp;
  temp.push_back(compiler);
  parser::infos_t &pi = *pi_;
  std::map<std::string, std::string> args;
  args["-std=c89"] = "-std=c89 -pedantic";
  args["-std=c99"] = "-std=c99 -pedantic";
  args["-std=c11"] = "-std=c11 -pedantic";
  args["-std=c++98"] = "-std=c++98 -pedantic";
  args["-std=c++03"] = "-std=c++03 -pedantic";
  args["-std=c++11"] = "-std=c++11 -pedantic";
  args["-std=c++14"] = "-std=c++14 -pedantic";
  args["-std=c++17"] = "-std=c++17 -pedantic";
  args["-O0"] = "-O0";
  args["-O1"] = "-O1";
  args["-O2"] = "-O2";
  args["-O3"] = "-O3";
  args["-Og"] = "-Og";
  args["-ffast-math"] = "-ffast-math";
  args["-g"] = "-g";
  args["-S"] = "-S";
  args["-c"] = "-c";
  args["-o"] = "-o";
  args["-Wall"] =
      "-Wall -Wextra -Wdouble-promotion -Wconversion -Wsign-conversion";
  args["-fPIC"] = "-fPIC";
  args["-msse"] = "-msse";
  args["-msse2"] = "-msse2";
  args["-msse3"] = "-msse3";
  args["-mssse3"] = "-mssse3";
  args["-msse41"] = "-msse4.1";
  args["-msse42"] = "-msse4.2";
  args["-mavx"] = "-mavx";
  args["-mavx2"] = "-mavx2";
  args["-mavx512_knl"] = "-mavx512f -mavx512pf -mavx512er -mavx512cd";
  args["-mavx512_skylake"] =
      "-mavx512f -mavx512dq -mavx512cd -mavx512bw -mavx512vl";
  args["-mneon64"] = "-mfpu=neon";
  args["-mneon128"] = "-mfpu=neon";
  args["-maarch64"] = "";
  args["-msve"] = "-march=armv8-a+sve";

  // Power extensions
  args["-maltivec"] = "-maltivec";
  args["-mcpu=power7"] = "-mcpu=power7";

  if (ci.arch == compiler::infos_t::Intel) {
    args["-mfma"] = "-mfma";
    args["-mfp16"] = "-mf16c";
  } else if (ci.arch == compiler::infos_t::ARM) {
    args["-mfma"] = "";
    if (ci.nbits == 32) {
      args["-mfp16"] = "";
    } else {
      args["-mfp16"] = "-march=native+fp16";
    }
  }
  args["-fopenmp"] = "-fopenmp";
  args["-shared"] = "-shared";
  args["--coverage"] = "--coverage";
  args["--color"] = "-fdiagnostics-color=always";

  for (size_t i = 1; i < tokens.size(); i++) {
    std::string const &arg = tokens[i].text;
    // The effect of --version on the command line is that the compiler
    // performs no action but displaying some infos, so no need to pass
    // other flags that will be ignored
    if (arg == "--version") {
      return temp[0] + " --version";
    }
    if (arg == "-lpthread") {
      temp.push_back("-lpthread");
      continue;
    } else if (arg == "-lm") {
      temp.push_back("-lm");
      continue;
    } else if (arg == "-L.") {
      temp.push_back("-L.");
      temp.push_back("'-Wl,-rpath=$ORIGIN'");
      continue;
    }
    if (arg[0] == '-') {
      if (arg[1] == 'l' && arg[2] == ':') {
        if (arg.size() == 3) {
          die("no file/directory given here", tokens[i].cursor);
        }
        temp.push_back("-l:" + shell::ify(&arg[3]));
      } else if (arg[1] == 'I' || arg[1] == 'L' || arg[1] == 'l') {
        if (arg.size() == 2) {
          die("no file/directory given here", tokens[i].cursor);
        }
        if (arg[1] == 'l') {
          temp.push_back("-l" + shell::ify(lib_basename(&arg[2])));
        } else if (arg[1] == 'L') {
          std::string path(shell::ify(&arg[2]));
          temp.push_back("-L" + path);
          temp.push_back("-Wl,-rpath=" + path);
        } else {
          temp.push_back(std::string("-") + arg[1] + shell::ify(&arg[2]));
        }
      } else if (arg[1] == 'D') {
        if (arg.size() == 2) {
          die("no macro name given here", tokens[i].cursor);
        }
        temp.push_back(arg);
      } else {
        std::map<std::string, std::string>::const_iterator it = args.find(arg);
        if (it != args.end()) {
          if (it->second.size() > 0) {
            temp.push_back(it->second);
          } else if (pi.verbosity >= VERBOSITY_DEBUG) {
            WARNING << "Option " << arg << " is not supported or known by "
                    << compiler << ", ignoring it" << std::endl;
          }
        } else if (pi.action == parser::infos_t::Permissive) {
          temp.push_back(arg);
        } else {
          die("unknown compiler option", tokens[i].cursor);
        }
      }
    } else {
      temp.push_back(stringify(ns2::sanitize(arg)));
    }
  }

  // if we have been asked to output include files then do it and tell the
  // the parser that it's GCC specific
  if (pi.generate_header_deps_flags) {
    pi.current_compiler.type = compiler::infos_t::GCC;
    temp.push_back("@@autodeps_flags");
  }

  return ns2::join(uniq(temp), " ");
}

// ----------------------------------------------------------------------------

static std::string msvc(std::vector<parser::token_t> const &tokens,
                        compiler::infos_t const &ci, parser::infos_t *pi_) {
  parser::infos_t &pi = *pi_;
  enum stop_stage_t { Assemble, Compile, CompileLink };
  stop_stage_t stop_stage = CompileLink;
  bool debug_infos = false;
  bool ffast_math = false;
  std::string output;
  std::vector<std::string> temp;
  temp.push_back("cl");
  temp.push_back("/nologo");
  temp.push_back("/EHsc");
  temp.push_back("/D_CRT_SECURE_NO_WARNINGS");

  std::map<std::string, std::string> args;
  args["-std=c89"] = "";
  args["-std=c99"] = "";
  args["-std=c11"] = "";
  if (ci.version >= 2000) {
    args["-std=c++98"] = "";
    args["-std=c++03"] = "";
    args["-std=c++11"] = "";
    args["-std=c++14"] = "/std:c++14";
    args["-std=c++17"] = "/std:c++17";
  } else if (ci.version >= 1900) {
    args["-std=c++98"] = "/Zc:__cplusplus";
    args["-std=c++03"] = "/Zc:__cplusplus";
    args["-std=c++11"] = "/Zc:__cplusplus";
    args["-std=c++14"] = "/std:c++14 /Zc:__cplusplus";
    args["-std=c++17"] = "/std:c++latest /Zc:__cplusplus";
  } else {
    args["-std=c++98"] = "/Zc:__cplusplus";
    args["-std=c++03"] = "/Zc:__cplusplus";
    args["-std=c++11"] = "/Zc:__cplusplus";
    args["-std=c++14"] = "/Zc:__cplusplus";
    args["-std=c++17"] = "/Zc:__cplusplus";
  }
  args["-O0"] = "/Od";
  args["-O1"] = "/O2";
  args["-O2"] = "/Ox";
  args["-O3"] = "/Ox";
  args["-Og"] = "/Od";
  args["-g"] = "/Zi";
  args["-S"] = "/FA";
  args["-c"] = "/c";
  args["-o"] = ""; // dealt with later because it's complicated for MSVC
  args["-Wall"] = "/W3";
  args["-fPIC"] = "";
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
  args["-mneon64"] = "";
  args["-mneon128"] = "";
  args["-maarch64"] = "";
  args["-msve"] = "";
  args["-mfma"] = "";
  args["-mfp16"] = "";
  args["-fopenmp"] = "/openmp";
  args["-shared"] = "/LD";
  args["--coverage"] = "";
  args["--color"] = "";

  // Power extensions
  args["-maltivec"] = "";
  args["-mcpu=power7"] = "";

  std::vector<std::string> linker_args;
  for (size_t i = 1; i < tokens.size(); i++) {
    std::string const &arg = tokens[i].text;
    // The equivalent of --version in MSVC is to call the compiler with
    // no arguments
    if (arg == "--version") {
      return "cl";
    } else if (arg == "-lpthread") {
      temp.push_back("/MT");
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
        temp.push_back("/I" + shell::ify(arg.c_str() + 2));
      } else if (arg[1] == 'D') {
        temp.push_back("/D" + stringify(arg.c_str() + 2));
      } else if (arg[1] == 'L') {
        linker_args.push_back("/LIBPATH:" + shell::ify(arg.c_str() + 2));
      } else if (arg[1] == 'l' && arg[2] == ':') {
        temp.push_back(shell::ify(&arg[3]));
      } else if (arg[1] == 'l') {
        temp.push_back(
            shell::ify("lib" + std::string(arg.c_str() + 2) + ".lib"));
      } else if (arg == "-o") {
        if (i == tokens.size() - 1) {
          parser::die("no filename given after -o", tokens[i].text);
        }
        output = tokens[i + 1].text;
        i++;
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
            temp.push_back(it->second);
          } else if (pi.verbosity >= VERBOSITY_DEBUG) {
            WARNING << "Option " << arg << " is not supported or known by msvc"
                    << ", ignoring it" << std::endl;
          }
        } else if (pi.action == parser::infos_t::Permissive) {
          temp.push_back(arg);
        } else {
          parser::die("unknown compiler option", tokens[i].cursor);
        }
      }
    } else {
      temp.push_back(shell::ify(arg));
    }
  }

  // Construct resources directory path. This is necessary when facing the
  // following situation: MSVC does always produce object files and leave
  // them. Their name is crafted wrt the source file only. Thus compiling in
  // parallel the same source file (with different compilation falgs) leads to
  // two different cl.exe processes writing to the same object file and causes
  // "Access denied" errors and failed compilation. Thus for each compilation
  // nsconfig creates a directory whose name is chosen wrt to the output name
  // given by the user only and tells MSVC to store in this directory all
  // "side" files.
  std::string resdir;
  if (output.size() == 0) {
    resdir = "resources\\";
  } else {
    resdir = shell::ify(ns2::sanitize(output) + ".resources\\");
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
      temp.push_back("/Fa" + shell::ify(output));
      temp.push_back("/Fe" + resdir);
      temp.push_back("/Fo" + resdir);
      break;
    case Compile:
      temp.push_back("/Fo" + shell::ify(output));
      break;
    case CompileLink:
      temp.push_back("/Fe" + shell::ify(output));
      temp.push_back("/Fo" + resdir);
      break;
    }
  }

  // we always put debug infos in the resdir directory
  if (debug_infos) {
    temp.push_back("/Fd" + resdir);
  }

  // if we have been asked to output include files then do it and tell the
  // the parser that it's MSVC specific
  if (pi.generate_header_deps_flags) {
    temp.push_back("@@autodeps_flags");
    pi.current_compiler.type = compiler::infos_t::MSVC;
  }

  // add ffast-math compiler flag
  if (ffast_math) {
    temp.push_back("/fp:fast");
  } else {
    temp.push_back("/fp:strict");
  }

  // finally add linker flags
  if (linker_args.size() > 0) {
    temp.push_back("/link");
    temp.insert(temp.end(), linker_args.begin(), linker_args.end());
  }

  if (debug_infos || stop_stage != Assemble) {
    return "cmd /c ( if not exist " + resdir + " md " + resdir + " ) & " +
           ns2::join(uniq(temp), " ");
  } else {
    return ns2::join(uniq(temp), " ");
  }
}

// ----------------------------------------------------------------------------

static std::string icc(std::string const &compiler,
                       std::vector<parser::token_t> const &tokens,
                       compiler::infos_t const & /*ci*/,
                       parser::infos_t *pi_) {
  std::vector<std::string> temp;
  temp.push_back(compiler);
  parser::infos_t &pi = *pi_;
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
  args["-O0"] = "-O0";
  args["-O1"] = "-O1";
  args["-O2"] = "-O2";
  args["-O3"] = "-O3";
  args["-Og"] = "-O0";
  args["-g"] = "-g";
  args["-S"] = "-S";
  args["-c"] = "-c";
  args["-o"] = "-o";
  args["-Wall"] = "-Wall -Wextra -Wconversion -Wsign-conversion";
  // -Wdouble-promotion is not supported by icc
  args["-fPIC"] = "-fPIC";
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
  args["-mfma"] = "-mfma";
  args["-mfp16"] = "-mf16c";
  args["-fopenmp"] = "-fopenmp";
  args["-shared"] = "-shared";
  args["--coverage"] = "";
  args["--color"] = "";

  // Power extensions
  args["-maltivec"] = "";
  args["-mcpu=power7"] = "";

  for (size_t i = 1; i < tokens.size(); i++) {
    std::string const &arg = tokens[i].text;
    // The effect of --version on the command line is that the compiler
    // performs no action but displaying some infos, so no need to pass
    // other flags that will be ignored
    if (arg == "--version") {
      return temp[0] + " --version";
    }
    if (arg == "-lpthread") {
      temp.push_back("-lpthread");
      continue;
    } else if (arg == "-lm") {
      temp.push_back("-lm");
      continue;
    } else if (arg == "-L.") {
      temp.push_back("-L.");
      temp.push_back("'-Wl,-rpath=$ORIGIN'");
      continue;
    } else if (arg == "-ffast-math") {
      ffast_math = true;
      continue;
    }
    if (arg[0] == '-') {
      if (arg[1] == 'l' && arg[2] == ':') {
        if (arg.size() == 3) {
          die("no file/directory given here", tokens[i].cursor);
        }
        temp.push_back("-l:" + shell::ify(&arg[3]));
      } else if (arg[1] == 'I' || arg[1] == 'L' || arg[1] == 'l') {
        if (arg.size() == 2) {
          die("no file/directory given here", tokens[i].cursor);
        }
        if (arg[1] == 'l') {
          temp.push_back("-l" + shell::ify(lib_basename(&arg[2])));
        } else if (arg[1] == 'L') {
          std::string path(shell::ify(&arg[2]));
          temp.push_back("-L" + path);
          temp.push_back("-Wl,-rpath=" + path);
        } else {
          temp.push_back(std::string("-") + arg[1] + shell::ify(&arg[2]));
        }
      } else if (arg[1] == 'D') {
        if (arg.size() == 2) {
          die("no macro name given here", tokens[i].cursor);
        }
        temp.push_back(arg);
      } else {
        std::map<std::string, std::string>::const_iterator it = args.find(arg);
        if (it != args.end()) {
          if (it->second.size() > 0) {
            temp.push_back(it->second);
          } else if (pi.verbosity >= VERBOSITY_DEBUG) {
            WARNING << "Option " << arg << " is not supported or known by icc"
                    << ", ignoring it" << std::endl;
          }
        } else if (pi.action == parser::infos_t::Permissive) {
          temp.push_back(arg);
        } else {
          die("unknown compiler option", tokens[i].cursor);
        }
      }
    } else {
      temp.push_back(stringify(ns2::sanitize(arg)));
    }
  }

  if (!ffast_math) {
    // Cf. https://scicomp.stackexchange.com/questions/20815
    //     /performance-of-icc-main-cpp-g-ffast-math-main-cpp
    temp.push_back("-fp-model strict");
  }

  // if we have been asked to output include files then do it and tell the
  // the parser that it's GCC specific
  if (pi.generate_header_deps_flags) {
    pi.current_compiler.type = compiler::infos_t::GCC;
    temp.push_back("@@autodeps_flags");
  }

  return ns2::join(uniq(temp), " ");
}

// ----------------------------------------------------------------------------

static std::string single_command(std::vector<parser::token_t> const &tokens,
                                  parser::infos_t *pi_) {
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
    return if_(tokens, &pi);
  } else if (cmd == "cc" || cmd == "c++" || cmd == "msvc" || cmd == "gcc" ||
             cmd == "g++" || cmd == "clang" || cmd == "clang++" ||
             cmd == "mingw" || cmd == "armclang" || cmd == "armclang++" ||
             cmd == "icc") {
    compiler::infos_t ci = compiler::get(cmd, &pi);
    switch (ci.type) {
    case compiler::infos_t::GCC:
    case compiler::infos_t::Clang:
    case compiler::infos_t::ARMClang:
      return gcc_clang(ci.path, tokens, ci, &pi);
      break;
    case compiler::infos_t::MSVC:
      return msvc(tokens, ci, &pi);
      break;
    case compiler::infos_t::ICC:
      return icc(ci.path, tokens, ci, &pi);
      break;
    case compiler::infos_t::None:
      NS2_THROW(std::runtime_error, "Invalid compiler");
      break;
    }
  } else if (pi.action == parser::infos_t::Permissive) {
    return raw(tokens);
  } else {
    die("unknown command", tokens[0].cursor);
  }
  return std::string(); // should never be reached
}

// ----------------------------------------------------------------------------

std::string translate(std::vector<parser::token_t> const &tokens,
                      parser::infos_t *pi_) {
  parser::infos_t &pi = *pi_;
  if (pi.action == parser::infos_t::Raw) {
    return raw(tokens);
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
      ret += single_command(std::vector<parser::token_t>(tokens.begin() + i0,
                                                         tokens.begin() + i1),
                            &pi);

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
      } else if (tokens[i1].text == ";") {
#ifdef NS2_IS_MSVC
        ret += " & ";
#else
        ret += " ; ";
#endif
      } else {
        ret += " " + tokens[i1].text + " ";
      }

      // update i0
      i0 = i1 + 1;
    }
    return ret;
  }
}

// ----------------------------------------------------------------------------

std::string autodeps_flags(compiler::infos_t::type_t type,
                           std::string autodeps_file) {
  if (type == compiler::infos_t::MSVC) {
    return "/showIncludes";
  } else if (type == compiler::infos_t::GCC) {
    return "-MMD -MF " + autodeps_file;
  } else {
    NS2_THROW(std::invalid_argument, "compiler must MSVC or GCC");
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
