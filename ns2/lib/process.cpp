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

#include <ns2/exception.hpp>
#include <ns2/process.hpp>

#include <cerrno>
#include <cstdio>

namespace ns2 {

std::pair<std::string, int> popen(std::string const &cmd) {
  std::pair<std::string, int> ret;
#ifdef NS2_IS_MSVC
#define POPEN(cmd) ::_popen((cmd), "rb")
#define PCLOSE ::_pclose
#else
#define POPEN(cmd) ::popen((cmd), "r")
#define PCLOSE ::pclose
#endif
  FILE *in = POPEN(cmd.c_str());
  if (in == NULL) {
    NS2_THROW(std::runtime_error,
              "cannot start " + cmd + ": " + strerror(errno));
  }
  char buf[4096];
  for (;;) {
    errno = 0;
    size_t len = fread(buf, 1, 4095, in);
    if (len == 0) {
      // We don't care if fread emits an error, we return what we already have
      break;
    }
    buf[len] = 0;
    try {
      ret.first += buf;
    } catch (std::bad_alloc const &) {
      PCLOSE(in);
      NS2_THROW_BAD_ALLOC();
    } catch (std::length_error const &) {
      PCLOSE(in);
      NS2_THROW_BAD_ALLOC();
    }
  }
  ret.second = PCLOSE(in);
  return ret;
#undef POPEN
#undef PCLOSE
}

} // namespace ns2
