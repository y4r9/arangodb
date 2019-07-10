////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2014-2016 ArangoDB GmbH, Cologne, Germany
/// Copyright 2004-2014 triAGENS GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Dr. Frank Celler
////////////////////////////////////////////////////////////////////////////////

#ifndef ARANGODB_BASICS_THREADS__WIN32_H
#define ARANGODB_BASICS_THREADS__WIN32_H 1

#include "Basics/Common.h"

////////////////////////////////////////////////////////////////////////////////
/// @brief process identifier
////////////////////////////////////////////////////////////////////////////////

#define TRI_pid_t int

////////////////////////////////////////////////////////////////////////////////
/// @brief thread identifier
////////////////////////////////////////////////////////////////////////////////

#define TRI_tid_t DWORD

////////////////////////////////////////////////////////////////////////////////
/// @brief thread
////////////////////////////////////////////////////////////////////////////////

#define TRI_thread_t HANDLE
#include <chrono>
#include <thread>
namespace arangodb {
namespace basics {
  // Lol microsoft https://developercommunity.visualstudio.com/content/problem/61684/stdthis-threadsleep-for-depends-on-system-time.html 
  template< class Rep, class Period >
    void sleep_for(const std::chrono::duration<Rep, Period>& sleep_duration) {
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(sleep_duration).count();
    if (duration == 0) {
        Sleep(1);
    } else {
      Sleep(static_cast<DWORD>(duration));
    }
  }
}}


#endif
