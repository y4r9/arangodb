#include <string>

#include <Logger/LogMacros.h>

#include <gtest/gtest.h>
#include <immer/vector.hpp>
#include <mpark/patterns.hpp>


#include "Aql/ExecutionNode.h"

namespace {

TEST(FooTest, Bar) {
  const auto v0 = immer::vector<int>{};
  const auto v1 = v0.push_back(42);

  using namespace std::string_literals;
  auto const p = std::make_pair(v1[0], "world"s);

  using namespace mpark::patterns;
  IDENTIFIERS(str);
  LOG_DEVEL << "Hello, " << match(p)(pattern(ds(42, str)) = [](auto const& str) {
    return str;
  });
}

TEST(FooTest, canMatchOnExecutionNodeClass) {
  using namespace mpark::patterns;
  using namespace arangodb::aql;
  
  std::unique_ptr<ExecutionNode> node =
      std::make_unique<SingletonNode>(nullptr, ExecutionNodeId(1));
      
      auto id = ExecutionNodeId(1);

  const auto v0 = immer::vector<ExecutionNodeId>{};
  // const auto v1 = v0.push_back(node->getType());
const auto v1 = v0.push_back(id);

  IDENTIFIERS(n);

  auto didMatch = match(v1[0])(
    pattern(n) = [](auto const& n) {
      LOG_DEVEL << "NodeType: ";
      return true;
    }
    
        /*
          auto didMatch = match(*(node.get()))(
    pattern(as<SingletonNode>(n)) = [](auto n) -> bool {
      LOG_DEVEL << "NodeType: " << n.getTypeString();
      return true;
    },

    pattern(_) = [](auto const&) -> bool {
       //LOG_DEVEL << "NodeType: " << n.getTypeString();
      return false;
    }
    */
  );

  EXPECT_TRUE(didMatch);
}
}  // namespace
