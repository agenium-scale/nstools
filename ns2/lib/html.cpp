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
#include <ns2/html.hpp>
#include <ns2/string.hpp>

#include <string>
#include <vector>

namespace ns2 {

// ----------------------------------------------------------------------------

NS_DLLSPEC std::string begin_html_tag(
    std::string const &tag,
    std::vector<std::pair<std::string, std::string> > const &attrs) {
  std::string ret("<" + tag);
  if (attrs.size() > 0) {
    for (size_t i = 0; i < attrs.size(); i++) {
      ret += " " + attrs[i].first + "=\"" + attrs[i].second + "\"";
    }
  }
  return ret + ">";
}

// ---

NS_DLLSPEC std::string end_html_tag(std::string const &tag) {
  std::string lower_tag(ns2::lower(tag));
  if (lower_tag == "img" || lower_tag == "br" || lower_tag == "hr") {
    return std::string();
  }
  return "</" + tag + ">";
}

// ----------------------------------------------------------------------------

NS_DLLSPEC std::string html_href(std::string const &url,
                                 std::string const &text) {
  return "<a href=\"" + url + "\">" + text + "</a>";
}

// ----------------------------------------------------------------------------

NS_DLLSPEC std::string html_img(std::string const &url,
                                std::string const &alt) {
  return "<img src=\"" + url + "\" alt=\"" + alt + "\" />";
}

// ----------------------------------------------------------------------------

static std::string
html_td_th_with_alignments(std::vector<std::string> const &cells,
                           std::vector<text_align_t> const &aligns,
                           std::string const &td_th) {
  std::string ret("<tr>\n");
  for (size_t i = 0; i < cells.size(); i++) {
    switch (aligns[i]) {
    case Left:
      ret += "<" + td_th + " style=\"text-align: left;\">";
      break;
    case Center:
      ret += "<" + td_th + " style=\"text-align: center;\">";
      break;
    case Right:
      ret += "<" + td_th + " style=\"text-align: right;\">";
      break;
    }
    ret += cells[i] + "</" + td_th + ">\n";
  }
  ret += "</tr>";
  return ret;
}

NS_DLLSPEC std::string
html_td_with_alignments(std::vector<std::string> const &cells,
                        std::vector<text_align_t> const &aligns) {
  return html_td_th_with_alignments(cells, aligns, "td");
}

NS_DLLSPEC std::string
html_th_with_alignments(std::vector<std::string> const &cells,
                        std::vector<text_align_t> const &aligns) {
  return html_td_th_with_alignments(cells, aligns, "th");
}

// ----------------------------------------------------------------------------

NS_DLLSPEC std::string htmlize(std::string const &s) {
  std::string ret;
  for (size_t i = 0; i < s.size(); i++) {
    ret += htmlize(s[i]);
  }
  return ret;
}

// ----------------------------------------------------------------------------

} // namespace ns2
