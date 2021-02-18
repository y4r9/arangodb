////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2014-2021 ArangoDB GmbH, Cologne, Germany
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
/// @author Jan Steemann
////////////////////////////////////////////////////////////////////////////////

#ifndef ARANGOD_METRICS_METRICS_DEFINITIONS_H
#define ARANGOD_METRICS_METRICS_DEFINITIONS_H 1

#include <cstdint>
#include <string>
#include <type_traits>

namespace arangodb {
namespace metrics {

enum class Unit {
  Seconds,
  Milliseconds,
  Microseconds,
  Nanoseconds,
  Number,
};

enum class Type {
  Counter,
  Gauge,
  Histogram,
};

enum class Complexity {
  Simple,
  Medium,
  Advanced,
};

enum class Category {
  Agency,
  AQL,
  Network,
};

enum class ExposedBy : uint32_t {
  None = 0,        // metric exposed nowhere
  Single = 1,      // metric exposed by single server
  Coordinator =2,  // metric exposed by coordinator
  DBServer = 4,    // metric exposed by db server
  Agent = 8,       // metric exposed by agent

  Cluster = Coordinator | DBServer | Agent,

  All = Single | Cluster,
};

static constexpr inline std::underlying_type<ExposedBy>::type exposedBy() {
  return static_cast<std::underlying_type<ExposedBy>::type>(ExposedBy::None);
}

template <typename... Args>
static constexpr inline std::underlying_type<ExposedBy>::type exposedBy(ExposedBy eb, Args... args) {
  return (static_cast<std::underlying_type<ExposedBy>::type>(eb) | exposedBy(args...));
}

struct Helptext {
  char const* value;
};

struct Description {
  char const* value;
};

struct Threshold {
  char const* value;
};

struct Metric {
  Metric(std::string name, Helptext help,
         Description description, Threshold threshold,
         Unit unit, Type type, Category category, Complexity complexity,
         std::underlying_type<ExposedBy>::type exposedBy);

  std::string const name;
  Helptext const help;
  Description const description;
  Threshold const threshold;
  Unit const unit;
  Type const type;
  Category const category;
  Complexity const complexity;
  std::underlying_type<ExposedBy>::type const exposedBy;
};

}  // namespace metrics
}  // namespace arangodb

#endif
