// MIT License
//
// Copyright (c) 2019 Agenium Scale
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal
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
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE
// SOFTWARE.

#ifndef UNITS_META_HPP_
#define UNITS_META_HPP_

#include <type_traits>

namespace ns2 {

namespace meta {
namespace list {

/**
 * Inherits std::true_type if T is in Ts..., std::false_type otherwise
 */
template <class T, class... Ts> struct holds : std::false_type {};

/**
 * Implementation
 */
template <class T, class U, class... Us>
struct holds<T, U, Us...>
    : std::conditional<std::is_same<T, U>::value, std::true_type,
                       holds<T, Us...> >::type {};

/**
 * Returns given list without types that do not satisfy predicate.
 */
template <template <class T> class Pred, class List, class... Ts>
struct filter {
  using type = List;
};

/**
 * Implementation. Recursion.
 */
template <template <class T> class Pred, template <class...> class List,
          class... Ls, class T, class... Ts>
struct filter<Pred, List<Ls...>, T, Ts...>
    : filter<Pred, typename std::conditional<Pred<T>::value, List<Ls..., T>,
                                             List<Ls...> >::type,
             Ts...> {};

template <template <class> class Trans, class List> struct transform;

template <template <class> class Trans, template <class...> class List,
          class... Ts>
struct transform<Trans, List<Ts...> > {
  using type = List<typename Trans<Ts>::type...>;
};

template <template <class> class Trans, template <class> class Pred,
          class List>
struct transform_if;

template <template <class> class Trans, template <class> class Pred,
          template <class...> class List, class... Ts>
struct transform_if<Trans, Pred, List<Ts...> > {
  using type =
      List<typename std::conditional<Pred<Ts>::value, typename Trans<Ts>::type,
                                     Ts>::type...>;
};

/**
 * Check whether two lists hold the same types, not specially in the same order
 */
template <class List1, class List2a, class List2b>
struct equiv : std::false_type {};

template <template <class...> class List>
struct equiv<List<>, List<>, List<> > : std::true_type {};

template <template <class...> class List, class... Ts, class T, class... Us,
          class... Vs>
struct equiv<List<T, Ts...>, List<T, Us...>, List<Vs...> >
    : equiv<List<Ts...>, List<Vs..., Us...>, List<> > {};

template <template <class...> class List, class... Ts, class U, class... Us,
          class... Vs>
struct equiv<List<Ts...>, List<U, Us...>, List<Vs...> >
    : equiv<List<Ts...>, List<Us...>, List<Vs..., U> > {};

} // namespace ns2::meta::list
} // namespace ns2::meta

template <class... Ts> struct type_list {

  template <class T> using holds = meta::list::holds<T, Ts...>;

  template <template <class T> class Pred>
  using filtered =
      typename meta::list::filter<Pred, type_list, type_list<>, Ts...>::type;

  template <template <class...> class Tpl> using apply = Tpl<Ts...>;

  template <class... Us> using append = type_list<Ts..., Us...>;

  template <template <class> class Trans, template <class> class Pred>
  using transform_if =
      typename meta::list::transform_if<Trans, Pred, type_list<Ts...> >::type;

  template <class List>
  using equiv =
      typename meta::list::equiv<type_list<Ts...>, List, type_list<> >;

  enum { empty = sizeof...(Ts) != 0 };
  enum { size = sizeof...(Ts) };
};

} // namespace qk

#endif /* UNITS_META_HPP_ */
