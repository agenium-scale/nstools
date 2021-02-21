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

#include <ns2/fs.hpp>
#include <ns2/process.hpp>

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <iostream>

#ifdef NS2_IS_MSVC
#include <windows.h>
#else
#include <spawn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#endif

// ----------------------------------------------------------------------------

int verbose;

// ----------------------------------------------------------------------------

void help(FILE *out) {
#define P(msg) fputs(msg "\n", out)
  P("usage: nstest [OPTIONS]... [-- GLOB]...");
  P("Execute files and write a quick summary.");
  P("");
  P("With no GLOB, all executables beginning by 'tests.' are executed.");
  P("");
  P("  --prefix=PREFIX  Prepand executables commands by PREFIX");
  P("  --suffix=SUFFIX  Append executables commands by SUFFIX");
  P("  --dir=DIR        Put all tests outputs in directory DIR");
  P("                   Defaults to 'tests-output'");
  P("  -q               Quiet run");
  P("  -v               Verbose run");
  P("  -jN              Run with N threads in parallel");
  P("  --help           Print the current text");
#undef P
}

// ----------------------------------------------------------------------------

#ifdef NS2_IS_MSVC
typedef HANDLE pid_t;

HANDLE spawn(std::string const &exe, std::string const &output) {
  std::string cmd("cmd.exe /C \"" + exe + " 2>&1 1>" + output + "\"");

  STARTUPINFO si;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);

  PROCESS_INFORMATION pi;
  ZeroMemory(&pi, sizeof(pi));

  if (CreateProcessA(NULL, cmd.c_str(), NULL, NULL, FALSE, CREATE_NO_WINDOW,
                     NULL, NULL, &si, &pi) == FALSE) {
    return -1;
  }
  CloseHandle(pi.hThread); // We don't nedd the handle of the main thread
  return pi.hProcess;
}
#else
extern char **environ;

pid_t spawn(std::string const &exe, std::string const &output) {
  std::vector<char> cmd(exe.size() + 1);
  memcpy((void *)&cmd[0], (void *)exe.c_str(), exe.size() + 1);
  char buf[] = "/bin/sh\0-c\0";
  char *argv[4];
  argv[0] = &buf[0];
  argv[1] = &buf[8];
  argv[2] = &cmd[0];
  argv[3] = NULL;
  pid_t pid;
  posix_spawn_file_actions_t actions;
  if (posix_spawn_file_actions_init(&actions) != 0) {
    return -1;
  }
  pid = -1;
  if (posix_spawn_file_actions_addopen(&actions, 1, output.c_str(),
                                       O_TRUNC | O_CREAT | O_WRONLY,
                                       0644) == 0 &&
      posix_spawn_file_actions_addopen(&actions, 2, output.c_str(),
                                       O_TRUNC | O_CREAT | O_WRONLY,
                                       0644) == 0 &&
      posix_spawn_file_actions_addclose(&actions, 0) == 0 &&
      posix_spawn(&pid, "/bin/sh", &actions, NULL, argv, environ) != 0) {
    pid = -1;
  }
  posix_spawn_file_actions_destroy(&actions);
  return pid;
}
#endif

// ----------------------------------------------------------------------------

#ifdef NS2_IS_MSVC
pid_t wait_for_children(std::vector<pid_t> const &handles) {
  std::vector<pid_t> list;
  for (size_t i = 0; i < handles.size(); i++) {
    if (handles[i] != -1) {
      list.push_back(handles[i]);
    }
  }
  DWORD code =
      WaitForMultipleObjects((DWORD)list.size(), &list[0], FALSE, INFINITE);
  if (code == WAIT_FAILED) {
    NS2_THROW(std::runtime_error, "error waiting for children: " +
                                      std::string(strerror(GetLastError())));
  }
  HANDLE hProcess = list[code - WAIT_OBJECT_0];
  DWORD ExitCode;
  if (GetExitCodeProcess(hProcess, &ExitCode) == FALSE) {
    NS2_THROW(std::runtime_error, "error getting exit code: " +
                                      std::string(strerror(GetLastError())));
  }
  CloseHandle(hProcess);
  return ExitCode == 0 ? hProcess : -hProcess;
}
#else
pid_t wait_for_children(std::vector<pid_t> const &) {
  int status;
  pid_t pid = wait(&status);
  if (pid == -1) {
    NS2_THROW(std::runtime_error,
              "error waiting for children: " + std::string(strerror(errno)));
  }
  return WEXITSTATUS(status) == 0 ? pid : -pid;
}
#endif

// ----------------------------------------------------------------------------

int main2(int argc, char **argv) {
  std::vector<std::string> globbed, exes;
  std::string suffix, prefix, directory;
  int i0 = 1;
  size_t nb_threads = 1;
  verbose = 1;

  // Special case of 0 argument
  if (argc == 1) {
    help(stdout);
    fflush(stdout);
    return 0;
  }

  // Parse arguments
  for (i0 = 1; i0 < argc; i0++) {
    if (!strcmp(argv[i0], "--help")) {
      help(stdout);
      fflush(stdout);
      return 0;
    }
    if (!memcmp(argv[i0], "--prefix=", 9)) {
      prefix = std::string(argv[i0] + 9) + " ";
      continue;
    }
    if (!memcmp(argv[i0], "--suffix=", 9)) {
      suffix = " " + std::string(argv[i0] + 9);
      continue;
    }
    if (!memcmp(argv[i0], "--dir=", 6)) {
      directory = std::string(argv[i0] + 6);
      continue;
    }
    if (!strcmp(argv[i0], "-q")) {
      verbose = 0;
      continue;
    }
    if (!strcmp(argv[i0], "-v")) {
      verbose = 2;
      continue;
    }
    if (!memcmp(argv[i0], "-j", 2)) {
      nb_threads = size_t(atoi(argv[i0] + 2));
      continue;
    }
    if (!strcmp(argv[i0], "--")) {
      i0++;
      break;
    }
    NS2_THROW(std::runtime_error,
              "unknown argument: " + std::string(argv[i0]));
    return -1;
  }

  // Glob all test executables
  if (i0 >= argc) {
    globbed = ns2::glob("tests.*");
  } else {
    for (int i = i0; i < argc; i++) {
      std::vector<std::string> temp = ns2::glob(argv[i]);
      globbed.insert(globbed.end(), temp.begin(), temp.end());
    }
  }

  // Keep only those that are executables
  for (size_t i = 0; i < globbed.size(); i++) {
    if (ns2::isexe(globbed[i])) {
#ifdef NS2_IS_MSVC
      exes.push_back(prefix + globbed[i] + suffix);
#else
      if (globbed[i].find('/') == std::string::npos) {
        exes.push_back(prefix + "./" + globbed[i] + suffix);
      } else {
        exes.push_back(prefix + globbed[i] + suffix);
      }
#endif
    } else {
      if (verbose >= 2) {
        std::cout << "-- Warning: " << globbed[i]
                  << " is not executable, discarding" << std::endl;
      }
    }
  }

  // Create directory for tests outputs
  if (directory.size() == 0) {
    directory = "tests-output";
  }
  ns2::mkdir(directory);

  // Execute all tests
  std::vector<pid_t> children(nb_threads, (pid_t)-1);
  std::vector<size_t> ids(nb_threads, 0);
  std::vector<std::string> fails;
  for (size_t i = 0;;) {
    bool has_children = false;
    for (size_t j = 0; j < nb_threads && i < exes.size();) {
      if (children[j] != -1) {
        has_children = true;
        j++;
        continue;
      }
      if (children[j] == -1) {
        children[j] = spawn(
            exes[i], ns2::join_path(directory, ns2::to_string(i) + ".txt"));
        std::cout << "-- [" << i << "/" << exes.size() << "] exec: " << exes[i]
                  << '\n';
      }
      if (children[j] == -1) {
        fails.push_back(exes[i] + ", cannot exec");
        continue;
      }
      ids[j] = i;
      has_children = true;
      i++;
      j++;
    }
    if (!has_children) {
      break;
    }
    pid_t pid = wait_for_children(children);
    bool failed = false;
    if (pid < 0) {
      failed = true;
      pid = -pid;
    }
    for (size_t j = 0; j < children.size(); j++) {
      if (pid == children[j]) {
        children[j] = -1;
        if (failed) {
          fails.push_back(
              exes[ids[j]] + ", output in " +
              ns2::join_path(directory, ns2::to_string(ids[j]) + ".txt"));
        }
      }
    }
  }

  // Print out summary
  std::cout << "--\n";
  std::cout << "-- SUMMARY: " << fails.size() << " fails out of "
            << exes.size() << " tests\n";
  if (verbose >= 1) {
    for (size_t i = 0; i < fails.size(); i++) {
      std::cout << "-- FAILED: " << fails[i] << std::endl;
    }
  }

  return (fails.size() > 0 ? 1 : 0);
}

// ----------------------------------------------------------------------------

int main(int argc, char **argv) {
  if (getenv("DEBUGGING_NSTEST") != NULL) {
    return main2(argc, argv);
  } else {
    try {
      return main2(argc, argv);
    } catch (std::exception const &e) {
      std::cerr << argv[0] << ": error: " << e.what() << std::endl;
      exit(EXIT_FAILURE);
    }
  }
}
