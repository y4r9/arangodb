////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2020-2020 ArangoDB GmbH, Cologne, Germany
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
/// @author Michael Hackstein
////////////////////////////////////////////////////////////////////////////////

#ifndef ARANGOD_GRAPH_K_SHORTEST_PATH_OPTIONS_H
#define ARANGOD_GRAPH_K_SHORTEST_PATH_OPTIONS_H 1

#include "Graph/ShortestPathOptions.h"

namespace arangodb {
namespace graph {
struct KShortestPathOptions : public ShortestPathOptions {
  double minWeight = std::numeric_limits<double>::min();
  double maxWeight = std::numeric_limits<double>::max();

  explicit KShortestPathOptions(aql::QueryContext& query)
      : ShortestPathOptions(query) {}
  ~KShortestPathOptions() = default;
};
}  // namespace graph
}  // namespace arangodb
#endif