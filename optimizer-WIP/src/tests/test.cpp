#include "gtest/gtest.h"

#include <Logger/LogMacros.h>

namespace {

TEST(FooTest, Bar) { LOG_DEVEL << "Hello, world."; }

}
