// MIT License
//
// Copyright (c) 2021 Agenium Scale
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

#include "lambda.hpp"
#include <iostream>

namespace lambda {

// ----------------------------------------------------------------------------

static size_t nb_args(lambda_t const &l) {
  return l.pieces.size() + (l.first_at_begin ? 1 : 0) +
         (l.last_at_end ? 1 : 0);
}

// ----------------------------------------------------------------------------

static bool match(lambda_t const &l, std::string const &str) {
  std::vector<size_t> pos;
  for (size_t i = 0; i < l.pieces.size(); i++) {
    pos.push_back(
        str.find(l.pieces[i],
                 (pos.size() == 0 ? 0 : pos.back() + l.pieces[i - 1].size())));
    if (pos.back() == std::string::npos) {
      return false;
    }
  }
  if (pos[0] != 0 && l.first_at_begin) {
    return false;
  }
  if ((pos.back() + l.pieces.back().size()) != str.size() && l.last_at_end) {
    return false;
  }
  return true;
}

// ----------------------------------------------------------------------------

//static void print(lambda_t const l) {
//  for (size_t i = 0; i < l.pieces.size(); i++) {
//    std::cout << l.pieces[i] << ' ';
//  }
//  std::cout << "| " << l.value << " | " << l.first_at_begin << " | "
//            << l.last_at_end << std::endl;
//}

// ----------------------------------------------------------------------------

std::pair<bool, std::string> find(std::string const &str,
                                  std::vector<lambda_t> const &vec) {
  size_t i0 = (size_t)-1;
  size_t nb_args_min = (size_t)-1;
  for (size_t i = 0; i < vec.size(); i++) {
    size_t curr_nb_args = nb_args(vec[i]);
    if (match(vec[i], str) && curr_nb_args < nb_args_min) {
      i0 = i;
      nb_args_min = curr_nb_args;
    }
  }
  if (i0 != (size_t)-1) {
    return std::pair<bool, std::string>(true, vec[i0].value);
  }
  return std::pair<bool, std::string>(false, std::string());
}

// ----------------------------------------------------------------------------

lambda_t create(std::string const &str, std::string const &value) {
  lambda_t ret;
  std::string buf;
  for (size_t i = 0; i < str.size(); i++) {
    if (str[i] == '*') {
      if (buf.size() > 0) {
        ret.pieces.push_back(buf);
      }
      buf.clear();
      continue;
    }
    buf.push_back(str[i]);
  }
  ret.first_at_begin = (str[0] != '*');
  ret.last_at_end = (str[str.size() - 1] != '*');
  ret.value = value;
  return ret;
}

// ----------------------------------------------------------------------------

bool cmp(lambda_t const &l1, lambda_t const &l2) {
  if (l1.first_at_begin != l2.first_at_begin) {
    return false;
  }
  if (l1.last_at_end != l2.last_at_end) {
    return false;
  }
  if (l1.pieces.size() != l2.pieces.size()) {
    return false;
  }
  for (size_t i = 0; i < l1.pieces.size(); i++) {
    if (l1.pieces[i] != l2.pieces[i]) {
      return false;
    }
  }
  return true;
}

// ----------------------------------------------------------------------------

} // namespace lambda
