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

#ifndef NS2_MARKDOWN_HPP
#define NS2_MARKDOWN_HPP

#include <ns2/config.hpp>
#include <ns2/cursor.hpp>
#include <string>
#include <utility>
#include <vector>

namespace ns2 {

// ----------------------------------------------------------------------------

struct markdown_parser_error_t : public std::exception {
  std::string what_;

public:
  markdown_parser_error_t(std::string const &what) : what_(what) {}

  virtual ~markdown_parser_error_t() throw() {}

  virtual const char *what() const throw() { return what_.c_str(); }
};

// ----------------------------------------------------------------------------

enum output_format_t { HTML };
struct markdown_infos_t;

typedef std::pair<std::string, bool> (*markdown_callback_link_t)(
    std::string const &, std::string const &, markdown_infos_t const &);

typedef std::string (*markdown_callback_macro_t)(std::string const &,
                                                 std::string const &,
                                                 markdown_infos_t const &);

struct markdown_infos_t {
  ns2::output_format_t output_format;
  ns2::markdown_callback_macro_t callback_macro;
  ns2::markdown_callback_link_t callback_link;
  bool use_highlight_js;

  markdown_infos_t(ns2::output_format_t output_format = ns2::HTML,
                   ns2::markdown_callback_macro_t callback_macro = NULL,
                   ns2::markdown_callback_link_t callback_link = NULL,
                   bool use_highlight_js = false)
      : output_format(output_format), callback_macro(callback_macro),
        callback_link(callback_link), use_highlight_js(use_highlight_js) {}
};

// ----------------------------------------------------------------------------

NS_DLLSPEC void
compile_markdown(std::istream *, std::ostream *,
                 markdown_infos_t const &mi = markdown_infos_t());

NS_DLLSPEC void
compile_markdown(std::string const &, std::string *,
                 markdown_infos_t const &mi = markdown_infos_t());

// ----------------------------------------------------------------------------

} // namespace ns2

#endif
