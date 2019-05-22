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

#include "Graph/DataProvider.h"

namespace arangodb {
namespace tests {
class MockDataProvider : public graph::DataProvider {
  MockDataProvider() : DataProvider() {}

  ~MockDataProvider() = default;

  EdgeIterator<graph::DataProvider> connectedEdges(StringRef vertex, uint64_t depth) override {
    // TODO Implement me
    return EdgeIterator<this>();
  }
}
}  // namespace tests
}  // namespace arangodb