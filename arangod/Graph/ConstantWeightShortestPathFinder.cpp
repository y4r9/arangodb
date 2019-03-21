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
/// @author Michael Hackstein
////////////////////////////////////////////////////////////////////////////////

#include "ConstantWeightShortestPathFinder.h"

#include "Cluster/ServerState.h"
#include "Graph/EdgeCursor.h"
#include "Graph/EdgeDocumentToken.h"
#include "Graph/ShortestPathOptions.h"
#include "Graph/ShortestPathResult.h"
#include "Graph/TraverserCache.h"
#include "Transaction/Helpers.h"
#include "Utils/OperationCursor.h"
#include "VocBase/LogicalCollection.h"

#include <velocypack/Iterator.h>
#include <velocypack/Slice.h>
#include <velocypack/StringRef.h>
#include <velocypack/velocypack-aliases.h>

using namespace arangodb;
using namespace arangodb::graph;

ConstantWeightShortestPathFinder::PathSnippet::PathSnippet(arangodb::velocypack::StringRef& pred,
                                                           EdgeDocumentToken&& path)
    : _pred(pred), _path(std::move(path)) {}

ConstantWeightShortestPathFinder::ConstantWeightShortestPathFinder(ShortestPathOptions& options)
    : ShortestPathFinder(options) {}

ConstantWeightShortestPathFinder::~ConstantWeightShortestPathFinder() {
  resetSearch();
}

bool ConstantWeightShortestPathFinder::shortestPath(
    arangodb::velocypack::Slice const& s, arangodb::velocypack::Slice const& e,
    arangodb::graph::ShortestPathResult& result, std::function<void()> const& callback) {
  result.clear();
  TRI_ASSERT(s.isString());
  TRI_ASSERT(e.isString());
  arangodb::velocypack::StringRef start(s);
  arangodb::velocypack::StringRef end(e);
  // Init
  if (start == end) {
    result._vertices.emplace_back(start);
    _options.fetchVerticesCoordinator(result._vertices);
    return true;
  }
  resetSearch();

  _leftFound.emplace(start, FoundVertex());
  _rightFound.emplace(end, FoundVertex());
  _leftClosure.emplace_back(start);
  _rightClosure.emplace_back(end);

  TRI_IF_FAILURE("TraversalOOMInitialize") {
    THROW_ARANGO_EXCEPTION(TRI_ERROR_DEBUG);
  }

  // nodes from which paths can be reconstructed using snippets
  std::vector<arangodb::velocypack::StringRef> nodes;
  while (!_leftClosure.empty() && !_rightClosure.empty()) {
    callback();

    if (_leftClosure.size() < _rightClosure.size()) {
      if (0 < expandClosure(_leftClosure, _leftFound, _rightFound, false, nodes)) {
        fillResult(nodes.at(0), result);
        return true;
      }
    } else {
      if (0 < expandClosure(_rightClosure, _rightFound, _leftFound, true, nodes)) {
        fillResult(nodes.at(0), result);
        return true;
      }
    }
  }
  return false;
}

size_t ConstantWeightShortestPathFinder::kShortestPath(
    arangodb::velocypack::Slice const& start, arangodb::velocypack::Slice const& end,
    size_t maxPaths, std::vector<arangodb::graph::ShortestPathResult>& result,
    std::function<void()> const& callback) {
  result.clear();

  if (start == end) {
    result.emplace_back(ShortestPathResult());
    result.at(0)._vertices.emplace_back(start);
    _options.fetchVerticesCoordinator(result.at(0)._vertices);

    return 1;
  }

  // Init
  resetSearch();

  _leftFound.emplace(start, FoundVertex());
  _rightFound.emplace(end, FoundVertex());
  _leftClosure.emplace_back(start);
  _rightClosure.emplace_back(end);

  TRI_IF_FAILURE("TraversalOOMInitialize") {
    THROW_ARANGO_EXCEPTION(TRI_ERROR_DEBUG);
  }

  std::vector<arangodb::velocypack::StringRef> nodes;
  while (!_leftClosure.empty() && !_rightClosure.empty()) {
    callback();

    if (0 < _leftClosure.size() < _rightClosure.size()) {
      if (expandClosure(_leftClosure, _leftFound, _rightFound, false, nodes)) {
        //        fillResult(n, result);
        return true;
      }
    } else {
      if (0 < expandClosure(_rightClosure, _rightFound, _leftFound, true, nodes)) {
        //        fillResult(n, result);
        return true;
      }
    }
  }
  return 0;
}

size_t ConstantWeightShortestPathFinder::expandClosure(
  Closure& sourceClosure, FoundVertices& foundFromSource, FoundVertices& foundToTarget,
  bool direction, std::vector<arangodb::velocypack::StringRef>& result) {
  size_t totalPaths = 0;

  _nextClosure.clear();
  result.clear();
  for (auto& v : sourceClosure) {
    _edges.clear();
    _neighbors.clear();
    auto foundv = foundFromSource.find(v);
    // v should be in foundFromSource, because
    // it is in sourceClosure
    TRI_ASSERT(foundv != foundFromSource.end());
    auto pathsToV = foundv->second.npaths;

    expandVertex(direction, v);  // direction is true iff the edge is traversed "backwards"
    size_t const neighborsSize = _neighbors.size();
    TRI_ASSERT(_edges.size() == neighborsSize);

    for (size_t i = 0; i < neighborsSize; ++i) {
      auto const& n = _neighbors[i];

      // NOTE: _edges[i] stays intact after move
      // and is reset to a nullptr. So if we crash
      // here no mem-leaks. or undefined behavior
      // Just make sure _edges is not used after
      auto snippet = PathSnippet(v, std::move(_edges[i]));
      auto inserted = foundFromSource.emplace(n, FoundVertex(1, {  }));
      auto &w = inserted.first->second;

      w.npaths += pathsToV;
      w.snippets.emplace_back(snippet);

      auto found = foundToTarget.find(n);
      if (found != foundToTarget.end()) {
        result.emplace_back(n);
        totalPaths += w.npaths + found->second.npaths;
        if (totalPaths >= _options.getMaxPaths()) {
          return totalPaths;
        }
      }
      if (inserted.second) {
        _nextClosure.emplace_back(n);
      }
    }
  }
  _edges.clear();
  _neighbors.clear();
  sourceClosure.swap(_nextClosure);
  _nextClosure.clear();
  return 0;
}

void ConstantWeightShortestPathFinder::fillResult(arangodb::velocypack::StringRef& n,
                                                  arangodb::graph::ShortestPathResult& result) {
  result._vertices.emplace_back(n);
  auto it = _leftFound.find(n);
  TRI_ASSERT(it != _leftFound.end());
  arangodb::velocypack::StringRef next;
  while (it != _leftFound.end() && it->second.npaths > 0) {
    next = it->second.snippets.at(0)._pred;
    result._vertices.push_front(next);
    result._edges.push_front(std::move(it->second.snippets.at(0)._path));
    it = _leftFound.find(next);
  }
  it = _rightFound.find(n);
  TRI_ASSERT(it != _rightFound.end());
  while (it != _rightFound.end() && it->second.npaths > 0) {
    next = it->second.snippets.at(0)._pred;
    result._vertices.emplace_back(next);
    result._edges.emplace_back(std::move(it->second.snippets.at(0)._path));
    it = _rightFound.find(next);
  }

  TRI_IF_FAILURE("TraversalOOMPath") {
    THROW_ARANGO_EXCEPTION(TRI_ERROR_DEBUG);
  }
  _options.fetchVerticesCoordinator(result._vertices);
  resetSearch();
}

void ConstantWeightShortestPathFinder::expandVertex(bool backward, arangodb::velocypack::StringRef vertex) {
  std::unique_ptr<EdgeCursor> edgeCursor;
  if (backward) {
    edgeCursor.reset(_options.nextReverseCursor(vertex));
  } else {
    edgeCursor.reset(_options.nextCursor(vertex));
  }

  auto callback = [&](EdgeDocumentToken&& eid, VPackSlice edge, size_t cursorIdx) -> void {
    if (edge.isString()) {
      if (edge.compareString(vertex.data(), vertex.length()) != 0) {
        arangodb::velocypack::StringRef id = _options.cache()->persistString(arangodb::velocypack::StringRef(edge));
        _edges.emplace_back(std::move(eid));
        _neighbors.emplace_back(id);
      }
    } else {
      arangodb::velocypack::StringRef other(transaction::helpers::extractFromFromDocument(edge));
      if (other == vertex) {
        other = arangodb::velocypack::StringRef(transaction::helpers::extractToFromDocument(edge));
      }
      if (other != vertex) {
        arangodb::velocypack::StringRef id = _options.cache()->persistString(other);
        _edges.emplace_back(std::move(eid));
        _neighbors.emplace_back(id);
      }
    }
  };
  edgeCursor->readAll(callback);
}

void ConstantWeightShortestPathFinder::resetSearch() {
  _leftClosure.clear();
  _rightClosure.clear();
  _leftFound.clear();
  _rightFound.clear();
}
