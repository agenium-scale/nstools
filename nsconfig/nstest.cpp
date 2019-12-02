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

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "thread.hpp"
#include <ns2/fs.hpp>
#include <ns2/process.hpp>

// ----------------------------------------------------------------------------

bool quiet;

// ----------------------------------------------------------------------------

struct thread_output_t {
  std::vector<std::string> fails;
  std::string output;
};

typedef thread::work_t<std::vector<std::string> > work_t;

// ----------------------------------------------------------------------------

thread_output_t thread_start(work_t *work) {
  thread_output_t ret;
  for (;;) {
    std::string const *exe = work->next();
    if (exe == NULL) {
      break;
    }
    if (!quiet) {
      printf("-- Executing %s\n", exe->c_str());
      fflush(stdout);
    }
#ifdef NS2_IS_MSVC
    std::pair<std::string, int> result = ns2::popen((*exe) + " 2>&1");
#else
    std::pair<std::string, int> result;
    if (exe->find('/') == std::string::npos) {
      result = ns2::popen("./" + (*exe) + " 2>&1");
    } else {
      result = ns2::popen((*exe) + " 2>&1");
    }
#endif
    if (!quiet) {
      printf("-- Return code of %s is %d\n", exe->c_str(), result.second);
      fflush(stdout);
    }
    if (result.second != 0) {
      ret.fails.push_back(*exe);
    }
    ret.output += result.first;
  }
  return ret;
}

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
  P("  -q               Quiet run");
  P("  -jN              Run with N threads in parallel");
  P("  --help           Print the current text");
#undef P
}

// ----------------------------------------------------------------------------

int main2(int argc, char **argv) {
  std::vector<std::string> globbed, exes;
  std::string suffix, prefix;
  int i0 = 1;
  int nb_threads = 1;
  quiet = false;

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
    if (!strcmp(argv[i0], "-q")) {
      quiet = true;
      continue;
    }
    if (!memcmp(argv[i0], "-j", 2)) {
      nb_threads = atoi(argv[i0] + 2);
      continue;
    }
    if (!strcmp(argv[i0], "--")) {
      i0++;
      break;
    }
    std::cerr << "";
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
      exes.push_back(prefix + globbed[i] + suffix);
    } else {
      if (!quiet) {
        std::cout << "-- Warning: " << globbed[i]
                  << " is not executable, discarding" << std::endl;
      }
    }
  }

  // Create work + output
  work_t work(exes);
  std::vector<thread_output_t> outputs;

  // Do the work
  thread::pool_work(&outputs, thread_start, nb_threads, &work);

  // Wait for all of them and concatenate the results
  std::vector<std::string> fails;
  std::string output;
  for (size_t i = 0; i < outputs.size(); i++) {
    fails.insert(fails.end(), outputs[i].fails.begin(),
                 outputs[i].fails.end());
    output += "\n" + outputs[i].output;
  }

  // Print out summary
  std::cout << "--" << std::endl
            << "-- OUTPUT:" << std::endl
            << output << "-- SUMMARY: " << fails.size() << " fails out of "
            << exes.size() << " tests" << std::endl;
  for (size_t i = 0; i < fails.size(); i++) {
    std::cout << "-- FAILED: " << fails[i] << std::endl;
  }

  return (fails.size() > 0 ? 1 : 0);
}

// ----------------------------------------------------------------------------

int main(int argc, char **argv) {

  // std::cout << "Attach MSVC...\n";
  // fgetc(stdin);

  try {
    return main2(argc, argv);
  } catch (std::exception const &e) {
    std::cerr << argv[0] << ": error: " << e.what() << std::endl;
    exit(EXIT_FAILURE);
  }
}
