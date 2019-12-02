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
#include <ns2.hpp>

// ----------------------------------------------------------------------------

const char *readfile(const char *filename) {
  static char buf[1024];
  FILE *in = fopen(filename, "rb");
  if (in == NULL) {
    fprintf(stderr, "ERROR: %s: %s\n", filename, strerror(errno));
    return NULL;
  }

  size_t cursor = 0;
  while (!feof(in)) {
    size_t code = fread(buf + cursor, 1, 1024, in);
    if (code == 0) {
      if (errno != 0) {
        fclose(in);
        fprintf(stderr, "ERROR: %s: %s\n", filename, strerror(errno));
        fflush(stdout);
      }
      break;
    }
    cursor += code;
  }

  fclose(in);
  return buf;
}

// ----------------------------------------------------------------------------

int test_string() {
  ns2::ofile_t out("ofile-test.txt");
  out.write("string", 6);
  out.close();
  const char *content = readfile("ofile-test.txt");
  if (content == NULL) {
    return -1;
  }
  if (memcmp(content, "string", 6)) {
    return -1;
  }
  return 0;
}

// ----------------------------------------------------------------------------

int test_decimal() {
  ns2::ofile_t out("ofile-test.txt");
  out << 123456;
  out.close();
  const char *content = readfile("ofile-test.txt");
  if (content == NULL) {
    return -1;
  }
  if (memcmp(content, "123456", 6)) {
    return -1;
  }
  return 0;
}

// ----------------------------------------------------------------------------

int test_hexadecimal() {
  ns2::ofile_t out("ofile-test.txt");
  out << std::hex << 123456;
  out.close();
  const char *content = readfile("ofile-test.txt");
  if (content == NULL) {
    return -1;
  }
  if (memcmp(content, "1e240", 5)) {
    return -1;
  }
  return 0;
}

// ----------------------------------------------------------------------------

int test_tellp() {
  ns2::ofile_t out("ofile-test.txt");
  out.write("The quick brown fox jumps over the lazy dog.", 44);
  if (out.tellp() != 44) {
    return -1;
  }
  out.close();
  const char *content = readfile("ofile-test.txt");
  if (content == NULL) {
    return -1;
  }
  if (memcmp(content, "The quick brown fox jumps over the lazy dog.", 44)) {
    return -1;
  }
  return 0;
}

// ----------------------------------------------------------------------------

int test_is_open() {
  ns2::ofile_t out;
  if (out.is_open()) {
    fprintf(stderr, "File should not be open.\n");
    return -1;
  }
  out.open("ofile-test.txt");
  if (!out.is_open()) {
    fprintf(stderr, "File should be open.\n");
    return -1;
  }
  return 0;
}

// ----------------------------------------------------------------------------

int main() {
  return test_string() || test_decimal() || test_hexadecimal() ||
         test_tellp() || test_is_open();
}
