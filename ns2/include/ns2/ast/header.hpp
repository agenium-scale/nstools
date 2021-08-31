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

#ifndef NS2_HEADER_HPP
#define NS2_HEADER_HPP

#include <string>

#include <ns2/ast/ast_node.hpp>

namespace ns2 {
namespace ast {

// Class var_decl_t node
class header_t : public ast_node_t {
protected:
  std::string m_name;

public:
  // Constructors
  header_t();

  header_t(const std::string &name);

  // Set a new name
  void set_name(const std::string &name);

  // Return the name of the header_t
  std::string get_name() const;

  // Method to print a header_t
  std::string pprint() const;

  std::string ast_print(std::string indent, bool last) const;
};

} // namespace ast
} // namespace ns2

#endif