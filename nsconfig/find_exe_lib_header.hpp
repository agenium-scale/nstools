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

#ifndef FIND_EXE_LIB_HEADER_HPP
#define FIND_EXE_LIB_HEADER_HPP

#include "nsconfig.hpp"
#include "parser.hpp"

// ----------------------------------------------------------------------------

enum libtype_t { Dynamic, Static, Automatic };

// ----------------------------------------------------------------------------

std::string lib_basename(std::string const &);

void find_lib(parser::variables_t *, rule_desc_t<WithShellTranslation> *,
              std::string const &, std::string const &, std::string const &,
              std::vector<std::string> const &, int, libtype_t, bool, bool);

void find_header(parser::variables_t *, std::string const &,
                 std::string const &, std::vector<std::string> const &, int,
                 bool);

void find_exe(parser::variables_t *, std::string const &, std::string const &,
              std::vector<std::string> const &, int, bool);

// ----------------------------------------------------------------------------

#endif
