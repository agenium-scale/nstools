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

#ifndef NS2_CONFIG_HPP
#define NS2_CONFIG_HPP

#include <climits>
#include <stdexcept>

namespace ns2 {

// Compilers
// ----------------------------------------------------------------------------

// GCC
#if defined(__GNUC__)
#define NS2_IS_GCC
#endif

// Clang
#if defined(__CLANG__)
#define NS2_IS_CLANG
#endif

// Microsoft Visual Studio
#if defined(_MSC_VER)
#define NS2_IS_MSVC
#endif

// Dll specs
// ----------------------------------------------------------------------------

#if defined(NS2_IS_MSVC)
#define NS_DLLEXPORT __declspec(dllexport)
#else
#define NS_DLLEXPORT
#endif

#if defined(NS2_IS_MSVC)
#define NS_DLLIMPORT __declspec(dllimport)
#else
#define NS_DLLIMPORT
#endif

#if defined(NS_COMPILING_LIBRARY)
#define NS_DLLSPEC NS_DLLEXPORT
#elif defined(NS_NO_DLLSPEC)
#define NS_DLLSPEC
#else
#define NS_DLLSPEC NS_DLLIMPORT
#endif

// Thread local storage
// ----------------------------------------------------------------------------

#ifdef NS2_IS_MSVC
#define NS2_TLS __declspec(thread)
#else
#define NS2_TLS __thread
#endif

// Pretty function name
// ----------------------------------------------------------------------------

#if defined(NS2_IS_GCC) || defined(NS2_IS_CLANG)
#define NS2_FUNCTION __PRETTY_FUNCTION__
#elif defined(NS2_IS_MSVC)
#define NS2_FUNCTION __FUNCSIG__
#else
#define NS2_FUNCTION __func__
#endif

// Integer type
// ----------------------------------------------------------------------------

#if defined(NS2_IS_MSVC)
#if defined(_M_AMD64)
typedef __int64 int_t;
#else
typedef __int32 int_t;
#endif
#else
typedef long int_t;
#endif

// ----------------------------------------------------------------------------
// C++ std determination

#ifdef NS2_IS_MSVC
#define NS2__cplusplus _MSVC_LANG
#else
#define NS2__cplusplus __cplusplus
#endif

#if NS2__cplusplus > 0 && NS2__cplusplus < 201103L
#define NS2_CXX 1998
#elif NS2__cplusplus >= 201103L && NS2__cplusplus < 201402L
#define NS2_CXX 2011
#elif NS2__cplusplus >= 201402L && NS2__cplusplus < 201703L
#define NS2_CXX 2014
#elif NS2__cplusplus >= 201703L
#define NS2_CXX 2017
#else
#define NS2_CXX 0
#endif

// ----------------------------------------------------------------------------

} // namespace ns2

#endif
