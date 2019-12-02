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
#include <ns2/string.hpp>

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace ns2 {

// ----------------------------------------------------------------------------

std::vector<std::string> split(std::string const &str, char sep) {
  std::vector<std::string> ret;
  std::string buf;
  for (size_t i = 0; i < str.size(); i++) {
    if (str[i] == sep) {
      ret.push_back(buf);
      buf.clear();
      continue;
    } else {
      buf += str[i];
    }
  }
  ret.push_back(buf);
  return ret;
}

std::vector<std::string> split(std::string const &str,
                               std::string const &sep) {
  std::vector<std::string> ret;
  std::string buf;
  for (size_t i = 0; i < str.size(); i++) {
    if (str.find(sep, i) == i) {
      ret.push_back(buf);
      buf.clear();
      i += sep.size() - 1;
      continue;
    } else {
      buf += str[i];
    }
  }
  ret.push_back(buf);
  return ret;
}

// ----------------------------------------------------------------------------

bool is_blank(char c) {
  return c == ' ' || c == '\n' || c == '\t' || c == '\r';
}

// ----------------------------------------------------------------------------

std::string strip(std::string const &str) {
  if (str.size() == 0) {
    return std::string();
  }
  size_t i0, i1;
  for (i0 = 0; i0 < str.size() && is_blank(str[i0]); i0++)
    ;
  for (i1 = str.size() - 1; i1 != 0 && is_blank(str[i1]); i1--)
    ;
  if (i0 > i1) {
    return std::string();
  }
  return std::string(str, i0, i1 - i0 + 1);
}

// ----------------------------------------------------------------------------

bool startswith(std::string const &haystack, std::string const &needle) {
  if (needle.size() > haystack.size()) {
    return false;
  }
  return needle == std::string(haystack, 0, needle.size());
}

// ----------------------------------------------------------------------------

bool endswith(std::string const &haystack, std::string const &needle) {
  if (needle.size() > haystack.size()) {
    return false;
  }
  return needle == std::string(haystack, haystack.size() - needle.size(),
                               haystack.size());
}

// ----------------------------------------------------------------------------

std::string replace(std::string const &str, char ol, char ne) {
  std::string ret(str);
  for (size_t i = 0; i < str.size(); i++) {
    if (ret[i] == ol) {
      ret[i] = ne;
    }
  }
  return ret;
}

// ----------------------------------------------------------------------------

std::string replace(std::string const &str, std::string const &needle,
                    std::string const &by) {
  std::string ret;
  size_t len = needle.size();
  size_t limit = str.size() - len;
  for (size_t i = 0; i < str.size();) {
    if (i <= limit && !memcmp(needle.c_str(), &str[i], len)) {
      ret += by;
      i += len;
    } else {
      ret += str[i];
      i++;
    }
  }
  return ret;
}

// ----------------------------------------------------------------------------

std::string join(std::vector<std::string> const &vec, std::string const &sep) {
  if (vec.size() == 0) {
    return std::string();
  }
  std::string ret(vec[0]);
  for (size_t i = 1; i < vec.size(); i++) {
    ret += sep;
    ret += vec[i];
  }
  return ret;
}

// ----------------------------------------------------------------------------

std::string lower(std::string const &str) {
  std::string ret;
  for (size_t i = 0; i < str.size(); i++) {
    ret += char(std::tolower(str[i]));
  }
  return ret;
}

// ----------------------------------------------------------------------------

std::string leading_whitespace(std::string const &s) {
  std::string r;
  for (size_t i = 0; i < s.size(); ++i) {
    if (ns2::is_blank(s[i])) {
      r += s[i];
    } else {
      break;
    }
  }
  return r;
}

std::string indent_finder(std::string const &s) {
  if (s == "") {
    return "";
  }
  std::vector<std::string> const lines = ns2::split(s, '\n');
  std::string leading_whitespace = ns2::leading_whitespace(lines[0]);
  for (size_t i = 1; i < lines.size(); ++i) {
    if (leading_whitespace.size() == 0) {
      return "";
    }
    std::string const tmp = ns2::leading_whitespace(lines[i]);
    size_t j_size = leading_whitespace.size();
    if (j_size > tmp.size()) {
      j_size = tmp.size();
      leading_whitespace = leading_whitespace.substr(0, j_size);
    }
    for (size_t j = 0; j < j_size; ++j) {
      if (tmp[j] != leading_whitespace[j]) {
        leading_whitespace = leading_whitespace.substr(0, j);
        break;
      }
    }
  }
  return leading_whitespace;
}

std::string deindent(std::string const &s) {
  if (s == "") {
    return "";
  }
  std::string r;
  size_t const leading_whitespace_size = indent_finder(s).size();
  std::vector<std::string> const lines = ns2::split(s, '\n');
  for (size_t i = 0; i < lines.size(); ++i) {
    if (lines[i].size() >= leading_whitespace_size) {
      r += lines[i].substr(leading_whitespace_size);
    } else {
      r += lines[i];
    }
    if (i != lines.size() - 1) {
      r += '\n';
    }
  }
  return r;
}

// ----------------------------------------------------------------------------

} // namespace ns2
