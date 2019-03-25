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

#ifndef ARANGODB_GRAPH_CONSTANT_WEIGHT_SHORTEST_PATH_FINDER_H
#define ARANGODB_GRAPH_CONSTANT_WEIGHT_SHORTEST_PATH_FINDER_H 1

#include "Basics/VelocyPackHelper.h"
#include "Graph/EdgeDocumentToken.h"
#include "Graph/ShortestPathFinder.h"

#include <velocypack/StringRef.h>

namespace arangodb {

namespace velocypack {
class Slice;
}

namespace graph {

struct ShortestPathOptions;

class ConstantWeightShortestPathFinder : public ShortestPathFinder {
 private:
  // A path snippet contains an edge and a vertex
  // and is used to reconstruct the path
  struct PathSnippet {
    arangodb::velocypack::StringRef const _pred;
    graph::EdgeDocumentToken _path;

    PathSnippet(arangodb::velocypack::StringRef& pred, graph::EdgeDocumentToken&& path);
  };

  struct FoundVertex {
    // Number of paths to this vertex
    bool _startOrEnd;
    size_t _depth;
    size_t _npaths;

    // Predecessor edges
    std::vector<PathSnippet> _snippets;

    // Used to assemble paths
    std::vector<PathSnippet>::iterator _tracer;

    FoundVertex(void)
      : _startOrEnd(false), _depth(0), _npaths(0), _snippets({}){};
    FoundVertex(bool startOrEnd)  // _npaths is 1 for start/end vertices
        : _startOrEnd(startOrEnd), _depth(0), _npaths(1), _snippets({}){};
    FoundVertex(bool startOrEnd, size_t depth, size_t npaths)
        : _startOrEnd(startOrEnd), _depth(depth), _npaths(npaths), _snippets({}){};
  };
  typedef arangodb::velocypack::StringRef VertexRef;
  typedef std::deque<arangodb::velocypack::StringRef> Closure;

  // Contains the vertices that were found while searching
  // for a shortest path between start and end together with
  // the number of paths leading to that vertex and information
  // how to trace paths from the vertex from start/to end.
  typedef std::unordered_map<arangodb::velocypack::StringRef, FoundVertex> FoundVertices;

 public:
  explicit ConstantWeightShortestPathFinder(ShortestPathOptions& options);

  ~ConstantWeightShortestPathFinder();

  bool shortestPath(arangodb::velocypack::Slice const& start,
                    arangodb::velocypack::Slice const& end,
                    arangodb::graph::ShortestPathResult& result,
                    std::function<void()> const& callback) override;

  size_t kShortestPath(arangodb::velocypack::Slice const& start,
                       arangodb::velocypack::Slice const& end,
                       size_t maxPaths,
                       std::function<void()> const& callback);
  // Number of paths that were *computed* between start and end. Note
  // that this does not mean this reflects the total number of paths
  // between start and end
  size_t getNrPaths() { return _nPaths; };

  // get the next available path.
  size_t getNextPath(arangodb::graph::ShortestPathResult& path);

 private:
  void expandVertex(bool backward, arangodb::velocypack::StringRef vertex);

  void resetSearch();

  // returns the number of paths found
  size_t expandClosure(Closure& sourceClosure, FoundVertices& foundFromSource,
                       FoundVertices& foundToTarget, bool direction,
                       std::vector<arangodb::velocypack::StringRef>& result);

  void fillResult(arangodb::velocypack::StringRef& n,
                  arangodb::graph::ShortestPathResult& result);

  // Compute the number of paths found from a list of joining nodes
  void computeNrPaths(std::vector<arangodb::velocypack::StringRef>& joiningNodes);

  // Set all iterators in _leftFound and _rightFound to the beginning
  void preparePathIteration(void);

 private:
  FoundVertices _leftFound;
  Closure _leftClosure;

  FoundVertices _rightFound;
  Closure _rightClosure;

  size_t _nPaths;

  Closure _nextClosure;

  // The nodes where shortest paths join
  std::vector<arangodb::velocypack::StringRef> _joiningNodes;
  std::vector<arangodb::velocypack::StringRef>::iterator _currentJoiningNode;

  std::vector<arangodb::velocypack::StringRef> _neighbors;
  std::vector<graph::EdgeDocumentToken> _edges;
};

}  // namespace graph
}  // namespace arangodb
#endif
