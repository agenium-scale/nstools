// MIT License
//
// Copyright (c) 2020 Agenium Scale
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

#include <iomanip>
#include <stdexcept>

// ----------------------------------------------------------------------------

struct parser_t : public ns2::json_parser_t {
  std::ostream *out_;
  int indent;
  bool inside_array;

  parser_t(std::ostream *out) : out_(out), indent(0), inside_array(false) {}

  bool begin_map(ns2::cursor_t const &) {
    (*out_) << "{\n";
    indent += 2;
    return true;
  }

  bool end_map(ns2::cursor_t const &) {
    indent -= 2;
    (*out_) << std::string(size_t(indent), ' ') << "}\n";
    if (indent < 0) {
      indent = 0; // should never happen
    }
    return true;
  }

  bool begin_array(ns2::cursor_t const &) {
    (*out_) << "[\n";
    indent += 2;
    inside_array = true;
    return true;
  }

  bool end_array(ns2::cursor_t const &) {
    indent -= 2;
    (*out_) << std::string(size_t(indent), ' ') << "]\n";
    if (indent < 0) {
      indent = 0; // should never happen
    }
    inside_array = false;
    return true;
  }

  bool new_key(ns2::cursor_t const &, std::string const &key) {
    (*out_) << std::string(size_t(indent), ' ') << key << ": ";
    return true;
  }

  bool new_null(ns2::cursor_t const &) {
    if (inside_array) {
      (*out_) << std::string(size_t(indent), ' ');
    }
    (*out_) << "null\n";
    return true;
  }

  bool new_boolean(ns2::cursor_t const &, bool b) {
    if (inside_array) {
      (*out_) << std::string(size_t(indent), ' ');
    }
    (*out_) << (b ? "true\n" : "false\n");
    return true;
  }

  bool new_string(ns2::cursor_t const &, std::string const &str) {
    if (inside_array) {
      (*out_) << std::string(size_t(indent), ' ');
    }
    (*out_) << "\"" << str << "\"\n";
    return true;
  }

  bool new_number(ns2::cursor_t const &, double dbl) {
    if (inside_array) {
      (*out_) << std::string(size_t(indent), ' ');
    }
    double adbl = dbl < 0 ? -dbl : dbl;
    if (adbl > 0.0001 && adbl < 1000.0) {
      (*out_) << dbl << "\n";
    } else {
      (*out_) << std::setprecision(std::numeric_limits<double>::digits10 + 1)
              << std::scientific << dbl << "\n";
    }
    return true;
  }
};

// ----------------------------------------------------------------------------

void dump_json(std::ostream *out_, std::istream *in_) {
  std::ostream &out = *out_;
  std::istream &in = *in_;
  parser_t parser(&out);
  std::vector<std::string> errors;
  ns2::parse_json(&in, &errors, &parser);
  if (errors.size() > 0) {
    std::cout << "ERROR: " << errors[0] << "\n";
  }
}

// ----------------------------------------------------------------------------

int main(int argc, char **argv) {
  if (argc == 1) {
    std::cout << "USAGE: " << argv[0] << " FILE1 [FILE2 ...]\n";
    return 0;
  }
  for (int i = 1; i < argc; i++) {
    ns2::ifile_t in(argv[i]);
    std::cout << std::string(79, '=') << "\n"
              << "DUMPING: " << argv[i] << "\n";
    dump_json(&std::cout, &in);
  }
  return 0;
}
