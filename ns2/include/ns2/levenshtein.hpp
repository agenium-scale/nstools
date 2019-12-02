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

#ifndef LEVENSHTEIN_HPP
#define LEVENSHTEIN_HPP

#include <algorithm>
#include <map>
#include <ns2/config.hpp>
#include <set>
#include <string>
#include <vector>

namespace ns2 {

// ----------------------------------------------------------------------------

NS_DLLSPEC int levenshtein(std::string const &, std::string const &);

NS_DLLSPEC std::vector<std::pair<std::string, int> >
levenshtein_sort(std::vector<std::string> const &, std::string const &);

NS_DLLSPEC std::vector<std::pair<std::string, int> >
levenshtein_sort(std::set<std::string> const &, std::string const &);

NS_DLLSPEC std::vector<std::pair<std::string, int> >
levenshtein_sort(const char **, size_t, std::string const &);

// ----------------------------------------------------------------------------

template <typename T>
std::vector<std::pair<std::string, int> >
levenshtein_sort(std::map<std::string, T> const &map,
                 std::string const &reference) {
  std::vector<std::string> buf;
  for (typename std::map<std::string, T>::const_iterator it = map.begin();
       it != map.end(); it++) {
    buf.push_back(it->first);
  }
  return levenshtein_sort(buf, reference);
}

// ----------------------------------------------------------------------------

} // namespace ns2

#endif
