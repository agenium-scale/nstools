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

#ifndef NS2_HTML_HPP
#define NS2_HTML_HPP

#include <ns2.hpp>

#include <string>
#include <utility>
#include <vector>

namespace ns2 {

// ----------------------------------------------------------------------------

NS_DLLSPEC std::string
begin_html_tag(std::string const &,
               std::vector<std::pair<std::string, std::string> > const &attrs =
                   std::vector<std::pair<std::string, std::string> >());
NS_DLLSPEC std::string end_html_tag(std::string const &);
NS_DLLSPEC std::string htmlize(std::string const &);
NS_DLLSPEC std::string html_href(std::string const &, std::string const &);
NS_DLLSPEC std::string html_img(std::string const &, std::string const &);
NS_DLLSPEC std::string
html_tr_with_alignments(std::vector<std::string> const &,
                        std::vector<text_align_t> const &);

// ----------------------------------------------------------------------------

inline std::string htmlize(char c) {
  switch (c) {
  case '&':
    return "&amp;";
  case '<':
    return "&lt;";
  case '>':
    return "&gt;";
  case '"':
    return "&quot;";
  case '\'':
    return "&apos;";
  default:
    return std::string(1, c);
  }
}

// ----------------------------------------------------------------------------

} // namespace ns2

#endif
