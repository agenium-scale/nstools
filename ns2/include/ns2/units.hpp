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

#ifndef _UNITS_HPP_
#define _UNITS_HPP_

#include <ratio>
#include <cstdint>

#include "meta.hpp"

namespace ns2 {

namespace units {
namespace details {

/**
 * Checks whether Ratio is a specialisation of std::ratio
 */
template<class Ratio>
struct is_std_ratio : std::false_type {};

/**
 * Implementation.
 */
template<std::intmax_t n, std::intmax_t d>
struct is_std_ratio<std::ratio<n, d>> : std::true_type {};

/**
 * Integer pow computation at compile time for unit ratio dimensions
 *
 * Warning: negative powers return 0.
 */
constexpr std::intmax_t pow(std::intmax_t I, std::intmax_t n) {
  return I < 0 ? 0 : (I == 0 ? 1 : n * pow(I - 1, n));
}

/**
 * Apply a dimension to a std::ratio at compile time.
 *
 * Use the `type` type definition to get the result.
 */
template<std::intmax_t I, class Ratio>
struct ratio_pow {
  using type =
    typename std::conditional<(I >= 0), std::ratio<pow(I, Ratio::num), pow(I, Ratio::den)>,
                              std::ratio<pow(-I, Ratio::den), pow(-I, Ratio::num)>>::type;

  constexpr operator double() const {
    return static_cast<double>(type::num) / type::den;
  }
};

/**
 * Constexpr evaluation of variadic multiplication for doubles.
 */
constexpr double mul() {
  return 1;
}

/**
 * Implementation. Recursion case
 */
template<class... Ts>
constexpr double mul(double a, Ts... as) {
  return a * mul(as...);
}

/**
 * Dummy template type to work around the fact that old clang have a problem with
 * using a partial specialisation inside a decltype.
 */
template<class T, class U>
struct dummy {
  using type = T;
};

} // namespace details
} // namespace units

/**
 * Decsription type that identifies a unit, its dimension and the ratio it has to
 * its base unit.
 *
 * Example:
 *   - metre is the base unit, it has a dimension of 1 and a ratio of 1.
 *   - millimetre is a derived unit, it has a dimension of one and a ratio of 1/1000.
 *   - square millimetre is a derived unit, it has a dimension of 2 and a ratio of 1/1000
 *
 * \tparam Tag The unit base tag dispatcher
 * \tparam Pow The unit dimension
 * \tparam Ratio A std::ratio specialisation gving the ratio between the unit and its base unit.
 */
template<class Tag, int Pow = 1, class Ratio = std::ratio<1>>
struct unit {
  static_assert(units::details::is_std_ratio<Ratio>::value,
                "Ratio template parameter must be std::ratio");

  using tag_t = Tag;
  using ratio_t = Ratio;
  enum { pow_v = Pow };

  template<int P>
  using pow = unit<Tag, P, Ratio>;
};

/**
 * Compute the conversion factor from the first ratio to the second
 */
template<std::intmax_t P, std::intmax_t n1, std::intmax_t d1, std::intmax_t n2, std::intmax_t d2>
constexpr double conversion_factor(std::ratio<n1, d1>, std::ratio<n2, d2>) {
  return units::details::ratio_pow<P, std::ratio<n1 * d2, d1 * n2>>{};
}

/**
 * Compute conversion ratio between two ratio lists taking their dimension into account
 */
template<int... Ps, class... Ratios1, class... Ratios2>
static constexpr double conversion_factor(type_list<Ratios1...>, type_list<Ratios2...>) {
  return units::details::mul(conversion_factor<Ps>(Ratios1{}, Ratios2{})...);
}

/**
 * Selects a `unit` specialisation that corresponds to the given Tag
 *
 * \tparam Tag Base unit tag
 * \tparam Units Pack of specialisations of `unit`
 *
 * Note: a compilation error occurs if no type matches.
 */
template<class Tag, class U, class... Units>
struct select_unit : select_unit<Tag, Units...> {};

/**
 * Implementation. Recursion end.
 */
template<class Tag, class Ratio, int Pow, class... Units>
struct select_unit<Tag, unit<Tag, Pow, Ratio>, Units...> {
  using type = unit<Tag, Pow, Ratio>;
};

/**
 * Main unit representation type.
 *
 * A unit is the composition of unit descriptors. For example speed may be
 * represented using metres and seconds or kilometres and hours.
 *
 * \tparam Rep The storage representation
 * \tparam Units A pack of unit specialisations. No two unit must have the same
 * base tag.
 */
template<class Rep, class... Units>
class compound_unit;

template<class Rep, class... Tags, int... Pows, class... Ratios>
class compound_unit<Rep, unit<Tags, Pows, Ratios>...> {
  Rep m_value;

public:
  template<int P>
  using pow = compound_unit<Rep, typename unit<Tags, Pows, Ratios>::template pow<P>...>;

  /**
   * List of the units
   */
  using units_t = type_list<unit<Tags, Pows, Ratios>...>;
  /**
   * List of the unit tags
   */
  using tags_t = type_list<Tags...>;
  /**
   * List of the unit ratios
   */
  using ratios_t = type_list<Ratios...>;

  template<class Unit>
  using dot = decltype(typename units::details::dummy<compound_unit, Unit>::type{} * Unit{});

  template<class Unit>
  using over = decltype(typename units::details::dummy<compound_unit, Unit>::type{} / Unit{});

  /**
   * Default constructor
   */
  constexpr compound_unit()
    : m_value() {
  }
  /**
   * Constructor from value
   */
  constexpr compound_unit(Rep value)
    : m_value(value) {
  }

private:
  /**
   * Helper constructor from other unit with same base descriptor composition and other ratios
   *
   * GCC 4.9 has a bug that prevents template argument deduction and causes an
   * ICE. This constructor helps working around that problem.
   */
  template<class... Ratios2>
  constexpr compound_unit(type_list<Ratios2...>, Rep other)
    : m_value(other * conversion_factor<Pows...>(type_list<Ratios2...>{}, type_list<Ratios...>{})) {
  }

  /**
   * Reorder unit descriptors to match this compound unit order
   */
  template<class... Units>
  using reordered_t = compound_unit<Rep, typename select_unit<Tags, Units...>::type...>;

public:
  /**
   * Copy constructor from other unit with equivalent descriptor composition
   *
   * A compile-time check ensures that this constructor only takes part in
   * overload resolution when the given compound_unit specialisation has a unit
   * list that is equivalent to this compound_unit specialisation unit list.
   */
  template<class... Tags2, int... Pows2, class... Ratios2,
           // SFINAE check for equivalence between this unit list and the other unit list
           class = typename std::enable_if<type_list<unit<Tags2, Pows2>...>::template equiv<
             type_list<unit<Tags, Pows>...>>::value>::type>
  constexpr compound_unit(const compound_unit<Rep, unit<Tags2, Pows2, Ratios2>...>& other)
    : compound_unit(typename reordered_t<unit<Tags2, Pows2, Ratios2>...>::ratios_t{},
                    other.value()) {
  }

  /**
   * Access stored value
   */
  constexpr Rep value() const {
    return m_value;
  }

  /**
   * Add an other value
   */
  constexpr compound_unit operator+(const compound_unit& o) const {
    return {value() + o.value()};
  }

  /**
   * Subtract an other value
   */
  constexpr compound_unit operator-(const compound_unit& o) const {
    return {value() - o.value()};
  }

  /**
   * Multiply by a scalar
   */
  friend constexpr compound_unit operator*(const compound_unit& a, Rep b) {
    return {a.value() * b};
  }

  /**
   * Multiply by a scalar
   */
  friend constexpr compound_unit operator*(Rep a, const compound_unit& b) {
    return {a * b.value()};
  }

  /**
   * Divide by a scalar
   */
  friend constexpr compound_unit operator/(const compound_unit& a, Rep b) {
    return {a.value() / b};
  }

  /**
   * Less than comparison operator
   */
  constexpr bool operator<(const compound_unit& a) const {
    return value() < a.value();
  }

  /**
   * Less or equal comparison operator
   */
  constexpr bool operator<=(const compound_unit& a) const {
    return value() <= a.value();
  }

  /**
   * Greater than comparison operator
   */
  constexpr bool operator>(const compound_unit& a) const {
    return value() > a.value();
  }

  /**
   * Greater or equal comparison operator
   */
  constexpr bool operator>=(const compound_unit& a) const {
    return value() >= a.value();
  }
};

namespace details {

/**
 * Check whether a unit descriptor has a dimension of zero
 */
template<class T>
struct has_zero_pow : std::false_type {};

/**
 * Implementation. True case.
 */
template<class Tag, class Ratio>
struct has_zero_pow<unit<Tag, 0, Ratio>> : std::true_type {};

/**
 * Compose two units together. Fuse type that are the same, add unique types.
 *
 * Example: km/s * m -> square_km / s
 */
template<class Rep, class DoneList, class TodoList, class... Us>
struct unit_composition;

/**
 * Implementation. Final step : remove unit if power is zero
 */
template<class Rep, class List, class... Us>
struct unit_composition<Rep, List, type_list<>, Us...> {
  template<class... Ts>
  using final_unit = compound_unit<Rep, Ts...>;

  using type = typename List::template append<Us...>::template filtered<
    has_zero_pow>::template apply<final_unit>;

  static constexpr double ratio() {
    return 1;
  }
};

/**
 * Implementation. No matching type for D and no more types to test, add D to Ts
 * and start with next type
 */
template<class Rep, class... Ts, class D, class... Ds>
struct unit_composition<Rep, type_list<Ts...>, type_list<D, Ds...>>
  : unit_composition<Rep, type_list<>, type_list<Ds...>, Ts..., D> {};

/**
 * Implementation. Matching type for the head. Add pows to final list and restart with next type
 */
template<class Rep, class... Ts, class... Ds, class Tag, int Pow1, int Pow2, class Ratio1,
         class Ratio2, class... Us>
struct unit_composition<Rep, type_list<Ts...>, type_list<unit<Tag, Pow1, Ratio1>, Ds...>,
                        unit<Tag, Pow2, Ratio2>, Us...>
  : unit_composition<Rep, type_list<>, type_list<Ds...>, Ts..., unit<Tag, Pow1 + Pow2, Ratio1>,
                     Us...> {
  static constexpr double ratio() {
    using parent = unit_composition<Rep, type_list<>, type_list<Ds...>, Ts...,
                                    unit<Tag, Pow1 + Pow2, Ratio1>, Us...>;
    return parent::ratio() * conversion_factor<Pow1>(Ratio2{}, Ratio1{});
  }
};

/**
 * Implementation. Head does not match D, check next type.
 */
template<class Rep, class... Ts, class D, class... Ds, class U, class... Us>
struct unit_composition<Rep, type_list<Ts...>, type_list<D, Ds...>, U, Us...>
  : unit_composition<Rep, type_list<Ts..., U>, type_list<D, Ds...>, Us...> {};

} // namespace details

/**
 * Compose two units
 */
template<class Rep, class... Us, class... Ts>
constexpr typename details::unit_composition<Rep, type_list<>, type_list<Ts...>, Us...>::type
operator*(compound_unit<Rep, Us...> a, compound_unit<Rep, Ts...> b) {
  return {a.value() * b.value()
          * details::unit_composition<Rep, type_list<>, type_list<Ts...>, Us...>::ratio()};
}

/**
 * Invert a unit dimension.
 *
 * Example: km/s -> unit<km>, inv<unit<second>>
 */
template<class T>
struct inv;

/**
 * Implementation.
 */
template<class Tag, int Pow, class Ratio>
struct inv<unit<Tag, Pow, Ratio>> {
  using type = unit<Tag, -Pow, Ratio>;
};

/**
 * Implementation.
 */
template<class... Tag, class Rep, int... Pow, class... Ratio>
struct inv<compound_unit<Rep, unit<Tag, Pow, Ratio>...>> {
  using type = compound_unit<Rep, unit<Tag, -Pow, Ratio>...>;
};

/**
 * Unit composition when dividing.
 */
template<class Rep, class... Us, class... Ts>
constexpr typename details::unit_composition<Rep, type_list<>, type_list<typename inv<Ts>::type...>,
                                             Us...>::type
operator/(compound_unit<Rep, Us...> a, compound_unit<Rep, Ts...> b) {
  return {a.value() / b.value()
          * details::unit_composition<Rep, type_list<>, type_list<typename inv<Ts>::type...>,
                                      Us...>::ratio()};
}

/**
 * Shorthand to create composed types from existing ones.
 */
template<class Unit1, class Unit2>
using compose = decltype(Unit1{} * Unit2{});

/**
 * Shorthand to use the inv template
 *
 * Example: km/h -> compose<kilometre, over<hour>>
 */
template<class Unit>
using over = typename inv<Unit>::type;

using metre = compound_unit<double, unit<struct metre_tag>>;
using decimetre = compound_unit<double, unit<struct metre_tag, 1, std::deci>>;
using centimetre = compound_unit<double, unit<struct metre_tag, 1, std::centi>>;
using millimetre = compound_unit<double, unit<struct metre_tag, 1, std::milli>>;
using decametre = compound_unit<double, unit<struct metre_tag, 1, std::deca>>;
using hectometre = compound_unit<double, unit<struct metre_tag, 1, std::hecto>>;
using kilometre = compound_unit<double, unit<struct metre_tag, 1, std::kilo>>;

using second = compound_unit<double, unit<struct second_tag>>;
using millisecond = compound_unit<double, unit<struct second_tag, 1, std::milli>>;
using minute = compound_unit<double, unit<struct second_tag, 1, std::ratio<60>>>;
using hour = compound_unit<double, unit<struct second_tag, 1, std::ratio<3600>>>;

using litre = compound_unit<double, unit<struct litre_tag>>;
using millilitre = compound_unit<double, unit<struct litre_tag, 1, std::milli>>;
using centilitre = compound_unit<double, unit<struct litre_tag, 1, std::centi>>;
using decilitre = compound_unit<double, unit<struct litre_tag, 1, std::deci>>;

} // namespace ns2

#endif /* _UNITS_HPP_ */
