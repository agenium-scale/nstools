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

#include "backend_ninja.hpp"
#include "nsconfig.hpp"
#include "compiler.hpp"
#include "shell.hpp"
#include <ctime>
#include <cstdlib>
#include <iostream>
#include <ns2/fs.hpp>
#include <ns2/string.hpp>
#include <ns2/process.hpp>
#ifndef NS2_IS_MSVC
#include <unistd.h>
#else
#include <algorithm>
#include <windows.h>
#endif

namespace backend {

// ----------------------------------------------------------------------------

static size_t ninja_cmds_len(rule_desc_t const &rule_desc) {
  size_t ret = 0;
#ifdef NS2_IS_MSVC
  const size_t command_overhead = 8;
  const size_t constant_overhead = 9;
#else
  const size_t command_overhead = 4;
  const size_t constant_overhead = 0;
#endif
  for (size_t i = 0; i < rule_desc.cmds.size(); i++) {
    ret += rule_desc.cmds[i].size() + command_overhead;
  }
  return ret + constant_overhead;
}

// ----------------------------------------------------------------------------

static size_t shell_cmd_max_len() {
  // We return the value as found in the documentation of the OS minus 1 to
  // avoid the caller to take into account the terminated null character
#ifdef NS2_IS_MSVC
  // On Windows, ninja calls CreateProcess (cf. "Rule variables" at
  // https://ninja-build.org/manual.html) and the limitation is 32768, it
  // is given at [https://docs.microsoft.com/en-us/windows/win32/api/
  // processthreadsapi/nf-processthreadsapi-createprocessa]. But we call
  // cmd.exe with the command as its argument which is subject to other
  // limitations [https://support.microsoft.com/en-us/help/830473/
  // command-prompt-cmd-exe-command-line-string-limitation], namely 8191 on
  // Windows XP and later. As we do not support earlier Windows system we
  // keep 8191 as limitation.
  return size_t(8191) - size_t(1);
#elif defined(NS2_IS_MACOS) || defined(NS2_IS_BSD)
  // On BSDs and MacOS it is simple, there is a syscall that does exactly
  // what we want.
  return size_t(sysconf(_SC_ARG_MAX)) - size_t(1);
#else
  // On Linux, the situation is no better than Windows. Ninja calls
  // posix_spawn(3) which in turn calls execve(2). A read at execve(2) seems
  // to indicate that the lower limit is = 32 * (size of a memory page) in any
  // case except two versions of the Linux kernel: 2.6.23 and 2.6.24 which we
  // ignore.
  return size_t(32) * sysconf(_SC_PAGESIZE) - size_t(1);
#endif
}

// ----------------------------------------------------------------------------

static void ninja_output_rule(rule_desc_t const &rule_desc,
                              ns2::ofile_t &out) {
  std::vector<std::string> cmds(rule_desc.cmds);
  out << "# ---\n\n";

  if (cmds.size() == 0) {
    // we have a ninja phony rule
    out << "build " << rule_desc.target << ": phony";
    for (size_t i = 0; i < rule_desc.deps.size(); i++) {
      out << " " << rule_desc.deps[i];
    }
    out << "\n\n";
    return;
  }

  // check if we have a path as target
  ns2::dir_file_t df = ns2::split_path(rule_desc.output);
  if (rule_desc.type != rule_desc_t::Phony && cmds.size() > 0 &&
      df.first != "") {
    std::string folder(shell::ify(df.first));
#ifdef NS2_IS_MSVC
    cmds.insert(cmds.begin(), "if not exist " + folder + " md " + folder);
#else
    cmds.insert(cmds.begin(), "mkdir -p " + df.first);
#endif
  }

  // Replace $ with $$
  for (size_t i = 0; i < cmds.size(); ++i) {
    cmds[i] = ns2::replace(cmds[i], "$", "$$");
  }

  // getting here means that we have a "standard" rule
  size_t total_len = ninja_cmds_len(rule_desc);
  static size_t max_total_len = shell_cmd_max_len();
#ifdef NS2_IS_MSVC
  std::string rule_name(ns2::replace(rule_desc.target, "\\", "."));
  out << "rule " << rule_name << "_rule\n";
  if (total_len >= max_total_len) {
    // Create script
    ns2::mkdir(NINJA_BUILD_SCRIPT);
    std::string script_filename(
        ns2::join_path(NINJA_BUILD_SCRIPT, rule_name + "_rule.bat"));
    ns2::ofile_t script(script_filename);
    script << "@echo on\n\n";
    for (size_t i = 0; i < cmds.size(); i++) {
      script << cmds[i] << "\n"
             << "if %errorlevel% neq 0 exit /B %errorlevel%\n\n";
    }
    // Add ninja entry
    out << "  command = cmd /C " << ns2::sanitize(script_filename);
  } else {
    if (cmds.size() > 1) {
      out << "  command = cmd /c ( " << cmds[0];
      for (size_t i = 1; i < cmds.size(); i++) {
        out << " ) && ( " << cmds[i];
      }
      out << " )";
    } else {
      out << "  command = " << cmds[0];
    }
  }
#else
  std::string rule_name(ns2::replace(rule_desc.target, "/", "."));
  if (total_len >= max_total_len) {
    // Create script
    ns2::mkdir(NINJA_BUILD_SCRIPT);
    std::string script_filename(
        ns2::join_path(NINJA_BUILD_SCRIPT, rule_name + "_rule.sh"));
    ns2::ofile_t script(script_filename);
    script << "#!/bin/bash\n\n"
           << "set -e\n"
           << "set -x\n\n";
    for (size_t i = 0; i < cmds.size(); i++) {
      script << cmds[i] << "\n\n";
    }
    script.close();
    // Add ninja entry
    out << "rule " << rule_name << "_rule\n"
        << "  command = sh " << script_filename;
  } else {
    out << "rule " << rule_name << "_rule\n"
        << "  command = " << cmds[0];
    for (size_t i = 1; i < cmds.size(); i++) {
      out << " && " << cmds[i];
    }
  }
#endif
  if (rule_desc.type == rule_desc_t::SelfGenerate) {
    out << "\n  generator = 1";
  }
  if (rule_desc.autodeps) {
    if (rule_desc.autodeps_by == compiler::infos_t::MSVC) {
      out << "\n  deps = msvc";
    } else if (rule_desc.autodeps_by == compiler::infos_t::GCC ||
               rule_desc.autodeps_by == compiler::infos_t::Clang ||
               rule_desc.autodeps_by == compiler::infos_t::ICC) {
      out << "\n  deps = gcc";
      out << "\n  depfile = " << rule_desc.autodeps_file;
    } else {
      NS2_THROW(std::invalid_argument, "unsupported compiler in autodeps_by");
    }
  }

  out << "\n\nbuild " << rule_desc.target << ": " << rule_name << "_rule";
  for (size_t i = 0; i < rule_desc.deps.size(); i++) {
    out << " " << shell::ify(ns2::replace(rule_desc.deps[i], ":", "$:"));
  }
  out << "\n\n";
}

// ----------------------------------------------------------------------------

static std::string get_msvc_deps_prefix(std::string const &path) {
  ns2::mkdir(COMPILER_INFOS_DIR);
  std::string header("4294967291.hpp");
  std::string src("msvc_deps_prefix.cpp");
  ns2::write_file(ns2::join_path(COMPILER_INFOS_DIR, header), "#define FOO\n");
  ns2::write_file(ns2::join_path(COMPILER_INFOS_DIR, src),
                  "#include \"" + header + "\"\nint main() {return 0;}");
  std::pair<std::string, int> code = ns2::popen(std::string("cd \"") +
                                                COMPILER_INFOS_DIR +
                                                "\" & " + path +
                                                " /nologo /showIncludes " +
                                                src);
  if (code.second == 0) {
    std::vector<std::string> lines = ns2::split(code.first, '\n');
    for (size_t i = 0; i < lines.size(); i++) {
      if (lines[i].find(header) != std::string::npos) {
        size_t sep = lines[i].find(':');
        if (sep == std::string::npos) {
          break;
        }
        sep = lines[i].find(':', sep + 1);
        if (sep == std::string::npos) {
          break;
        }
        return std::string(lines[i], 0, sep + 1);
      }
    }
  }
  OUTPUT << "Cannot get MSVC prefix when /showIncludes" << std::endl;
  exit(EXIT_FAILURE);
  return std::string(); // Should never be reached
}

// ----------------------------------------------------------------------------

void ninja(rules_t const &rules, std::string const &ninja_file,
           std::string const &cmdline, std::string const &msvc_path) {
  ns2::ofile_t out(ninja_file);
  char buf[256];
  time_t now = time(NULL);
  strftime(buf, 256, "%Y/%m/%d %H:%M:%S", localtime(&now));
  out << "#\n"
      << "# File generated by nsconfig on " << buf << "\n"
      << "# Command line: " << cmdline << "\n"
      << "#\n\n";

  if (msvc_path.size() > 0) {
    out << "msvc_deps_prefix = " << get_msvc_deps_prefix(msvc_path) << "\n\n";
  }

  // Default rule has to be specified for ninja
  rule_desc_t const *rd = rules.find_by_target("all");
  if (rd != NULL) {
    ninja_output_rule(*rd, out);
    out << "default all\n\n";
  }

  // Dump all rules
  for (rules_t::const_iterator it = rules.begin(); it != rules.end(); ++it) {
    if (it->first != "all" && it->first != "install" && it->first != "clean") {
      ninja_output_rule(it->second, out);
    }
  }

  // Last rules are install and clean
  rd = rules.find_by_target("install");
  if (rd != NULL) {
    ninja_output_rule(*rd, out);
  }
  rd = rules.find_by_target("clean");
  if (rd != NULL) {
    ninja_output_rule(*rd, out);
  }

  OUTPUT << "Ninja build file written to " << ninja_file << std::endl;
}

// ----------------------------------------------------------------------------

} // namespace backend
