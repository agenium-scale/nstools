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

#include <ns2.hpp>

// ----------------------------------------------------------------------------

int test_levenshtein(std::string const &s1, std::string const &s2,
                     int expected_dist) {
  if (ns2::levenshtein(s1, s2) != expected_dist) {
    return 1;
  } else {
    return 0;
  }
}

// ----------------------------------------------------------------------------

int test_levenshtein_sort(size_t n) {
  std::vector<std::string> v;
  for (size_t i = 0; i < n; i++) {
    v.push_back(std::string(i + 1, 'a'));
  }
  std::vector<std::pair<std::string, int> > w = ns2::levenshtein_sort(v, "a");
  for (size_t i = 1; i < n; i++) {
    if (w[i].second != int(i)) {
      return 1;
    }
    if (w[i - 1].second > w[i].second) {
      return 1;
    }
  }
  return 0;
}

// ----------------------------------------------------------------------------

int main() {
  // clang-format off
  return 0
      || test_levenshtein("a", "a", 0)
      || test_levenshtein("a", "b", 1)
      || test_levenshtein("b", "a", 1)
      || test_levenshtein("aa", "a", 1)
      || test_levenshtein("a", "aa", 1)
      || test_levenshtein("abcdefg", "abcdefg", 0)
      || test_levenshtein("abcdefg", "1234567", 7)
      || test_levenshtein("abcdefg", "abc4567", 4)
      || test_levenshtein_sort(2)
      || test_levenshtein_sort(3)
      || test_levenshtein_sort(4)
      || test_levenshtein_sort(5)
      || test_levenshtein_sort(10)
      || test_levenshtein_sort(20)
      ;
  // clang-format on
}
