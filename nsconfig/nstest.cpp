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
const HANDLE INVALID_PID = NULL;

HANDLE spawn(std::string const &exe, std::string const &output) {
  std::string cmd("cmd.exe /C \"" + exe + "\"");
  std::vector<char> buf(cmd.size() + 1);
  memcpy((void *)&buf[0], (void *)cmd.c_str(), cmd.size() + 1);
  HANDLE ret = INVALID_PID;

  STARTUPINFO si;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  si.dwFlags = STARTF_USESTDHANDLES;

  SECURITY_ATTRIBUTES sa;
  ZeroMemory(&sa, sizeof(sa));
  sa.nLength = sizeof(sa);
  sa.bInheritHandle = TRUE;

  si.hStdOutput = CreateFile(output.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE,
                             &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (si.hStdOutput == INVALID_HANDLE_VALUE) {
    return INVALID_PID;
  }

  si.hStdError = CreateFile(output.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE,
                            &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (si.hStdError == INVALID_HANDLE_VALUE) {
    goto free_hStdOutput;
  }

  PROCESS_INFORMATION pi;
  ZeroMemory(&pi, sizeof(pi));

  if (CreateProcessA(NULL, &buf[0], NULL, NULL, TRUE, CREATE_NO_WINDOW,
                     NULL, NULL, &si, &pi) == FALSE) {
    goto free_hStdError;
  }
  CloseHandle(pi.hThread); // We don't nedd the handle of the main thread
  ret = pi.hProcess;

free_hStdError:
  CloseHandle(si.hStdError);

free_hStdOutput:
  CloseHandle(si.hStdOutput);

  return ret;
}
#else
extern char **environ;
const pid_t INVALID_PID = -1;

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
    return INVALID_PID;
  }
  pid = INVALID_PID;
  if (posix_spawn_file_actions_addopen(&actions, 1, output.c_str(),
                                       O_TRUNC | O_CREAT | O_WRONLY,
                                       0644) == 0 &&
      posix_spawn_file_actions_addopen(&actions, 2, output.c_str(),
                                       O_TRUNC | O_CREAT | O_WRONLY,
                                       0644) == 0 &&
      posix_spawn_file_actions_addclose(&actions, 0) == 0 &&
      posix_spawn(&pid, "/bin/sh", &actions, NULL, argv, environ) != 0) {
    pid = INVALID_PID;
  }
  posix_spawn_file_actions_destroy(&actions);
  return pid;
}
#endif

// ----------------------------------------------------------------------------

#ifdef NS2_IS_MSVC
// Note that on Windows, one cannot wait for more than MAXIMUM_WAIT_OBJECTS
// handles. In practice MAXIMUM_WAIT_OBJECTS is 64 or 128. This is not much
// and prevents in our case the use of say more 64 cores which is not that
// uncommon in today's machines. One way to get rid of this limit is to
// spawn threads that will each wait for 64 handles in a tree manner so that
// the main thread wait for the spawned threads. Well that's a lot of code
// and a lot of overhead just to get around this limitation and for now we
// don't need it so we have not implemented it.
std::pair<pid_t, bool> wait_for_children(std::vector<pid_t> const &handles) {
  std::vector<pid_t> list;
  for (size_t i = 0; i < handles.size(); i++) {
    if (handles[i] != INVALID_PID) {
      list.push_back(handles[i]);
    }
  }
  if (list.size() == 0) {
    return std::pair<pid_t, bool>(INVALID_PID, true);
  }
  DWORD code =
      WaitForMultipleObjects((DWORD)list.size(), &list[0], FALSE, INFINITE);
  if (code == WAIT_FAILED) {
    NS2_THROW(std::runtime_error, "error waiting for children: " +
                                  std::string(ns2::get_last_system_error()));
  }
  HANDLE hProcess = list[code - WAIT_OBJECT_0];
  DWORD ExitCode;
  if (GetExitCodeProcess(hProcess, &ExitCode) == FALSE) {
    NS2_THROW(std::runtime_error, "error getting exit code: " +
                                  std::string(ns2::get_last_system_error()));
  }
  CloseHandle(hProcess);
  return std::pair<pid_t, bool>(hProcess, ExitCode == 0 ? true : false);
}
#else
std::pair<pid_t, bool> wait_for_children(std::vector<pid_t> const &) {
  int status;
  pid_t pid = wait(&status);
  if (errno == ECHILD) {
    return std::pair<pid_t, bool>(INVALID_PID, true);
  }
  if (pid == -1) {
    NS2_THROW(std::runtime_error,
              "error waiting for children: " + std::string(strerror(errno)));
  }
  return std::pair<pid_t, bool>(pid, WEXITSTATUS(status) == 0 ? true : false);
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
#ifdef NS2_IS_MSVC
      if (nb_threads > MAXIMUM_WAIT_OBJECTS) {
        NS2_THROW(std::runtime_error, "on windows, we cannot have more than "
        + ns2::to_string(MAXIMUM_WAIT_OBJECTS) + " jobs");
      }
#endif
      continue;
    }
    if (!strcmp(argv[i0], "--")) {
      i0++;
      break;
    }
    NS2_THROW(std::runtime_error,
              "unknown command line argument: " + std::string(argv[i0]));
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
  std::vector<pid_t> children(nb_threads, INVALID_PID);
  std::vector<size_t> ids(nb_threads);
  std::vector<std::string> fails;
  for (size_t i = 0;;) {
    for (size_t j = 0; j < nb_threads && i < exes.size();) {
      if (children[j] != INVALID_PID) {
        j++;
        continue;
      }
      if (children[j] == INVALID_PID) {
        children[j] =
            spawn(exes[i],
                  ns2::join_path(directory, ns2::to_string(i + 1) + ".txt"));
        std::cout << "-- [" << (i + 1) << "/" << exes.size()
                  << "] exec: " << exes[i] << '\n';
      }
      if (children[j] == INVALID_PID) {
        fails.push_back(exes[i] + ", cannot exec: " +
                          ns2::get_last_system_error());
        continue;
      }
      ids[j] = i;
      i++;
      j++;
    }
    std::pair<pid_t, bool> code = wait_for_children(children);
    if (code.first == INVALID_PID) {
      break;
    }
    for (size_t j = 0; j < children.size(); j++) {
      if (code.first == children[j]) {
        children[j] = INVALID_PID;
        if (code.second == false) {
          fails.push_back(
              exes[ids[j]] + ", output in " +
              ns2::join_path(directory, ns2::to_string(ids[j] + 1) + ".txt"));
        }
        break;
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
