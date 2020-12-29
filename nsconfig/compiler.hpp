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

#ifndef COMPILER_HPP
#define COMPILER_HPP

#include <map>
#include <string>
#include <vector>

// ----------------------------------------------------------------------------
// Forward declaration

namespace parser {

struct infos_t;

} // namespace parser

// ----------------------------------------------------------------------------

namespace compiler {

// ----------------------------------------------------------------------------

struct infos_t {
  enum type_t {
    None,
    GCC,
    Clang,
    MSVC,
    ARMClang,
    ICC,
    NVCC,
    HIPCC,
    HCC,
    DPCpp
  } type;
  enum arch_t { Intel, ARMEL, ARMHF, AARCH64 } arch;
  enum lang_t { Unknown, C, CPP } lang;
  std::string name;
  std::string path;
  int version;
  int nbits;
  bool fully_filled;
};

typedef std::map<std::string, compiler::infos_t> list_t;

// ----------------------------------------------------------------------------

infos_t get(std::string, parser::infos_t *);
std::ostream &operator<<(std::ostream &, const compiler::infos_t &);
int get_type_and_lang(compiler::infos_t *, std::string const &);
std::string get_type_str(compiler::infos_t::type_t const);
std::string get_type_and_lang_str(compiler::infos_t::type_t const,
                                  compiler::infos_t::lang_t const);
std::string get_corresponding_cpp_comp(std::string const &);
std::string get_corresponding_c_comp(std::string const &);
void get_archi_from_string(compiler::infos_t *, std::string const &);
void get_version_from_string(compiler::infos_t *,
                             std::vector<std::string> const &);

// ----------------------------------------------------------------------------

#define COMPILER_INFOS_DIR "_compiler_infos"

// ----------------------------------------------------------------------------

} // namespace compiler

#endif
