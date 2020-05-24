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

#include <ns2/backtrace.hpp>
#include <ns2/config.hpp>

#include <cstdio>
#include <cstdlib>

#if defined(__GLIBC__) && !defined(_MSC_VER)
#include <cstring>
#include <execinfo.h>
#elif defined(_MSC_VER)
// clang-format off
// The order of the headers cannot be changed: dbghelp.h needs windows.h to
// define some stuff, apparently the former does not include the latter.
#include <windows.h>
#include <dbghelp.h>
// clang-format on
#endif

namespace ns2 {

// ----------------------------------------------------------------------------

// The following magic number is here to avoid doing mallocs. We will most
// likely be called in error context where we do not know the program state.
// So we do the minimum amount of fancy stuff, so no malloc here and 100
// is considered to be more than sufficient. Also no use of C++ container.

#define BACKTRACE_SIZE 100
#define MAX_SYMBOL_LEN 128

// ----------------------------------------------------------------------------

NS_DLLSPEC const char *backtrace() {

#if defined(__GLIBC__) && !defined(_MSC_VER)

  void *pointers[BACKTRACE_SIZE];
  static NS2_TLS char ret[BACKTRACE_SIZE * MAX_SYMBOL_LEN];

  // get stacktrace addresses
  int n = ::backtrace((void **)&pointers, BACKTRACE_SIZE);
  char **symbol_names = ::backtrace_symbols((void **)&pointers, n);
  if (symbol_names == NULL) {
    return "error: cannot determine symbol names";
  }

  // build human-readable stacktrace
  size_t offset = 0;
  for (int i = 0; i < n; i++) {
    size_t symbol_len = strlen(symbol_names[i]);
    if (offset + symbol_len + 1 >= BACKTRACE_SIZE * MAX_SYMBOL_LEN) {
      break;
    }
    strcpy(ret + offset, symbol_names[i]);
    offset += symbol_len;
    ret[offset] = '\n';
    offset++;
  }
  ret[offset - 1] = 0;

  free(symbol_names);
  return ret;

#elif defined(_MSC_VER) && defined(_M_AMD64)

  void *pointers[BACKTRACE_SIZE];
  static NS2_TLS char ret[BACKTRACE_SIZE * MAX_SYMBOL_LEN];

  // get backtrace addresses
  int n = CaptureStackBackTrace(0, BACKTRACE_SIZE, (void **)&pointers, NULL);

  // some initialization for getting symbol names
  HANDLE process = GetCurrentProcess();
  if (SymInitialize(process, NULL, TRUE) == FALSE) {
    return "error: cannot read symbols";
  }

  // symbol struct for Win32 API
  char buf[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
  SYMBOL_INFO *si = (SYMBOL_INFO *)buf;

  // build human-readable stacktrace
  size_t offset = 0;
  for (int i = 0; i < n; i++) {
    si->SizeOfStruct = sizeof(SYMBOL_INFO);
    si->MaxNameLen = MAX_SYM_NAME;
    if (SymFromAddr(process, (DWORD64)pointers[i], 0, si) == FALSE) {
      return "error: cannot get symbol name";
    }
    size_t symbol_len = strlen(si->Name);
    if (offset + symbol_len + 22 >= BACKTRACE_SIZE * MAX_SYMBOL_LEN) {
      // 22 = strlen of ' [0x' + address in hexadecimal + ']\n'
      break;
    }
    strcpy(ret + offset, si->Name);
    offset += symbol_len;
    ret[offset] = ' ';
    offset++;
    ret[offset] = '[';
    offset++;
    int code = sprintf(ret + offset, "%p", (void *)si->Address);
    if (code == -1) {
      return "error: cannot convert symbol address to string";
    }
    offset += code;
    ret[offset] = ']';
    offset++;
    ret[offset] = '\n';
    offset++;
  }
  ret[offset - 1] = 0;

  return ret;

#else

  return "only available on Glibc and Win64 systems";

#endif

} // end of function backtrace()

// ----------------------------------------------------------------------------

} // namespace ns2
