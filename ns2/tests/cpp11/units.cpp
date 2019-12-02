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

#include "ns2/units.hpp"

int main() {

  // Normal unit: metre

  { // default constructor
    constexpr ns2::metre m {};
    static_assert(m.value() == 0, "");
  }

  { // constructor from value
    constexpr ns2::metre m = 5;
    static_assert(m.value() == 5, "");
  }

  { // copy constructor
    constexpr ns2::metre m = 5;
    constexpr ns2::metre m2(m);
    static_assert(m2.value() == 5, "");
  }

  { // constructor from derived unit, narrowing
    constexpr ns2::kilometre km = 5;
    constexpr ns2::metre m = km;
    static_assert(m.value() == 5000, "");
  }

  { // constructor from derived unit, expanding
    constexpr ns2::metre m = 5000;
    constexpr ns2::kilometre km = m;
    static_assert(km.value() == 5, "");
  }

  { // addition
    constexpr ns2::metre m1 = 3, m2 = 2;
    constexpr ns2::metre m3 = m1 + m2;
    static_assert(m3.value() == 5, "");
  }

  { // subtraction
    constexpr ns2::metre m1 = 8, m2 = 3;
    constexpr ns2::metre m3 = m1 - m2;
    static_assert(m3.value() == 5, "");
  }

  { // left scalar multiplication
    constexpr ns2::metre m1 = 3;
    constexpr ns2::metre m2 = 2 * m1;
    static_assert(m2.value() == 6, "");
  }

  { // right scalar multiplication
    constexpr ns2::metre m1 = 3;
    constexpr ns2::metre m2 = m1 * 2;
    static_assert(m2.value() == 6, "");
  }

  { // right scalar division
    constexpr ns2::metre m1 = 10;
    constexpr ns2::metre m2 = m1 / 2;
    static_assert(m2.value() == 5, "");
  }

  { // comparison
    constexpr ns2::metre m1 = 2, m2 = 3;
    static_assert(m1 < m2, "");
    static_assert(m1 <= m1, "");
    static_assert(m1 <= m2, "");
    static_assert(m2 > m1, "");
    static_assert(m1 >= m1, "");
    static_assert(m2 >= m1, "");
  }

  { // comparison between derived units
    constexpr ns2::metre m = 999;
    constexpr ns2::kilometre km = 1;
    static_assert(m < km, "");
    static_assert(m <= km, "");
    static_assert(km > m, "");
    static_assert(km >= m, "");
  }

  { // function overloading
    constexpr struct {
      constexpr bool operator()(ns2::metre) const {return true;}
      constexpr bool operator()(ns2::litre) const {return false;}
    } f {}; // Clang < 3.9 can't initialise without braces
    constexpr ns2::metre m;
    constexpr ns2::litre l;
    static_assert(f(m), "");
    static_assert(!f(l), "");
  }

  // Composition

  { // square composition T1 * T1
    constexpr ns2::metre m = 5, n = 4;
    constexpr auto sq_m = m * n;
    static_assert(sq_m.value() == 20, "");
    static_assert(std::is_same<decltype(sq_m), const ns2::metre::pow<2>>::value, "");
  }

  { // cube composition T1 * T1 * T1
    constexpr ns2::metre m = 5, n = 4, o = 3;
    constexpr auto sq_m = m * n;
    constexpr auto cb_m = sq_m * o;
    static_assert(cb_m.value() == 60, "");
    static_assert(std::is_same<decltype(cb_m), const ns2::metre::pow<3>>::value, "");
  }

  { // different types composition T1 * T2
    constexpr ns2::metre m = 5;
    constexpr ns2::litre l = 4;
    constexpr auto ml = m * l;
    static_assert(ml.value() == 20, "");
    static_assert(std::is_same<decltype(ml), const ns2::compose<ns2::metre, ns2::litre>>::value, "");
  }

  { // different types composition T1 * T2 * T1 -> T1^2 * T2
    constexpr ns2::metre m = 5, n = 4;
    constexpr ns2::litre l = 3;
    constexpr auto sq_ml = m * l * n;
    static_assert(sq_ml.value() == 60, "");
    static_assert(std::is_same<decltype(sq_ml), const ns2::compose<ns2::metre::pow<2>, ns2::litre>>::value, "");
  }

  { // inversion composition T1 / T2
    constexpr ns2::metre m = 5;
    constexpr ns2::second s = 10;
    constexpr auto ms = m / s;
    static_assert(ms.value() == 0.5, "");
    static_assert(std::is_same<decltype(ms), const ns2::compose<ns2::metre, ns2::over<ns2::second>>>::value, "");
  }

  { // construction from logically same type (T1 * T2 <=> T2 * T1)
    constexpr ns2::compose<ns2::metre, ns2::second> ms = 5;
    constexpr ns2::compose<ns2::second, ns2::metre> sm = ms;
    static_assert(sm.value() == 5, "");
    static_assert(! std::is_same<decltype(ms), decltype(sm)>::value, "");
  }

  { // complex conversion
    constexpr ns2::metre::over<ns2::second> ms = 5;
    constexpr ns2::kilometre::over<ns2::hour> kh = ms;
    static_assert(kh.value() == 5 * 3600 / 1000, "");
  }

  { // square conversion
    constexpr ns2::kilometre::pow<2> sq_km = 5;
    constexpr ns2::metre::pow<2> sq_m = sq_km;
    static_assert(sq_m.value() == 5000000, "");
  }

  { // composition
    static_assert(std::is_same<ns2::metre::over<ns2::second>,
                               ns2::compose<ns2::metre, ns2::over<ns2::second>>>::value, "");
    static_assert(std::is_same<ns2::metre::dot<ns2::second>,
                               ns2::compose<ns2::metre, ns2::second>>::value, "");
  }

  ////////////////////

  {
    constexpr ns2::compose<ns2::metre, ns2::second> m = 6;
    constexpr ns2::compose<ns2::second, ns2::metre> n = m;
    static_assert(m.value() == n.value(), "");
  }

  {
    static_assert(std::is_same<ns2::second::pow<2>, ns2::compound_unit<double, ns2::unit<struct ns2::second_tag, 2>>>::value, "");
  }

  {
    constexpr ns2::metre m = 2;
    constexpr ns2::second s = 4;
    constexpr auto m_s = m * s;
    static_assert(std::is_same<const ns2::compound_unit<double, ns2::unit<struct ns2::metre_tag>, ns2::unit<ns2::second_tag>>, decltype(m_s)>::value, "");
    static_assert(m_s.value() == 8, "");

    constexpr auto m_sq_s = m_s * s;
    static_assert(std::is_same<const ns2::compound_unit<double, ns2::unit<struct ns2::metre_tag>, ns2::unit<ns2::second_tag, 2>>, decltype(m_sq_s)>::value, "");
  }

  {
    constexpr ns2::metre m = 2;
    constexpr ns2::second s = 4;
    constexpr auto m_s = m / s;
    static_assert(std::is_same<const ns2::compound_unit<double, ns2::unit<struct ns2::metre_tag>, ns2::unit<ns2::second_tag, -1>>, decltype(m_s)>::value, "");
    static_assert(std::is_same<const ns2::compose<ns2::metre, ns2::over<ns2::second>>, decltype(m_s)>::value, "");
    static_assert(m_s.value() == 0.5, "");
  }

  {
    static_assert(ns2::type_list<float,int, char>::equiv<ns2::type_list<int, char, float>>::value, "");
  }

  {
    constexpr auto d = ns2::millilitre(1) / ns2::second(1);
    using litre_hour = decltype(ns2::litre(1) / ns2::hour(1));
    constexpr litre_hour dd = d;
    static_assert(dd.value() == 3.6, "");
  }

  {
    constexpr ns2::metre::pow<3> m3 = 1;
    constexpr ns2::millimetre::pow<3> mm3 = m3;
    static_assert(mm3.value() == 1000000000, "");
  }

}
