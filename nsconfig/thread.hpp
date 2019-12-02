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

#ifndef THREAD_HPP
#define THREAD_HPP

#include <ns2/config.hpp>
#include <ns2/exception.hpp>

#ifdef NS2_IS_MSVC
#include <windows.h>
#else
#include <pthread.h>
#include <string.h>
#endif

#include <exception>
#include <stdexcept>
#include <vector>

namespace thread {

// ----------------------------------------------------------------------------

template <typename Output, typename Work> struct start_func_t {
  struct param_t {
    Output *output;
    Work *work;
    Output (*func)(Work *);
  };

#ifdef NS2_IS_MSVC
  static DWORD WINAPI start(LPVOID param)
#else
  static void *start(void *param)
#endif
  {
    param_t *param_ = (param_t *)param;
    try {
      *(param_->output) = param_->func(param_->work);
#ifdef NS2_IS_MSVC
      return 0;
#else
      return NULL;
#endif
    } catch (std::exception &e) {
      fprintf(stderr, "error: %s\n", e.what());
#ifdef NS2_IS_MSVC
      return -1;
#else
      return (void *)-1;
#endif
    }
  }
};

// ----------------------------------------------------------------------------

template <typename Container> struct work_t {
  Container data_;
  typedef typename Container::value_type value_type;
  typename Container::const_iterator it_;
#ifdef NS2_IS_MSVC
  HANDLE mutex_;

  work_t(Container const &wc) {
    data_ = wc;
    mutex_ = CreateMutex(NULL, FALSE, NULL);
    if (mutex_ == NULL) {
      NS2_THROW(std::runtime_error, "cannot create mutex");
    }
    it_ = data_.begin();
  }

  value_type const *next() {
    if (WaitForSingleObject(mutex_, INFINITE) == WAIT_OBJECT_0) {
      value_type const *ret;
      if (it_ != data_.end()) {
        ret = &(*it_);
        it_++;
      } else {
        ret = NULL;
      }
      if (ReleaseMutex(mutex_) == 0) {
        NS2_THROW(std::runtime_error, "cannot release mutex");
      }
      return ret;
    } else {
      NS2_THROW(std::runtime_error, "cannot acquire mutex");
    }
  }

  ~work_t() { CloseHandle(mutex_); }
#else
  pthread_mutex_t mutex_;

  work_t(Container const &wc) {
    data_ = wc;
    pthread_mutex_init(&mutex_, NULL);
    it_ = data_.begin();
  }

  value_type const *next() {
    int code = pthread_mutex_lock(&mutex_);
    if (code == 0) {
      value_type const *ret;
      if (it_ != data_.end()) {
        ret = &(*it_);
        it_++;
      } else {
        ret = NULL;
      }
      code = pthread_mutex_unlock(&mutex_);
      if (code != 0) {
        NS2_THROW(std::runtime_error,
                  std::string("cannot release mutex") + strerror(errno));
      }
      return ret;
    } else {
      NS2_THROW(std::runtime_error,
                std::string("cannot acquire mutex") + strerror(code));
    }
  }

  ~work_t() { pthread_mutex_destroy(&mutex_); }
#endif

private:
  work_t(work_t const &) {}
  work_t &operator=(work_t const &) {}
};

// ----------------------------------------------------------------------------

template <typename Output, typename Work>
inline void pool_work(std::vector<Output> *output, Output (*func)(Work *),
                      int nb_threads, Work *work) {

#ifdef NS2_IS_MSVC
  std::vector<HANDLE> threads(nb_threads);
#else
  std::vector<pthread_t> threads(nb_threads);
#endif

  typedef start_func_t<Output, Work> st_t;
  typedef typename st_t::param_t param_t;
  std::vector<param_t> params(nb_threads);
  output->resize(nb_threads);
  for (int i = 0; i < nb_threads; i++) {
    params[i].output = &(output->operator[](i));
    params[i].work = work;
    params[i].func = func;
#ifdef NS2_IS_MSVC
    threads[i] =
        CreateThread(NULL, 0, st_t::start, (LPVOID)&params[i], 0, NULL);
    if (threads[i] == NULL) {
      NS2_THROW(std::runtime_error, "cannot create thread");
    }
#else
    int code =
        pthread_create(&threads[i], NULL, st_t::start, (void *)&params[i]);
    if (code != 0) {
      NS2_THROW(std::runtime_error,
                "cannot create thread" + std::string(strerror(code)));
    }
#endif
  }

  for (int i = 0; i < nb_threads; i++) {
#ifdef NS2_IS_MSVC
    if (WaitForSingleObject(threads[i], INFINITE) == WAIT_FAILED) {
      NS2_THROW(std::runtime_error, "cannot wait for thread");
    }
    CloseHandle(threads[i]);
#else
    int code = pthread_join(threads[i], NULL);
    if (code != 0) {
      NS2_THROW(std::runtime_error,
                "cannot wait for thread" + std::string(strerror(errno)));
    }
#endif
  }
}

// ----------------------------------------------------------------------------

} // namespace thread

#endif
