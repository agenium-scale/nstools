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

#ifndef NS2_TYPE_HPP
#define NS2_TYPE_HPP

#include <string>

#include <ns2/lexer/lexer.hpp>
#include <ns2/string.hpp>

namespace ns2 {
namespace ast {

// Class type
struct type_t {
  lexer::lexeme_t *name;
  std::vector<lexer::lexeme_t *> qualifier_before;
  std::vector<lexer::lexeme_t *> qualifier_after;
  std::vector<lexer::lexeme_t *> asterisk;
  lexer::lexeme_t *ref;
  // template
  std::vector<type_t> template_types;

  type_t()
      : name(NULL), qualifier_before(), qualifier_after(), asterisk(),
        ref(NULL){};

  // Method to print the type
  virtual std::string pprint() const;
};

std::string bool_();

std::string void_();

std::string char_();

std::string short_();

std::string int_();

std::string long_();

std::string float_();

std::string double_();

std::string uchar_();

std::string ushort_();

std::string uint_();

std::string ulong_();

std::string ufloat_();

std::string udouble_();

std::string ufloat16_();

std::string unknown_();

std::string float16_();

std::string float32_();

std::string float64_();

std::string int8_();

std::string int16_();

std::string int32_();

std::string int64_();

std::string uint8_();

std::string uint16_();

std::string uint32_();

std::string uint64_();

type_t void_type();

type_t unknown_type();

} // namespace ast
} // namespace ns2

#endif