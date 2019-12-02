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

#include <string>
#include <vector>

// ----------------------------------------------------------------------------

std::pair<std::string, bool> link(std::string const &lbl,
                                  std::string const &url,
                                  ns2::markdown_infos_t const &) {
  return std::pair<std::string, bool>(
      std::string("[lbl=" + lbl + "|url=" + url + "]"), true);
}

std::string macro(std::string const &arg1, std::string const &arg2,
                  ns2::markdown_infos_t const &) {
  return "[arg1=" + arg1 + "|arg2=" + arg2 + "]";
}

// ----------------------------------------------------------------------------

int test_markdown(std::string const &src, std::string const &expected_output,
                  bool expecting_error) {
  std::string dst;
  bool error = false;
  std::string error_msg;
  try {
    ns2::markdown_infos_t mi;
    mi.output_format = ns2::HTML;
    mi.callback_link = link;
    mi.callback_macro = macro;
    mi.use_highlight_js = true;
    compile_markdown(src, &dst, mi);
  } catch (ns2::markdown_parser_error_t &e) {
    error = true;
    error_msg = std::string(e.what());
  }
  if (expecting_error && error) {
    return 0;
  } else if (expecting_error && !error) {
    std::cerr << "--   incorrect Markdown but no parsing error" << std::endl;
    return 1;
  } else if (!expecting_error && error) {
    std::cerr << "--   markdown test: " << error_msg << std::endl;
    return 1;
  } else {
    if (dst == expected_output) {
      return 0;
    } else {
      std::cerr << "--   input >>>" << std::endl
                << src << std::endl
                << "<<< expected >>>" << std::endl
                << expected_output << std::endl
                << "<<< but got >>>" << std::endl
                << dst << std::endl;
      if (error) {
        std::cerr << "--   markdown test: " << error_msg << std::endl;
      }
      return 1;
    }
  }
  return 0; // should never be reached
}

// ----------------------------------------------------------------------------

int main() {
  std::vector<std::string> file_inputs = ns2::glob(MARKDOWN_DIR "/*.md");

  if (file_inputs.size() == 0) {
    std::cerr << "No input file found in \"" << MARKDOWN_DIR << "\""
              << std::endl;
    return -1;
  }

  for (size_t i = 0; i < file_inputs.size(); i++) {
    ns2::dir_file_t df = ns2::split_path(file_inputs[i]);
    bool valid_markdown = ns2::startswith(df.second, "ok");
    std::string file_expected_output(
        ns2::join_path(MARKDOWN_DIR, "ref_" + df.second + ".html"));
    std::cout << "-- Markdown test: input = \"" << file_inputs[i] << "\" ("
              << (valid_markdown ? "valid" : "non valid") << ")" << std::endl;

    std::string markdown(ns2::read_file(file_inputs[i]));
    std::string expected_output;
    if (valid_markdown) {
      expected_output = ns2::read_file(file_expected_output);
    }
    if (test_markdown(markdown, expected_output, !valid_markdown)) {
      return -1;
    } else {
      std::cout << "--   ok" << std::endl;
    }
  }

  return 0;
}
