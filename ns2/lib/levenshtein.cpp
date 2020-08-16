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

#include <ns2/config.hpp>
#include <ns2/levenshtein.hpp>

#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace ns2 {

// ----------------------------------------------------------------------------

int levenshtein(std::string const &s1, std::string const &s2) {
  size_t n1 = s1.size();
  size_t n2 = s2.size();
  std::vector<std::vector<int> > m(n1 + 1, std::vector<int>(n2 + 1));

  m[0][0] = 0;
  for (size_t i = 1; i <= n1; ++i) {
    m[i][0] = int(i);
  }
  for (size_t i = 1; i <= n2; ++i) {
    m[0][i] = int(i);
  }

  for (size_t i = 1; i <= n1; ++i) {
    for (size_t j = 1; j <= n2; ++j) {
      m[i][j] = std::min(std::min(m[i - 1][j] + 1, m[i][j - 1] + 1),
                         m[i - 1][j - 1] + (s1[i - 1] == s2[j - 1] ? 0 : 1));
    }
  }
  return m[n1][n2];
}

// ----------------------------------------------------------------------------

static bool compare(std::pair<std::string, int> const &item1,
                    std::pair<std::string, int> const &item2) {
  return item1.second < item2.second;
}

// ----------------------------------------------------------------------------

template <typename Iterator>
std::vector<std::pair<std::string, int> >
levenshtein_sort(Iterator const &begin, Iterator const &end,
                 std::string const &reference) {
  std::vector<std::pair<std::string, int> > ret;
  for (Iterator it = begin; it != end; it++) {
    ret.push_back(
        std::pair<std::string, int>(*it, levenshtein(*it, reference)));
  }
  std::sort(ret.begin(), ret.end(), compare);
  return ret;
}

// ----------------------------------------------------------------------------

std::vector<std::pair<std::string, int> >
levenshtein_sort(std::vector<std::string> const &data,
                 std::string const &reference) {
  return levenshtein_sort(data.begin(), data.end(), reference);
}

// ----------------------------------------------------------------------------

std::vector<std::pair<std::string, int> >
levenshtein_sort(const char **tab, size_t nbelem,
                 std::string const &reference) {
  std::vector<std::string> data(tab, tab + nbelem);
  return levenshtein_sort(data.begin(), data.end(), reference);
}

// ----------------------------------------------------------------------------

std::vector<std::pair<std::string, int> >
levenshtein_sort(std::set<std::string> const &data,
                 std::string const &reference) {
  return levenshtein_sort(data.begin(), data.end(), reference);
}

// ----------------------------------------------------------------------------

} // namespace ns2
