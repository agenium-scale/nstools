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

#include <ns2/time.hpp>

#include <iostream>
#include <map>
#include <string>
#include <vector>

// ----------------------------------------------------------------------------

template <typename Fct>
int test_timestamp_fct(Fct const &fct, std::string const &fct_name) {
  std::cout << "test_timestamp_fct: " << fct_name << std::endl;

  static std::map<std::string, std::vector<double> > map;

  map[fct_name].push_back(fct());

  double const last = map[fct_name][map[fct_name].size() - 1];
  double const previous = map[fct_name][map[fct_name].size() - 2];

  if (map[fct_name].size() >= 2 && last < previous) {
    std::cerr << "- Error: last values are " << previous << " " << last
              << std::endl;
    return 1;
  } else {
    return 0;
  }
}

// ----------------------------------------------------------------------------

int test_tic_toc() {
  int r = 0;
  double ns = ns2::tic();
  ns2::sleep_ms(9);
  double elapsed_ns = ns2::toc(ns);

  std::cout << "ns2::sleep_ms(9) between ns2::tic and ns2::toc = "
            << elapsed_ns << " ns" << std::endl;
  double const elapsed_ns_min = 9.0 * 1000.0 * 1000.0;
  if (elapsed_ns < elapsed_ns_min) {
    std::cerr << "- Error: elapsed time is lower than " << elapsed_ns_min
              << " ns" << std::endl;
    ++r;
  }
  return r;
}

// ----------------------------------------------------------------------------

int main() {
  int r = 0;

  // timestamp

  for (int i = 0; i < 5; ++i) {
    r += test_timestamp_fct(ns2::timestamp_ns, "ns2::timestamp_ns()");
    r += test_timestamp_fct(ns2::timestamp_us, "ns2::timestamp_us()");
    r += test_timestamp_fct(ns2::timestamp_ms, "ns2::timestamp_ms()");
    r += test_timestamp_fct(ns2::timestamp_s, "ns2::timestamp_s()");
    r += test_timestamp_fct(ns2::timestamp_min, "ns2::timestamp_min()");
    r += test_timestamp_fct(ns2::timestamp_h, "ns2::timestamp_h()");
    r += test_timestamp_fct(ns2::timestamp_d, "ns2::timestamp_d()");
  }
  r += test_tic_toc();
  std::cout << std::endl;

  return r;
}
