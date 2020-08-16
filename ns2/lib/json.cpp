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

#include <cassert>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <ns2/json.hpp>
#include <sstream>

namespace ns2 {

// ----------------------------------------------------------------------------

static bool is_blank(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

// ---

static bool is_end_of_token(char c) {
  return is_blank(c) || c == ',' || c == ']' || c == '}';
}

// ----------------------------------------------------------------------------

namespace { // put status_t into anonymous namespace to avoid linkage problem

struct parser_error_t {};

struct status_t {
  cursor_t currpos;

  enum expect_t {
    Any,
    MapFirstKeyOrEnd,
    MapKey,
    MapColon,
    MapValue,
    MapComaOrEnd,
    ArrayFirstValueOrEnd,
    ArrayValue,
    ArrayComaOrEnd,
    EndOfFile
  } expect;

  enum inside_t { File, Map, Array };
  std::vector<inside_t> inside;

  std::vector<std::pair<cursor_t, std::string> > errors;

  void error(std::string const &msg) {
    errors.push_back(std::pair<cursor_t, std::string>(currpos, msg));
    throw parser_error_t();
  }

  status_t() {
    expect = Any;
    inside.push_back(File);
  }

  std::string expect_as_str() {
    const char *str[] = {
        "",
        "string for object key or '}'",
        "string for object key",
        "colon between key and value",
        "string, number, object, array, null, true or false",
        "coma or '}'",
        "string, number, object, array, null, true, false or ']'",
        "string, number, object, array, null, true or false",
        "coma or ']'",
        "end of file"};
    return str[int(expect)];
  }
};

} // anonymous namespace

// ----------------------------------------------------------------------------

static long to_long(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  }
  if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  }
  if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  }
  return -1; // indicate error
}

// ---

static std::string to_utf8(long codepoint_) {
  char ret[5] = {0};
  assert(codepoint_ >= 0 && codepoint_ <= 1114111);
  unsigned long codepoint = (unsigned long)(codepoint_);
  if (codepoint <= 0x7c) {
    // 0xxxxxxx
    ret[0] = char(codepoint);
  } else if (codepoint >= 0x80 && codepoint <= 0x7ff) {
    // 110xxxxx 10xxxxxx
    ret[0] = char(0xc0 | (codepoint >> 6));
    ret[1] = char(0x80 | (codepoint & 0x3f));
  } else if (codepoint >= 0x1000 && codepoint <= 0xffff) {
    // 1110xxxx 10xxxxxx 10xxxxxx
    ret[0] = char(0xe0 | (codepoint >> 12));
    ret[1] = char(0x80 | ((codepoint >> 6) & 0x3f));
    ret[2] = char(0x80 | (codepoint & 0x3f));
  } else if (codepoint >= 0x10000 && codepoint <= 0x10fff) {
    // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    ret[0] = char(0xf0 | (codepoint >> 18));
    ret[1] = char(0x80 | ((codepoint >> 12) & 0x3f));
    ret[2] = char(0x80 | ((codepoint >> 6) & 0x3f));
    ret[3] = char(0x80 | (codepoint & 0x3f));
  }
  return std::string(ret);
}

// ----------------------------------------------------------------------------

static bool parse_token(std::string const &token, std::string const &line,
                        size_t i) {
  if (i + token.size() == line.size() &&
      std::string(line, i, token.size()) == token) {
    return true;
  }
  if (i + token.size() + 1 <= line.size() &&
      std::string(line, i, token.size()) == token &&
      is_end_of_token(line[i + token.size()])) {
    return true;
  }
  return false;
}

// ----------------------------------------------------------------------------

enum plus_minus_t { AllowNone, AllowMinusOnly, AllowPlusOnly, AllowPlusMinus };

static double parse_long(status_t *status_, std::string const &line,
                         size_t *i_, std::string const &end_chars,
                         plus_minus_t plus_minus, bool fractional) {
  status_t &status = *status_;
  size_t &i = *i_;

  double sign = 1.0;
  double num = 0.0;
  double pm10 = 0.1;

  if (line[i] == '+') {
    if (plus_minus == AllowPlusOnly || plus_minus == AllowPlusMinus) {
      i++;
      status.currpos.nextchar();
    } else {
      status.error("ill-formed number, unexpected '+', expected digit");
    }
  } else if (line[i] == '-') {
    if (plus_minus == AllowMinusOnly || plus_minus == AllowPlusMinus) {
      i++;
      status.currpos.nextchar();
      sign = -1.0;
    } else {
      status.error("ill-formed number, unexpected '-', expected digit");
    }
  }

  const char *plus_minus_str[4] = {"", " or '-'", " or '+'", ", '-' or '+'"};

  if (i >= line.size()) {
    status.error(
        std::string(
            "unexpected end of line, ill-formed number, expected digit") +
        plus_minus_str[int(plus_minus)]);
  }
  if (is_end_of_token(line[i])) {
    status.error(std::string("illformed number, expected digit") +
                 plus_minus_str[int(plus_minus)]);
  }

  for (;;) {
    if (i >= line.size() || is_end_of_token(line[i]) ||
        end_chars.find(line[i]) != std::string::npos) {
      break;
    } else if (line[i] >= '0' && line[i] <= '9') {
      if (fractional) {
        num += double(line[i] - '0') * pm10;
        pm10 /= 10.0;
      } else {
        num = num * 10.0 + double(line[i] - '0');
      }
    } else {
      status.error("illformed number, expected digit");
    }
    i++;
    status.currpos.nextchar();
  }

  return sign * num;
}

// ---

static double parse_double(status_t *status_, std::string const &line,
                           size_t *i_) {
  status_t &status = *status_;
  size_t &i = *i_;
  double integral_part = 0.0;
  double fractional_part = 0.0;
  double exponent_part = 0.0;

  // integral part
  integral_part = parse_long(&status, line, &i, ".eE", AllowMinusOnly, false);

  // fractional part (if any)
  if (i < line.size() && line[i] == '.') {
    i++;
    status.currpos.nextchar();
    if (i >= line.size()) {
      status.error("unexpected end of line, expected digit");
    } else if (is_end_of_token(line[i])) {
      status.error("expected digit");
    }
    fractional_part =
        parse_long(&status, line, &i, "eE", AllowMinusOnly, true);
  }

  // exponent part (if any)
  if (i < line.size() && (line[i] == 'e' || line[i] == 'E')) {
    i++;
    status.currpos.nextchar();
    if (i >= line.size()) {
      status.error("unexpected end of line, expected digit, '+' or '-'");
    } else if (is_end_of_token(line[i])) {
      status.error("expected digit, '+' or '-'");
    }
    exponent_part = parse_long(&status, line, &i, "", AllowMinusOnly, false);
  }

  i--; // don't forget to go back to the last char part of the number

  if (integral_part >= 0.0) {
    return (integral_part + fractional_part) * std::pow(10.0, exponent_part);
  } else {
    return (integral_part - fractional_part) * std::pow(10.0, exponent_part);
  }
}

// ----------------------------------------------------------------------------

static std::string parse_string(status_t *status_, std::string const &line,
                                size_t *i_) {
  status_t &status = *status_;
  size_t &i = *i_;
  std::string ret;
  for (;;) {
    i++;
    status.currpos.nextchar();
    if (i >= line.size()) {
      status.error("unexpected end of line");
    }
    if (line[i] == '"') {
      break;
    }
    if (line[i] == '\\') {
      i++;
      if (i >= line.size()) {
        status.error("unexpected end of line, expected double quote");
      }
      status.currpos.nextchar();
      switch (line[i]) {
      case '"':
        ret += '"';
        break;
      case '\\':
        ret += '\\';
        break;
      case 'n':
        ret += '\n';
        break;
      case 't':
        ret += '\t';
        break;
      case 'r':
        ret += '\r';
        break;
      case 'f':
        ret += '\f';
        break;
      case 'b':
        ret += '\b';
        break;
      case 'u': {
        long codepoint = 0;
        for (int p = 3; p >= 0; p--) {
          i++;
          if (i >= line.size()) {
            status.error("unexpected end of line, expected hexadecimal digit");
          }
          status.currpos.nextchar();
          long digit = to_long(line[i]);
          if (digit == -1) {
            status.error(std::string("unexpected '") + line[i] +
                         "', expected hexadecimal digit");
          }
          codepoint = codepoint * 16 + digit;
        }
        ret += to_utf8(codepoint);
        break;
      }
      default:
        status.error(std::string("unexpected '") + line[i] +
                     "', expected '\\', 'n', 't', 'r', 'f', 'b'");
      }
      continue;
    }
    ret += line[i];
  }
  return ret;
}

// ----------------------------------------------------------------------------

NS_DLLSPEC void parse_json(std::istream *in_,
                           std::vector<std::string> *errors_,
                           json_parser_t *parser_) {
  std::istream &in = *in_;
  std::vector<std::string> &errors = *errors_;
  json_parser_t &parser = *parser_;
  status_t status;

  try {

    while (!in.eof()) {
      // read line
      std::string line;
      std::getline(in, line);
      if (line.size() > 0 && line.back() == '\r') { // Windows endline...
        line.pop_back();
      }
      status.currpos.newline();

      for (size_t i = 0; i < line.size(); i++) {
        status.currpos.nextchar();
        if (is_blank(line[i])) {
          continue;
        }

        if (line[i] == '"') {
          cursor_t pos(status.currpos);
          std::string str(parse_string(&status, line, &i));
          switch (status.expect) {
          case status_t::MapKey:
          case status_t::MapFirstKeyOrEnd:
            status.expect = status_t::MapColon;
            parser.new_key(pos, str);
            break;
          case status_t::MapValue:
            status.expect = status_t::MapComaOrEnd;
            parser.new_string(pos, str);
            break;
          case status_t::ArrayFirstValueOrEnd:
          case status_t::ArrayValue:
            status.expect = status_t::ArrayComaOrEnd;
            parser.new_string(pos, str);
            break;
          case status_t::Any:
            status.expect = status_t::EndOfFile;
            parser.new_string(pos, str);
            break;
          default:
            status.error("unexpected string, expected " +
                         status.expect_as_str());
            return;
          }
          continue;
        }

        if (line[i] == '-' || (line[i] >= '0' && line[i] <= '9')) {
          cursor_t pos(status.currpos);
          double val = parse_double(&status, line, &i);
          switch (status.expect) {
          case status_t::MapValue:
            status.expect = status_t::MapComaOrEnd;
            break;
          case status_t::ArrayValue:
          case status_t::ArrayFirstValueOrEnd:
            status.expect = status_t::ArrayComaOrEnd;
            break;
          case status_t::Any:
            status.expect = status_t::EndOfFile;
            break;
          default:
            status.error("unexpected number, expected " +
                         status.expect_as_str());
            return;
          }
          parser.new_number(pos, val);
          continue;
        }

        bool token_is_null = parse_token("null", line, i);
        bool token_is_false = parse_token("false", line, i);
        bool token_is_true = parse_token("true", line, i);
        if (token_is_null || token_is_false || token_is_true) {
          switch (status.expect) {
          case status_t::Any:
            status.expect = status_t::EndOfFile;
            break;
          case status_t::MapValue:
            status.expect = status_t::MapComaOrEnd;
            break;
          case status_t::ArrayValue:
          case status_t::ArrayFirstValueOrEnd:
            status.expect = status_t::ArrayComaOrEnd;
            break;
          default:
            if (token_is_null) {
              status.error("unexpected 'null', expected " +
                           status.expect_as_str());
            } else if (token_is_false) {
              status.error("unexpected 'false', expected " +
                           status.expect_as_str());
            } else {
              status.error("unexpected 'true', expected " +
                           status.expect_as_str());
            }
            return;
          }
          if (token_is_null) {
            i += 3;
            parser.new_null(status.currpos);
          } else if (token_is_false) {
            i += 4;
            parser.new_boolean(status.currpos, false);
          } else {
            i += 3;
            parser.new_boolean(status.currpos, true);
          }
          continue;
        }

        if (line[i] == ']' || line[i] == '}') {
          if (line[i] == ']') {
            switch (status.expect) {
            case status_t::ArrayComaOrEnd:
            case status_t::ArrayFirstValueOrEnd:
              break;
            default:
              status.error("unexpected ']', expected " +
                           status.expect_as_str());
              return;
            }
            parser.end_array(status.currpos);
          }
          if (line[i] == '}') {
            switch (status.expect) {
            case status_t::MapComaOrEnd:
            case status_t::MapFirstKeyOrEnd:
              break;
            default:
              status.error("unexpected '}', expected " +
                           status.expect_as_str());
              return;
            }
            parser.end_map(status.currpos);
          }
          status.inside.pop_back();
          switch (status.inside.back()) {
          case status_t::Array:
            status.expect = status_t::ArrayComaOrEnd;
            break;
          case status_t::Map:
            status.expect = status_t::MapComaOrEnd;
            break;
          case status_t::File:
            status.expect = status_t::EndOfFile;
            break;
          }
          continue;
        }

        if (line[i] == '[') {
          switch (status.expect) {
          case status_t::Any:
          case status_t::MapValue:
          case status_t::ArrayFirstValueOrEnd:
          case status_t::ArrayValue:
            status.expect = status_t::ArrayFirstValueOrEnd;
            break;
          default:
            status.error("unexpected '[', expected " + status.expect_as_str());
            return;
          }
          parser.begin_array(status.currpos);
          status.inside.push_back(status_t::Array);
          continue;
        }

        if (line[i] == '{') {
          switch (status.expect) {
          case status_t::Any:
          case status_t::MapValue:
          case status_t::ArrayFirstValueOrEnd:
          case status_t::ArrayValue:
            status.expect = status_t::MapFirstKeyOrEnd;
            break;
          default:
            status.error("unexpected '{', expected " + status.expect_as_str());
            return;
          }
          parser.begin_map(status.currpos);
          status.inside.push_back(status_t::Map);
          continue;
        }

        if (line[i] == ',') {
          switch (status.expect) {
          case status_t::MapComaOrEnd:
            status.expect = status_t::MapKey;
            break;
          case status_t::ArrayComaOrEnd:
            status.expect = status_t::ArrayValue;
            break;
          default:
            status.error("unexpected coma, expected " +
                         status.expect_as_str());
            return;
          }
          continue;
        }

        if (line[i] == ':') {
          switch (status.expect) {
          case status_t::MapColon:
            status.expect = status_t::MapValue;
            break;
          default:
            status.error("unexpected ':', expected " + status.expect_as_str());
            return;
          }
          continue;
        }
      }
    }

    // getting here means end of file and we expect it of Any if nothing is
    // present in the file
    switch (status.expect) {
    case status_t::Any:
    case status_t::EndOfFile:
      break;
    default:
      status.error("unexpected end of file, expected " +
                   status.expect_as_str());
      return;
    }

  } catch (parser_error_t const &) {
    for (size_t i = 0; i < status.errors.size(); i++) {
      errors.push_back(status.errors[i].first.to_string() + ": " +
                       status.errors[i].second);
    }
  }
}

// ----------------------------------------------------------------------------

NS_DLLSPEC void parse_json(std::string const &in_,
                           std::vector<std::string> *errors,
                           json_parser_t *parser) {

  std::istringstream in(in_);
  parse_json(&in, errors, parser);
}

// ----------------------------------------------------------------------------

} // namespace ns2
