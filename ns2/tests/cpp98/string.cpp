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

int main() {

  int nb_fail = 0;

  // ---------------------------------------------------------------------------

  // TODO ns2::to_string

  // ---------------------------------------------------------------------------

  // TODO ns2::split
  {
    std::vector<std::string> r = ns2::split(":A:B:C:", ':');
    for (size_t i = 0; i < r.size(); ++i) {
      std::cout << "r[" << i << "] = \"" << r[i] << "\"" << std::endl;
    }
  }
  {
    std::vector<std::string> r = ns2::split(":L7:L42", ":L");
    for (size_t i = 0; i < r.size(); ++i) {
      std::cout << "r[" << i << "] = \"" << r[i] << "\"" << std::endl;
    }
  }

  // TODO ns2::is_blank

  // TODO ns2::strip

  // TODO ns2::startswith

  // TODO ns2::endswith

  // TODO ns2::replace

  // TODO ns2::join

  // TODO ns2::lower

  // ---------------------------------------------------------------------------

  // TODO ns2::join

  // ---------------------------------------------------------------------------

  // TEST ns2::leading_whitespace
  {
    std::vector<std::string> in;
    std::vector<std::string> out;
    in.push_back("");
    out.push_back("");
    in.push_back("Bouh");
    out.push_back("");
    in.push_back(" Bouh");
    out.push_back(" ");
    in.push_back("\tBouh");
    out.push_back("\t");
    in.push_back("\nBouh");
    out.push_back("\n");
    in.push_back("\rBouh");
    out.push_back("\r");
    in.push_back(" Bouh ");
    out.push_back(" ");
    in.push_back("\tBouh\t");
    out.push_back("\t");
    in.push_back("\rBouh\r");
    out.push_back("\r");
    in.push_back(" \t\n\rBouh");
    out.push_back(" \t\n\r");
    in.push_back(" \t\n\rBouh \t\n\r");
    out.push_back(" \t\n\r");
    for (size_t i = 0; i < in.size(); ++i) {
      if (ns2::leading_whitespace(in[i]) != out[i]) {
        std::cerr << "ns2::leading_whitespace(\"" << in[i] << "\") is \""
                  << ns2::leading_whitespace(in[i])
                  << "\" instead of \"" + out[i] + "\"" << std::endl;
        ++nb_fail;
      }
    }
  }

  // TEST ns2::indent_finder
  {
    std::vector<std::string> in;
    std::vector<std::string> out;
    in.push_back("");
    out.push_back("");
    in.push_back(" ");
    out.push_back(" ");
    in.push_back("\t ");
    out.push_back("\t ");
    in.push_back("i0\n  i2");
    out.push_back("");
    in.push_back("  i2\ni0");
    out.push_back("");
    in.push_back("  i2\n  i2");
    out.push_back("  ");
    in.push_back("  i2\n    i4");
    out.push_back("  ");
    in.push_back("    i4\n  i2");
    out.push_back("  ");
    in.push_back("    i4\n    i4");
    out.push_back("    ");
    for (size_t i = 0; i < in.size(); ++i) {
      if (ns2::indent_finder(in[i]) != out[i]) {
        std::cerr << "ns2::indent_finder(\"" << in[i] << "\") is \""
                  << ns2::indent_finder(in[i])
                  << "\" instead of \"" + out[i] + "\"" << std::endl;
        ++nb_fail;
      }
    }
  }

  // TEST ns2::deindent
  {
    std::vector<std::string> in;
    std::vector<std::string> out;
    in.push_back("");
    out.push_back("");
    in.push_back(" ");
    out.push_back("");
    in.push_back("\t ");
    out.push_back("");
    in.push_back("i0\n  i2");
    out.push_back("i0\n  i2");
    in.push_back("  i2\ni0");
    out.push_back("  i2\ni0");
    in.push_back("  i2\n  i2");
    out.push_back("i2\ni2");
    in.push_back("  i2\n    i4");
    out.push_back("i2\n  i4");
    in.push_back("    i4\n  i2");
    out.push_back("  i4\ni2");
    in.push_back("    i4\n    i4");
    out.push_back("i4\ni4");
    in.push_back("    i4\n      i6\n    i4");
    out.push_back("i4\n  i6\ni4");
    for (size_t i = 0; i < in.size(); ++i) {
      if (ns2::deindent(in[i]) != out[i]) {
        std::cerr << "ns2::deindent(\"" << in[i] << "\") is \""
                  << ns2::deindent(in[i]) << "\" instead of \"" + out[i] + "\""
                  << std::endl;
        ++nb_fail;
      }
    }
  }

  return nb_fail;
}
