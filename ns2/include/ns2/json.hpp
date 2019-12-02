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

#ifndef NS2_JSON_HPP
#define NS2_JSON_HPP

#include <cassert>
#include <istream>
#include <map>
#include <ns2/config.hpp>
#include <ns2/cursor.hpp>
#include <string>
#include <vector>

namespace ns2 {

// ----------------------------------------------------------------------------

struct json_parser_t {
  virtual bool begin_map(cursor_t const &) = 0;
  virtual bool end_map(cursor_t const &) = 0;
  virtual bool begin_array(cursor_t const &) = 0;
  virtual bool end_array(cursor_t const &) = 0;
  virtual bool new_key(cursor_t const &, std::string const &) = 0;
  virtual bool new_string(cursor_t const &, std::string const &) = 0;
  virtual bool new_number(cursor_t const &, double) = 0;
  virtual bool new_null(cursor_t const &) = 0;
  virtual bool new_boolean(cursor_t const &, bool) = 0;
};

// ----------------------------------------------------------------------------

NS_DLLSPEC void parse_json(std::istream *, std::vector<std::string> *,
                           json_parser_t *);

NS_DLLSPEC void parse_json(std::string const &, std::vector<std::string> *,
                           json_parser_t *);

// ----------------------------------------------------------------------------

class json_value_t {

public:
  // possible types
  enum type_t { Null, Boolean, Number, String, Array, Object, InvalidValue };

private:
  // type
  type_t type;

  // posible values
  std::string str;
  double dbl;
  bool boolean;
  std::vector<json_value_t *> array;
  std::map<std::string, json_value_t *> map;

public:
  // is_* functions (here for performances)
  bool is_null() const { return type == Null; }
  bool is_boolean() const { return type == Boolean; }
  bool is_number() const { return type == Number; }
  bool is_string() const { return type == String; }
  bool is_array() const { return type == Array; }
  bool is_object() const { return type == Object; }
  bool is_valid_value() const { return type != InvalidValue; }

  // get_* functions (here for performances)
  std::string const &get_string() const {
    assert(is_string());
    return str;
  }

  double get_number() const {
    assert(is_number());
    return dbl;
  }

  bool get_boolean() const {
    assert(is_boolean());
    return boolean;
  }

  // when an array or a map
  size_t size() const {
    assert(is_array() || is_object());
    if (is_array()) {
      return array.size();
    } else {
      return map.size();
    }
  }

  json_value_t const &operator[](size_t i) const {
    assert(is_array());
    return *array[i];
  }

  json_value_t const &operator[](std::string const &key) const {
    static json_value_t invalid;
    assert(is_object());
    typedef std::map<std::string, json_value_t *> map_t;
    map_t::const_iterator it = map.find(key);
    if (it != map.end()) {
      return *(it->second);
    } else {
      invalid.type = InvalidValue;
      return invalid;
    }
  }

  // ctor and dtor
  json_value_t();
  ~json_value_t();
};

// ----------------------------------------------------------------------------

} // namespace ns2

#endif
