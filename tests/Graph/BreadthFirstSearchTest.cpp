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

#include "gtest/gtest.h"

#include "Aql/ExecutionState.h"
#include "Graph/DataProvider.h"
#include "Graph/EdgeIterator.h"
#include "Logger/Logger.h"

#include <velocypack/StringRef.h>

namespace arangodb {
namespace graph {
// TODO: move this into production cpp file
template class arangodb::graph::EdgeIterator<arangodb::graph::DataProvider>;
}  // namespace graph
}  // namespace arangodb

#include "TestDataProvider.h"

namespace arangodb {
namespace tests {
namespace breadth_first_search_test {

class BreadthFirstSearchTest : public ::testing::Test {
 protected:
  BreadthFirstSearchTest(){
      // Setup code
  };

  ~BreadthFirstSearchTest(){
      // Teardown code
  };
};

TEST_F(BreadthFirstSearchTest, it_has_a_short_description_of_the_test_name) {
  MockDataProvider provider;
  auto printer = [](graph::EdgeDocumentToken const token, velocypack::StringRef) {
    std::cout << "HUHU" << velocypack::Slice{token.vpack()}.toJson() << std::endl;
  };

  auto const origin = provider.node_name(1ul);
  velocypack::StringRef v(origin);
  auto iterator = provider.incidentEdges(v, 0);
  iterator.all(printer);

  ASSERT_EQ(iterator.getState(), aql::ExecutionState::DONE);
}

}  // namespace breadth_first_search_test
}  // namespace tests
}  // namespace arangodb
