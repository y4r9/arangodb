#include <string>

#include <Logger/LogMacros.h>

#include <gtest/gtest.h>
#include <immer/vector.hpp>
#include <mpark/patterns.hpp>

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

}  // namespace
