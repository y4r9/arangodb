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

#ifndef ARANGOD_GRAPH_EDGE_ITERATOR_H
#define ARANGOD_GRAPH_EDGE_ITERATOR_H 1

namespace arangodb {
namespace graph {
/**
 * This class is supposed to iterate over a list
 * of edges connected to a vertex at a given depth.
 */
template <class DataProvider>
class EdgeIterator {
  EdgeIterator() {}
  ~EdgeIterator() {}

  /**
   * Call the callback with next pair of Edge + ConnectedVertexId
   * Will return the getState value.
   */
  ExecutionState next(std::function<void(EdgeToken, StringRef)> const& callback);

  /**
   * Call the callback on all Edge + ConnectedVertexId tokens.
   * It guaranetees to only return
   * 1) DONE -> we are done
   * 2) WAITING -> did an async call, and needs to be reactivated after
   * response. In this case the callback might have been triggered.
   */
  ExecutionState all(std::function<void(EdgeToken, StringRef)> callback);

  /**
   * Returns the state of this iterator.
   * It can either be:
   * 1) DONE -> no more edges
   * 2) HASMORE -> more edges ask again
   * 3) WAITING -> did an async call and needs to be reactivated after response
   */
  ExecutionState getState() const;
};
}  // namespace graph
}  // namespace arangodb

#endif