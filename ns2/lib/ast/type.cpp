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

#include <ns2/ast/type.hpp>

namespace ns2 {
namespace ast {

std::string type_t::pprint() const {
  std::string output = "";
  for (size_t i = 0; i < qualifier_before.size(); i++) {
    output += qualifier_before[i]->data + " ";
  }
  output += name->data;
  for (size_t i = 0; i < asterisk.size(); i++) {
    output += asterisk[i]->data;
  }
  if (ref != NULL) {
    output += ref->data;
  }
  for (size_t i = 0; i < qualifier_after.size(); i++) {
    output += qualifier_after[i]->data + " ";
  }
  /*
  // template
  std::vector<type_t> template_types;
  */
  return output;
}

std::string bool_() { return "bool"; }

std::string void_() { return "void"; }

std::string char_() { return "char"; }

std::string short_() { return "short"; }

std::string int_() { return "int"; }

std::string long_() { return "long"; }

std::string float_() { return "float"; }

std::string double_() { return "double"; }

std::string uchar_() { return "unsigned char"; }

std::string ushort_() { return "unsigned short"; }

std::string uint_() { return "unsigned int"; }

std::string ulong_() { return "unsigned long"; }

std::string ufloat_() { return "unsigned float"; }

std::string udouble_() { return "unsigned double"; }

std::string ufloat16_() { return "unsigned float16_t"; }

std::string unknown_() { return "????"; }

std::string float16_() { return "float16_t"; }

std::string float32_() { return "float32_t"; }

std::string float64_() { return "float64_t"; }

std::string int8_() { return "int8_t"; }

std::string int16_() { return "int16_t"; }

std::string int32_() { return "int32_t"; }

std::string int64_() { return "int64_t"; }

std::string uint8_() { return "uint8_t"; }

std::string uint16_() { return "uint16_t"; }

std::string uint32_() { return "uint32_t"; }

std::string uint64_() { return "uint64_t"; }

type_t void_type() {
  static lexer::lexeme_t tmp(-1, -1, lexer::lexeme_t::KIND_T_WORD, void_());
  type_t t;
  t.name = &tmp;
  return t;
}

type_t unknown_type() {
  static lexer::lexeme_t tmp(-1, -1, lexer::lexeme_t::KIND_T_WORD, unknown_());
  type_t t;
  t.name = &tmp;
  return t;
}

} // namespace ast
} // namespace ns2