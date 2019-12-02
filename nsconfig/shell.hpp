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

#ifndef SHELL_HPP
#define SHELL_HPP

#include "parser.hpp"
#include <string>
#include <vector>

namespace shell {

// ----------------------------------------------------------------------------

std::string translate(std::vector<parser::token_t> const &, parser::infos_t *);
std::string stringify(std::string const &);
std::string ify(std::string const &);
std::vector<std::string> ify(std::vector<std::string> const &);
std::string autodeps_flags(compiler::infos_t::type_t, std::string);
std::string rm(bool, std::string const &);
std::string cp(bool, std::string const &, std::string const &);
std::string mkdir_p(std::string const &);
std::string zip(std::string const &);

// ----------------------------------------------------------------------------

} // namespace shell

#endif
