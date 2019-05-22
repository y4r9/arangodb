////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2019-2019 ArangoDB GmbH, Cologne, Germany
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

#ifndef TEST_GRAPH_TEST_DATA_PROVIDER_H
#define TEST_GRAPH_TEST_DATA_PROVIDER_H 1

#include <map>
#include <vector>

#include "Basics/StringUtils.h"

#include "Graph/DataProvider.h"
#include "Graph/EdgeDocumentToken.h"
#include "Graph/EdgeIterator.h"

#include <velocypack/Builder.h>
#include <velocypack/Iterator.h>
#include <velocypack/StringRef.h>

namespace arangodb {
namespace tests {

class MockDataProviderIterator {
 private:
  velocypack::ArrayIterator _pos;
  aql::ExecutionState _state;

  void update_state(aql::ExecutionState const state) { _state = state; }

 public:
  using Callback = std::function<void(graph::EdgeDocumentToken, velocypack::StringRef)>;
  MockDataProviderIterator(velocypack::Slice const& slice)
      : _pos{slice}, _state{aql::ExecutionState::HASMORE} {}

  aql::ExecutionState next(Callback const& cb) {
    if (not _pos.valid()) {
      update_state(aql::ExecutionState::DONE);
    } else {
      cb(graph::EdgeDocumentToken{*_pos}, (*_pos).get("_to").stringRef());
      ++_pos;
      update_state(aql::ExecutionState::HASMORE);
    }
    return getState();
  }

  aql::ExecutionState all(Callback const& cb) {
    while (getState() == aql::ExecutionState::HASMORE) {
      next(cb);
    }
    return getState();
  }

  aql::ExecutionState getState() const { return _state; }
};

class MockDataProvider {
 public:
  using Iterator = MockDataProviderIterator;
  using Depth = uint64_t;

 private:
  Depth _depth_limit;
  velocypack::Builder _graph;

  void add_edge_json(velocypack::Builder& builder, std::size_t const from,
                     std::size_t const to) {
    static auto num_edges = 0ul;
    velocypack::ObjectBuilder const scope{std::addressof(builder)};
    builder.add("_from", velocypack::Value{node_name(from)});
    builder.add("_to", velocypack::Value{node_name(to)});
    builder.add("_key", velocypack::Value{num_edges++});
  }

  velocypack::Builder load_graph() {
    auto raw = std::map<std::size_t, std::vector<std::size_t>>{};

    /**
              2
             / \
        0 - 1   4 - 5
             \ /
              3
    **/
    for (auto const pair : std::vector<std::pair<std::size_t, std::size_t>>{
             {0, 1}, {1, 2}, {1, 3}, {2, 4}, {3, 4}, {4, 5}}) {
      raw[pair.first].push_back(pair.second);
    }

    auto builder = velocypack::Builder{};
    auto const scope = velocypack::ObjectBuilder{std::addressof(builder)};
    velocypack::ObjectBuilder const object_scope{std::addressof(builder)};
    for (auto const& pos : raw) {
      auto const& from = pos.first;
      builder.add(velocypack::Value(node_name(from)));

      velocypack::ArrayBuilder const array_scope{std::addressof(builder)};
      for (auto const to : pos.second) {
        add_edge_json(builder, from, to);
      }
    }
    return builder;
  }

 public:
  MockDataProvider() : _depth_limit{3}, _graph(load_graph()) {}

  Iterator incidentEdges(velocypack::StringRef vertex, Depth depth) {
    auto const slice = _graph.slice().get(vertex);
    if (depth >= _depth_limit or slice.isNone()) {
      return {velocypack::Slice::emptyArraySlice()};
    }
    return {slice};
  }

  std::string node_name(std::size_t const id) {
    return std::string{"node"} + basics::StringUtils::itoa(id);
  }
};

}  // namespace tests
}  // namespace arangodb

#endif
