#include "gtest/gtest.h"

#include "mpark/patterns.hpp"

#include <Logger/LogMacros.h>

#include <string>

namespace {

TEST(FooTest, Bar) {
  using namespace std::string_literals;
  auto const p = std::make_pair(42, "world"s);

  using namespace mpark::patterns;
  IDENTIFIERS(str);
  LOG_DEVEL << "Hello, " << match(p)(pattern(ds(42, str)) = [](auto const& str) {
    return str;
  });
}

}  // namespace
