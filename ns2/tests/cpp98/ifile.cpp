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
#include <ns2.hpp>
#include <string>

// ----------------------------------------------------------------------------

void writefile(const char *content) {
  FILE *out = fopen("ifile-test.txt", "wb");
  if (out == NULL) {
    fprintf(stderr, "ERROR: ifile-test.txt: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  if (fputs(content, out) < 0) {
    fclose(out);
    fprintf(stderr, "ERROR: ifile-test.txt: %s\n", strerror(errno));
    fflush(stdout);
    exit(EXIT_FAILURE);
  }

  fclose(out);
}

// ----------------------------------------------------------------------------

int test_string() {
  writefile("string");
  ns2::ifile_t in("ifile-test.txt");
  std::string str;
  in >> str;
  if (memcmp(str.c_str(), "string", 6)) {
    return -1;
  }
  return 0;
}

// ----------------------------------------------------------------------------

int test_decimal() {
  writefile("123456");
  ns2::ifile_t in("ifile-test.txt");
  int i;
  in >> i;
  if (i != 123456) {
    return -1;
  }
  return 0;
}

// ----------------------------------------------------------------------------

int test_hexadecimal() {
  writefile("1e240");
  ns2::ifile_t in("ifile-test.txt");
  int i;
  in >> std::hex >> i;
  if (i != 123456) {
    fprintf(stdout, "DEBUG: dec=%d  hex=%X\n", i, (unsigned int)i);
    return -1;
  }
  return 0;
}

// ----------------------------------------------------------------------------

int test_is_open() {
  writefile("1e240");
  ns2::ifile_t in;
  if (in.is_open()) {
    fprintf(stdout, "DEBUG: file should not be open.\n");
    return -1;
  }
  in.open("ifile-test.txt");
  if (!in.is_open()) {
    fprintf(stdout, "DEBUG: file should be open.\n");
    return -1;
  }
  return 0;
}

// ----------------------------------------------------------------------------

int main() {
  return test_string() || test_decimal() || test_hexadecimal() ||
         test_is_open();
}
