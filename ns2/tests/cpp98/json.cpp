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

struct jp1_t : public ns2::json_parser_t {
  std::string *str;
  double *dbl;
  size_t lineno, col;

  jp1_t(std::string *str_, double *dbl_) : str(str_), dbl(dbl_) {}

  bool begin_map(ns2::cursor_t const &) { return true; }
  bool end_map(ns2::cursor_t const &) { return true; }
  bool begin_array(ns2::cursor_t const &) { return true; }
  bool end_array(ns2::cursor_t const &) { return true; }
  bool new_key(ns2::cursor_t const &, std::string const &) { return true; }
  bool new_null(ns2::cursor_t const &) { return true; }
  bool new_boolean(ns2::cursor_t const &, bool) { return true; }

  bool new_string(ns2::cursor_t const &cursor, std::string const &str_) {
    if (str != NULL) {
      *str = str_;
      lineno = cursor.lineno;
      col = cursor.col;
    }
    return true;
  }

  bool new_number(ns2::cursor_t const &cursor, double dbl_) {
    if (dbl != NULL) {
      *dbl = dbl_;
      lineno = cursor.lineno;
      col = cursor.col;
    }
    return true;
  }
};

// ----------------------------------------------------------------------------

int test_string(std::string const &json_src, bool correct_json, int lineno,
                int col, std::string const &expected_string) {
  std::string dst;
  jp1_t jp1(&dst, NULL);
  std::vector<std::string> errors;

  ns2::parse_json(json_src, &errors, &jp1);

  if (correct_json && errors.size() > 0) {
    std::cerr << "JSON test_string" << errors[0] << std::endl;
    return 1;
  } else if (!correct_json && errors.size() == 0) {
    std::cerr << "JSON test_string: incorrect json but no parsing error"
              << std::endl;
    return 1;
  } else if (!correct_json && errors.size() > 0) {
    return 0;
  } else {
    if (jp1.lineno == lineno && jp1.col == col &&
        *jp1.str == expected_string) {
      return 0;
    } else {
      std::cerr << "JSON test_string: expected '" << expected_string
                << "' at (" << lineno << "," << col << ") but got '"
                << *jp1.str << "' at (" << jp1.lineno << "," << jp1.col << ")"
                << std::endl;
      if (errors.size() > 0) {
        std::cerr << "JSON test_string" << errors[0] << std::endl;
      }
      return 1;
    }
  }
}

// ----------------------------------------------------------------------------

int test_double(std::string const &json_src, bool correct_json, int lineno,
                int col, double expected_double) {
  double dst;
  jp1_t jp1(NULL, &dst);
  std::vector<std::string> errors;

  ns2::parse_json(json_src, &errors, &jp1);

  if (correct_json && errors.size() > 0) {
    std::cerr << "JSON test_double" << errors[0] << std::endl;
    return 1;
  } else if (!correct_json && errors.size() == 0) {
    std::cerr << "JSON test_double: incorrect json but no parsing error"
              << std::endl;
    return 1;
  } else if (!correct_json && errors.size() > 0) {
    return 0;
  } else {
    if (jp1.lineno == lineno && jp1.col == col &&
        *jp1.dbl == expected_double) {
      return 0;
    } else {
      std::cerr << "JSON test_double: expected '" << expected_double
                << "' at (" << lineno << "," << col << ") but got '"
                << *jp1.dbl << "' at (" << jp1.lineno << "," << jp1.col << ")"
                << std::endl;
      if (errors.size() > 0) {
        std::cerr << "JSON test_double" << errors[0] << std::endl;
      }
      return 1;
    }
  }
}

// ----------------------------------------------------------------------------

struct jp2_t : public ns2::json_parser_t {
  std::string result;

  bool begin_map(ns2::cursor_t const &) {
    result += '{';
    return true;
  }

  bool end_map(ns2::cursor_t const &) {
    result += '}';
    return true;
  }

  bool begin_array(ns2::cursor_t const &) {
    result += '[';
    return true;
  }

  bool end_array(ns2::cursor_t const &) {
    result += ']';
    return true;
  }

  bool new_key(ns2::cursor_t const &, std::string const &) {
    result += 'K';
    return true;
  }

  bool new_null(ns2::cursor_t const &) {
    result += 'N';
    return true;
  }

  bool new_boolean(ns2::cursor_t const &, bool) {
    result += 'B';
    return true;
  }

  bool new_string(ns2::cursor_t const &, std::string const &) {
    result += 'S';
    return true;
  }

  bool new_number(ns2::cursor_t const &, double) {
    result += 'D';
    return true;
  }
};

// ----------------------------------------------------------------------------

int test_json(std::string const &json_src, bool correct_json,
              std::string const &expected_result) {
  jp2_t jp2;
  std::vector<std::string> errors;

  ns2::parse_json(json_src, &errors, &jp2);

  if (correct_json && errors.size() > 0) {
    std::cerr << "JSON test_json" << errors[0] << std::endl;
    return 1;
  } else if (!correct_json && errors.size() == 0) {
    std::cerr << "JSON test_json: incorrect json but no parsing error"
              << std::endl;
    return 1;
  } else if (!correct_json && errors.size() > 0) {
    return 0;
  } else {
    if (jp2.result == expected_result) {
      return 0;
    } else {
      std::cerr << "JSON test_json: expected '" << expected_result
                << "' but got '" << jp2.result << std::endl;
      if (errors.size() > 0) {
        std::cerr << "JSON test_json" << errors[0] << std::endl;
      }
      return 1;
    }
  }
}

// ----------------------------------------------------------------------------

int main() {
  // clang-format off
  return 0
      || test_string("\"foo\"", true, 1, 1, "foo")
      || test_string("    \"foo\"", true, 1, 5, "foo")
      || test_string("\n\n    \"foo\"", true, 3, 5, "foo")
      || test_string("\"", false, 0, 0, "")
      || test_string("\"\"", true, 1, 1, "")
      || test_string("\"\\t\"", true, 1, 1, "\t")
      || test_string("\"\\n\"", true, 1, 1, "\n")
      || test_string("\"\\b\"", true, 1, 1, "\b")
      || test_string("\"\\f\"", true, 1, 1, "\f")
      || test_string("\"\\\\\"", true, 1, 1, "\\")
      || test_string("\"\\\"\"", true, 1, 1, "\"")
      || test_string("\"\\u\"", false, 0, 0, "")
      || test_string("\"\\u12\"", false, 0, 0, "")
      || test_string("\"\\uXX\"", false, 0, 0, "")
      || test_string("\"\\u0041\"", true, 1, 1, "A")
      || test_string("\"\\u00e9\"", true, 1, 1, "é")
      || test_string("\"\\u20AC\"", true, 1, 1, "€")
      || test_double("1", true, 1, 1, 1.0)
      || test_double("1.0", true, 1, 1, 1.0)
      || test_double("1.0e0", true, 1, 1, 1.0)
      || test_double("1E0", true, 1, 1, 1.0)
      || test_double("1.", false, 0, 0, 1.0)
      || test_double("-1", true, 1, 1, -1.0)
      || test_double("-1.0", true, 1, 1, -1.0)
      || test_double("-1.0e0", true, 1, 1, -1.0)
      || test_double("-1E0", true, 1, 1, -1.0)
      || test_double("-1.", false, 0, 0, -1.0)
      || test_double("-.", false, 0, 0, -1.0)
      || test_double("-", false, 0, 0, -1.0)
      || test_double("0.5", true, 1, 1, 0.5)
      || test_double("000.500", true, 1, 1, 0.5)
      || test_double("5.0e-1", true, 1, 1, 0.5)
      || test_double("500E-3", true, 1, 1, 0.5)
      || test_double("5e-", false, 0, 0, 0.5)
      || test_double("500", true, 1, 1, 500.0)
      || test_double("000.500e3", true, 1, 1, 500.0)
      || test_double("5.0e2", true, 1, 1, 500.0)
      || test_double("500.00", true, 1, 1, 500.0)
      || test_json("", true, "")
      || test_json("   ", true, "")
      || test_json(" \t  \t \t\t \n\n \t\t \n\t\n\t    ", true, "")
      || test_json("[0]", true, "[D]")
      || test_json("[0,0]", true, "[DD]")
      || test_json("[0,null,0]", true, "[DND]")
      || test_json("[0,]", false, "")
      || test_json("[,0]", false, "")
      || test_json("[", false, "")
      || test_json("[0", false, "")
      || test_json("[[0]]", true, "[[D]]")
      || test_json("[[0,true,0]]", true, "[[DBD]]")
      || test_json("[null,[0],true]", true, "[N[D]B]")
      || test_json("[[]]", true, "[[]]")
      || test_json("[]", true, "[]")
      || test_json("[[]", false, "")
      || test_json("[]]", false, "")
      || test_json("[null,[[]],true,[0,[0],0]]", true, "[N[[]]B[D[D]D]]")
      || test_json("{}", true, "{}")
      || test_json("}", false, "")
      || test_json("{", false, "")
      || test_json("{\"a\":null}", true, "{KN}")
      || test_json("{\"a\":null,}", false, "")
      || test_json("{\"a\":null,\"b\":true,\"c\":false}", true, "{KNKBKB}")
      || test_json("{\"a\"}", false, "")
      || test_json("{null:false}", false, "")
      || test_json("{\"a\":false,\"b\":-1.9e8,\"c\":\"str\"}", true, "{KBKDKS}")
      || test_json("[{}]", true, "[{}]")
      || test_json("[{}", false, "")
      || test_json("{}]", false, "")
      || test_json("[{]", false, "")
      || test_json("[}]", false, "")
      || test_json("{\"a\":[],\"b\":[null,6,false]}", true, "{K[]K[NDB]}")
      || test_json("{\"a\":[],\"b\":[{},[[]]]}", true, "{K[]K[{}[[]]]}")
      || test_json("{\"a\":[,\"b\":[{},[[]]]}", false, "")
      || test_json("{\"a\":[],\"b\":[},[[]]]}", false, "")
      || test_json("{\"a\":[],\"b\":[{},[]]]}", false, "")
      ;
  // clang-format on
}
