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

ConstantWeightShortestPathFinder::PathSnippet::PathSnippet(VertexRef& pred,
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
  VertexRef start(s);
  VertexRef end(e);
  // Init
  if (start == end) {
    result._vertices.emplace_back(start);
    _options.fetchVerticesCoordinator(result._vertices);
    return true;
  }
  resetSearch();

  _leftFound.emplace(start, FoundVertex(true));
  _rightFound.emplace(end, FoundVertex(true));
  _leftClosure.emplace_back(start);
  _rightClosure.emplace_back(end);

  TRI_IF_FAILURE("TraversalOOMInitialize") {
    THROW_ARANGO_EXCEPTION(TRI_ERROR_DEBUG);
  }

  // nodes from which paths can be reconstructed using snippets
  std::vector<VertexRef> nodes;
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
    size_t maxPaths, std::function<void()> const& callback) {
  if (start == end) {
    _nPaths = 1;
    return _nPaths;
  }

  // Init
  resetSearch();

  _leftFound.emplace(start, FoundVertex(true));
  _rightFound.emplace(end, FoundVertex(true));
  _leftClosure.emplace_back(start);
  _rightClosure.emplace_back(end);

  TRI_IF_FAILURE("TraversalOOMInitialize") {
    THROW_ARANGO_EXCEPTION(TRI_ERROR_DEBUG);
  }

  while (!_leftClosure.empty() && !_rightClosure.empty()) {
    callback();

    if (_leftClosure.size() < _rightClosure.size()) {
      if (0 < expandClosure(_leftClosure, _leftFound, _rightFound, false, _joiningNodes)) {
        computeNrPaths(_joiningNodes);
        preparePathIteration();
        return _nPaths;
      }
    } else {
      if (0 < expandClosure(_rightClosure, _rightFound, _leftFound, true, _joiningNodes)) {
        computeNrPaths(_joiningNodes);
        preparePathIteration();
        return _nPaths;
      }
    }
  }
  _nPaths = 0;
  return _nPaths;
}

bool ConstantWeightShortestPathFinder::getNextPath(arangodb::graph::ShortestPathResult& path) {
  if (!_firstPath) {
    advancePathIterator();
  }
  if (_currentJoiningNode == _joiningNodes.end()) {
    return false;
  }

  _firstPath = false;
  path._vertices.emplace_back(*_currentJoiningNode);

  auto it = _leftFound.find(*_currentJoiningNode);
  TRI_ASSERT(it != _leftFound.end());

  _leftTrace.clear();
  _leftTrace.push_back(*_currentJoiningNode);
  while (it->second._startOrEnd == false) {
    VertexRef next;

    next = it->second._tracer->_pred;
    path._vertices.push_front(next);
    _leftTrace.push_back(next);
    path._edges.push_front(std::move(it->second._tracer->_path));
    it = _leftFound.find(next);
  }

  _rightTrace.clear();
  it = _rightFound.find(*_currentJoiningNode);
  TRI_ASSERT(it != _rightFound.end());
  while (it->second._startOrEnd == false) {
    VertexRef next;

    next = it->second._tracer->_pred;
    _rightTrace.push_back(next);
    path._vertices.emplace_back(next);
    path._edges.emplace_back(std::move(it->second._tracer->_path));
    it = _rightFound.find(next);
  }

  TRI_IF_FAILURE("TraversalOOMPath") {
    THROW_ARANGO_EXCEPTION(TRI_ERROR_DEBUG);
  }
  _options.fetchVerticesCoordinator(path._vertices);
  return true;
}

size_t ConstantWeightShortestPathFinder::expandClosure(
    Closure& sourceClosure, FoundVertices& foundFromSource,
    FoundVertices& foundToTarget, bool direction, std::vector<VertexRef>& result) {
  _nextClosure.clear();
  result.clear();
  for (auto& v : sourceClosure) {
    _edges.clear();
    _neighbors.clear();
    auto foundv = foundFromSource.find(v);
    // v should be in foundFromSource, because
    // it is in sourceClosure
    TRI_ASSERT(foundv != foundFromSource.end());
    auto depth = foundv->second._depth;
    auto pathsToV = foundv->second._npaths;

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
      auto inserted = foundFromSource.emplace(n, FoundVertex(false, depth + 1, pathsToV));
      auto& w = inserted.first->second;
      w._snippets.emplace_back(snippet);

      // If we know this vertex and it is at the frontier, we found more paths
      if (!inserted.second && w._depth == depth + 1) {
        w._npaths += pathsToV;
      }

      auto found = foundToTarget.find(n);
      if (found != foundToTarget.end()) {
        // This is a path joining node, but we do not know
        // yet whether we added all paths to it, so we have
        // to finish computing the closure.
        result.emplace_back(n);
      }
      // vertex was new
      if (inserted.second) {
        _nextClosure.emplace_back(n);
      }
    }
  }
  _edges.clear();
  _neighbors.clear();
  sourceClosure.swap(_nextClosure);
  _nextClosure.clear();
  return result.size();
}

void ConstantWeightShortestPathFinder::fillResult(VertexRef& n,
                                                  arangodb::graph::ShortestPathResult& result) {
  result._vertices.emplace_back(n);
  auto it = _leftFound.find(n);
  TRI_ASSERT(it != _leftFound.end());
  VertexRef next;
  while (it != _leftFound.end() && (it->second._startOrEnd == false)) {
    next = it->second._snippets.at(0)._pred;
    result._vertices.push_front(next);
    result._edges.push_front(std::move(it->second._snippets.at(0)._path));
    it = _leftFound.find(next);
  }
  it = _rightFound.find(n);
  TRI_ASSERT(it != _rightFound.end());
  while (it != _rightFound.end() && (it->second._startOrEnd == false)) {
    next = it->second._snippets.at(0)._pred;
    result._vertices.emplace_back(next);
    result._edges.emplace_back(std::move(it->second._snippets.at(0)._path));
    it = _rightFound.find(next);
  }

  TRI_IF_FAILURE("TraversalOOMPath") {
    THROW_ARANGO_EXCEPTION(TRI_ERROR_DEBUG);
  }
  _options.fetchVerticesCoordinator(result._vertices);
  resetSearch();
}

void ConstantWeightShortestPathFinder::computeNrPaths(std::vector<VertexRef>& joiningNodes) {
  size_t npaths = 0;

  for (auto& n : joiningNodes) {
    // Find the joining node in both left
    // and right
    auto lfv = _leftFound.find(n);
    auto rfv = _rightFound.find(n);

    npaths += lfv->second._npaths * rfv->second._npaths;
  }
  _nPaths = npaths;
}

void ConstantWeightShortestPathFinder::preparePathIteration(void) {
  _firstPath = true;
  _currentJoiningNode = _joiningNodes.begin();
  _leftTrace.clear();
  _rightTrace.clear();
  for (auto& i : _leftFound) {
    i.second._tracer = i.second._snippets.begin();
  }
  for (auto& i : _rightFound) {
    i.second._tracer = i.second._snippets.begin();
  }
}

void ConstantWeightShortestPathFinder::advancePathIterator(void) {
  // Try advancing the left hand side of the current
  // joining node
  bool advanced = false;

  auto advancer = [](std::deque<VertexRef>& trace, FoundVertices& found) {
    auto t = trace.rbegin();
    t++;  // skip start node
    while (t != trace.rend()) {
      auto& f = found.find(*t)->second;
      f._tracer++;
      if (f._tracer == f._snippets.end()) {
        f._tracer = f._snippets.begin();
        t++;
      } else {
        return true;
      }
    }
    return false;
  };

  // Advance left path
  advanced = advancer(_leftTrace, _leftFound);

  // If this did not advance, advance right path
  if (!advanced) {
    advanced = advancer(_rightTrace, _rightFound);
  }

  // If both sides of the current joining node are exhausted,
  // advance joining node, reset all tracers
  if (!advanced) {
    _currentJoiningNode++;
  }

  // Exhaustion of the path iterator is determined by
  // _currentJoiningNode being the end() of _joiningNodes
}

void ConstantWeightShortestPathFinder::expandVertex(bool backward, VertexRef vertex) {
  std::unique_ptr<EdgeCursor> edgeCursor;
  if (backward) {
    edgeCursor.reset(_options.nextReverseCursor(vertex));
  } else {
    edgeCursor.reset(_options.nextCursor(vertex));
  }

  auto callback = [&](EdgeDocumentToken&& eid, VPackSlice edge, size_t cursorIdx) -> void {
    if (edge.isString()) {
      if (edge.compareString(vertex.data(), vertex.length()) != 0) {
        VertexRef id = _options.cache()->persistString(VertexRef(edge));
        _edges.emplace_back(std::move(eid));
        _neighbors.emplace_back(id);
      }
    } else {
      VertexRef other(transaction::helpers::extractFromFromDocument(edge));
      if (other == vertex) {
        other = VertexRef(transaction::helpers::extractToFromDocument(edge));
      }
      if (other != vertex) {
        VertexRef id = _options.cache()->persistString(other);
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
  _nPaths = 0;
}
