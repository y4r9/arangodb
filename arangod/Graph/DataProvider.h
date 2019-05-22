////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2019 ArangoDB GmbH, Cologne, Germany
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

#ifndef ARANGOD_GRAPH_DATA_PROVIDER_H
#define ARANGOD_GRAPH_DATA_PROVIDER_H 1

namespace arangodb {

namespace velocypack {
class StringRef;
}
namespace graph {

template <class Provider>
class EdgeIterator;

/**
 * This class is an abstraction to access the Graph-Data
 * It should have implementations for Local and Cluster Case.
 * It should also abstract from the cursor specifics.
 * s.t. we can implement the graph algorithms independently
 */
class DataProvider {
 public:
  using Depth = uint64_t;

  DataProvider() {}

  virtual ~DataProvider() = default;

  /**
   * This will be the main function to be called.
   * It is suppossed to support the following guarantees:
   * 1) Edges from all Indexes/Shards/Cursors should be included
   * 2) Only edges matching given filter conditions should be included.
   * 3) If depth >= maximal depth the Iterator will be empty.
   *
   * @param vertex The source vertex edges should originate from
   * @param depth The depth of the source vertex in the path
   */
  virtual EdgeIterator<DataProvider> incidentEdges(velocypack::StringRef vertex,
                                                   Depth depth);
};

}  // namespace graph
}  // namespace arangodb

#endif
