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

#include "thread.hpp"

namespace thread {

// ----------------------------------------------------------------------------

#ifdef NS2_IS_MSVC

cpp_mutex_t::cpp_mutex_t() {
  mutex = CreateMutex(NULL, FALSE, NULL);
  if (mutex == NULL) {
    NS2_THROW(std::runtime_error, "Unable to initialize mutex");
  }
}

void cpp_mutex_t::lock() {
  if (WaitForSingleObject(mutex, INFINITE) != WAIT_OBJECT_0) {
    NS2_THROW(std::runtime_error, "Unable to lock mutex");
  }
}

// Cannot do error check as it can be called from dtors
void cpp_mutex_t::unlock() { ReleaseMutex(mutex); }

cpp_mutex_t::~cpp_mutex_t() { CloseHandle(mutex); }

#else

cpp_mutex_t::cpp_mutex_t() {
  int code = pthread_mutex_init(&mutex, NULL);
  if (code != 0) {
    NS2_THROW(std::runtime_error, "Unable to initialize mutex");
  }
}

void cpp_mutex_t::lock() {
  int code = pthread_mutex_lock(&mutex);
  if (code != 0) {
    NS2_THROW(std::runtime_error, "Unable to lock mutex");
  }
}

// Cannot do error check as it can be called from dtors
void cpp_mutex_t::unlock() { pthread_mutex_unlock(&mutex); }

cpp_mutex_t::~cpp_mutex_t() { pthread_mutex_destroy(&mutex); }

#endif

// ----------------------------------------------------------------------------

scoped_lock_t::scoped_lock_t(cpp_mutex_t *cpp_mutex) {
  cpp_mutex_ = cpp_mutex;
  cpp_mutex_->lock();
}

scoped_lock_t::~scoped_lock_t() { cpp_mutex_->unlock(); }

// ----------------------------------------------------------------------------

} // namespace thread
