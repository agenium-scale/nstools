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
#ifdef NS2_IS_MSVC
#include <windows.h>
#else
#include <cerrno>
#include <cstring>
#endif

namespace ns2 {

// ----------------------------------------------------------------------------

#ifdef NS2_IS_MSVC
NS_DLLSPEC const char *win_strerror(DWORD code) {
  __declspec(thread) static char buf[2048];

  if (FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM |
                         FORMAT_MESSAGE_IGNORE_INSERTS,
                     NULL, code, 0, buf, 2048, NULL) == 0) {
    sprintf(buf, "cannot get error string for error code %u (%X)", code, code);
    return buf;
  }

  return buf;
}
#endif

// ----------------------------------------------------------------------------

NS_DLLSPEC const char *get_last_system_error(void) {
#ifdef NS2_IS_MSVC
  return win_strerror(GetLastError());
#else
  return strerror(errno);
#endif
}

// ----------------------------------------------------------------------------

} // namespace ns2
