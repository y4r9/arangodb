////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2014-2020 ArangoDB GmbH, Cologne, Germany
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

#include "gtest/gtest.h"

#include "AqlExecutorTestCase.h"
#include "RowFetcherHelper.h"

#include "Aql/AqlItemBlock.h"
#include "Aql/AqlItemBlockHelper.h"
#include "Aql/InputAqlItemRow.h"
#include "Aql/OutputAqlItemRow.h"
#include "Aql/RegisterInfos.h"
#include "Aql/Stats.h"
#include "Aql/SubqueryStartExecutor.h"

#include <velocypack/Builder.h>
#include <velocypack/Parser.h>
#include <velocypack/velocypack-aliases.h>

#include "Logger/LogMacros.h"
using namespace arangodb;
using namespace arangodb::aql;
using namespace arangodb::tests;
using namespace arangodb::tests::aql;

namespace {
RegisterInfos MakeBaseInfos(RegisterId numRegs) {
  RegIdSet prototype{};
  for (RegisterId r = 0; r < numRegs; ++r) {
    prototype.emplace(r);
  }
  return RegisterInfos({}, {}, numRegs, numRegs, {},
                       {{prototype}, {prototype}, {prototype}});
}
}  // namespace

// We need to be backwards compatible, with version 3.6
// There we do not get a fullStack, but only a single entry.
// We have a compatibility mode Stack for this version
// These tests can be removed again in the branch for the version
// after 3.7.*
enum CompatibilityMode { VERSION36, VERSION37 };

using SubqueryStartSplitType = ExecutorTestHelper<1, 1>::SplitType;

class SubqueryStartExecutorTest
    : public AqlExecutorTestCaseWithParam<std::tuple<CompatibilityMode, SubqueryStartSplitType, bool>, false> {
 protected:
  auto GetCompatMode() const -> CompatibilityMode {
    auto const [mode, split, upsert] = GetParam();
    return mode;
  }

  auto GetSplit() const -> SubqueryStartSplitType {
    auto const [mode, split, upsert] = GetParam();
    return split;
  }

  auto GetUpsertStyle() const -> bool {
    auto const [mode, split, upsert] = GetParam();
    return upsert;
  }

  auto queryStack(AqlCall fromSubqueryEnd, AqlCall insideSubquery) const -> AqlCallStack {
    AqlCallList list = insideSubquery.getOffset() == 0 && !insideSubquery.needsFullCount()
                           ? AqlCallList{insideSubquery, insideSubquery}
                           : AqlCallList{insideSubquery};
    if (GetCompatMode() == CompatibilityMode::VERSION36) {
      return AqlCallStack{list, true};
    }
    AqlCallStack stack(AqlCallList{fromSubqueryEnd});
    stack.pushCall(list);
    return stack;
  }

  template <std::size_t inputColumns, std::size_t outputColumns>
  auto addConsumer(ExecutorTestHelper<inputColumns, outputColumns>& helper) -> ExecutorTestHelper<inputColumns, outputColumns>& {
    if (GetUpsertStyle()) {
        return helper.template addConsumer<SubqueryStartExecutor<true>>(MakeBaseInfos(1), MakeBaseInfos(1), ExecutionNode::SUBQUERY_START);
    }
    return helper.template addConsumer<SubqueryStartExecutor<false>>(MakeBaseInfos(1), MakeBaseInfos(1), ExecutionNode::SUBQUERY_START);
  }
};



template <size_t... vs>
const SubqueryStartSplitType splitIntoBlocks =
    SubqueryStartSplitType{std::vector<std::size_t>{vs...}};
template <size_t step>
const SubqueryStartSplitType splitStep = SubqueryStartSplitType{step};

INSTANTIATE_TEST_CASE_P(
    SubqueryStartExecutorTest, SubqueryStartExecutorTest,
    ::testing::Combine(::testing::Values(CompatibilityMode::VERSION36, CompatibilityMode::VERSION37),
                       ::testing::Values(splitIntoBlocks<2, 3>,
                                         splitIntoBlocks<3, 4>, splitStep<2>),
                                         ::testing::Bool()));

TEST_P(SubqueryStartExecutorTest, check_properties) {
  EXPECT_TRUE(SubqueryStartExecutor<false>::Properties::preservesOrder)
      << "The block has no effect on ordering of elements, it adds additional "
         "rows only.";
  EXPECT_EQ(SubqueryStartExecutor<false>::Properties::allowsBlockPassthrough, ::arangodb::aql::BlockPassthrough::Disable)
      << "The block cannot be passThrough, as it increases the number of rows.";
  EXPECT_TRUE(SubqueryStartExecutor<false>::Properties::inputSizeRestrictsOutputSize)
      << "The block is restricted by input, it will atMost produce 2 times the "
         "input. (Might be less if input contains shadowRows";

  EXPECT_TRUE(SubqueryStartExecutor<true>::Properties::preservesOrder)
      << "The block has no effect on ordering of elements, it adds additional "
         "rows only.";
  EXPECT_EQ(SubqueryStartExecutor<true>::Properties::allowsBlockPassthrough,
            ::arangodb::aql::BlockPassthrough::Disable)
      << "The block cannot be passThrough, as it increases the number of rows.";
  EXPECT_TRUE(SubqueryStartExecutor<true>::Properties::inputSizeRestrictsOutputSize)
      << "The block is restricted by input, it will atMost produce 2 times the "
         "input. (Might be less if input contains shadowRows";
}

TEST_P(SubqueryStartExecutorTest, empty_input_does_not_add_shadow_rows) {
  auto helper = makeExecutorTestHelper<1, 1>();
  addConsumer(helper)
      .setInputValue({})
      .expectedStats(ExecutionStats{})
      .expectedState(ExecutionState::DONE)
      .expectOutput({0}, {})
      .expectSkipped(0, 0)
      .setCallStack(queryStack(AqlCall{}, AqlCall{}))
      .setInputSplitType(GetSplit())
      .run();
}

TEST_P(SubqueryStartExecutorTest, adds_a_shadowrow_after_single_input) {
 auto helper =  makeExecutorTestHelper<1, 1>();
      addConsumer(helper)
      .setInputValue({{R"("a")"}})
      .expectedStats(ExecutionStats{})
      .expectedState(ExecutionState::DONE)
      .expectSkipped(0, 0)
      .expectOutput({0}, {{R"("a")"}, {R"("a")"}}, {{1, 0}})
      .setCallStack(queryStack(AqlCall{}, AqlCall{}))
      .setInputSplitType(GetSplit())
      .run();
}

TEST_P(SubqueryStartExecutorTest, adds_a_shadowrow_after_every_input_line_in_single_pass) {
 auto helper =  makeExecutorTestHelper<1, 1>();
      addConsumer(helper)
      .setInputValue({{{R"("a")"}}, {{R"("b")"}}, {{R"("c")"}}})
      .expectedStats(ExecutionStats{})
      .expectedState(ExecutionState::DONE)
      .expectSkipped(0, 0)
      .expectOutput({0}, {{R"("a")"}, {R"("a")"}, {R"("b")"}, {R"("b")"}, {R"("c")"}, {R"("c")"}},
                    {{1, 0}, {3, 0}, {5, 0}})
      .setCallStack(queryStack(AqlCall{}, AqlCall{}))
      .setInputSplitType(GetSplit())
      .run();
}

// NOTE: As soon as the single_pass test is enabled this test is superflous.
// It will be identical to the one above
TEST_P(SubqueryStartExecutorTest, adds_a_shadowrow_after_every_input_line) {
 auto helper =  makeExecutorTestHelper<1, 1>();
      addConsumer(helper)
      .setInputValue({{{R"("a")"}}, {{R"("b")"}}, {{R"("c")"}}})
      .expectedStats(ExecutionStats{})
      .expectedState(ExecutionState::DONE)
      .expectSkipped(0, 0)
      .expectOutput({0}, {{R"("a")"}, {R"("a")"}, {R"("b")"}, {R"("b")"}, {R"("c")"}, {R"("c")"}},
                    {{1, 0}, {3, 0}, {5, 0}})
      .setCallStack(queryStack(AqlCall{}, AqlCall{}))
      .setInputSplitType(GetSplit())
      .run(true);
}

TEST_P(SubqueryStartExecutorTest, shadow_row_does_not_fit_in_current_block) {
  // NOTE: This test relies on batchSizes being handled correctly and we do not over-allocate memory
  // Also it tests, that ShadowRows go into place accounting of the output block (count as 1 line)

  // NOTE: Reduce batch size to 1, to enforce a too small output block
  ExecutionBlock::setDefaultBatchSize(1);
  TRI_DEFER(ExecutionBlock::setDefaultBatchSize(ExecutionBlock::ProductionDefaultBatchSize););
  {
    // First test: Validate that the shadowRow is not written
    // We only do a single call here
   auto helper =  makeExecutorTestHelper<1, 1>();
        addConsumer(helper)
        .setInputValue({{R"("a")"}})
        .expectedStats(ExecutionStats{})
        .expectedState(ExecutionState::HASMORE)
        .expectSkipped(0, 0)
        .expectOutput({0}, {{R"("a")"}}, {})
        .setCallStack(queryStack(AqlCall{}, AqlCall{}))
        .setInputSplitType(GetSplit())
        .run();
  }
  {
    // Second test: Validate that the shadowRow is eventually written
    // if we call often enough
   auto helper =  makeExecutorTestHelper<1, 1>();
        addConsumer(helper)
        .setInputValue({{R"("a")"}})
        .expectedStats(ExecutionStats{})
        .expectedState(ExecutionState::DONE)
        .expectSkipped(0, 0)
        .expectOutput({0}, {{R"("a")"}, {R"("a")"}}, {{1, 0}})
        .setCallStack(queryStack(AqlCall{}, AqlCall{}))
        .setInputSplitType(GetSplit())
        .run(true);
  }
}

TEST_P(SubqueryStartExecutorTest, skip_in_subquery) {
 auto helper =  makeExecutorTestHelper<1, 1>();
      addConsumer(helper)
      .setInputValue({{R"("a")"}})
      .expectedStats(ExecutionStats{})
      .expectedState(ExecutionState::DONE)
      .expectOutput({0}, {{R"("a")"}}, {{0, 0}})
      .expectSkipped(0, 1)
      .setCallStack(queryStack(AqlCall{}, AqlCall{10, false}))
      .setInputSplitType(GetSplit())
      .run();
}

TEST_P(SubqueryStartExecutorTest, fullCount_in_subquery) {
 auto helper =  makeExecutorTestHelper<1, 1>();
      addConsumer(helper)
      .setInputValue({{R"("a")"}})
      .expectedStats(ExecutionStats{})
      .expectedState(ExecutionState::DONE)
      .expectOutput({0}, {{R"("a")"}}, {{0, 0}})
      .expectSkipped(0, 1)
      .setCallStack(queryStack(AqlCall{}, AqlCall{0, true, 0, AqlCall::LimitType::HARD}))
      .setInputSplitType(GetSplit())
      .run();
}

TEST_P(SubqueryStartExecutorTest, shadow_row_forwarding) {
  if (GetCompatMode() == CompatibilityMode::VERSION36) {
    // This situation cannot occur
    return;
  }
  auto helper = makeExecutorTestHelper<1, 1>();

  AqlCallStack stack = queryStack(AqlCall{}, AqlCall{});
  stack.pushCall(AqlCallList{AqlCall{}});
      addConsumer(helper);
      addConsumer(helper);

  helper.expectSkipped(0, 0, 0);

  helper.setInputValue({{R"("a")"}})
      .expectedStats(ExecutionStats{})
      .expectedState(ExecutionState::DONE)
      .expectOutput({0}, {{R"("a")"}, {R"("a")"}, {R"("a")"}}, {{1, 0}, {2, 1}})
      .setCallStack(stack)
      .setInputSplitType(GetSplit())
      .run();
}

TEST_P(SubqueryStartExecutorTest, shadow_row_forwarding_many_inputs_single_call) {
  if (GetCompatMode() == CompatibilityMode::VERSION36) {
    // This situation cannot occur
    return;
  }
  auto helper = makeExecutorTestHelper<1, 1>();
  AqlCallStack stack = queryStack(AqlCall{}, AqlCall{});
  stack.pushCall(AqlCallList{AqlCall{}});

  addConsumer(helper);
  addConsumer(helper);

  helper.expectSkipped(0, 0, 0);

  helper.setInputValue({{R"("a")"}, {R"("b")"}, {R"("c")"}})
      .expectedStats(ExecutionStats{})
      .expectedState(ExecutionState::HASMORE)
      .expectOutput({0}, {{R"("a")"}, {R"("a")"}, {R"("a")"}}, {{1, 0}, {2, 1}})
      .setCallStack(stack)
      .setInputSplitType(GetSplit())
      .run();
}

TEST_P(SubqueryStartExecutorTest, shadow_row_forwarding_many_inputs_many_requests) {
  if (GetCompatMode() == CompatibilityMode::VERSION36) {
    // This situation cannot occur
    return;
  }
  auto helper = makeExecutorTestHelper<1, 1>();
  AqlCallStack stack = queryStack(AqlCall{}, AqlCall{});
  stack.pushCall(AqlCallList{AqlCall{}});

  addConsumer(helper);
  addConsumer(helper);

  helper.expectSkipped(0, 0, 0);

  helper.setInputValue({{R"("a")"}, {R"("b")"}, {R"("c")"}})
      .expectedStats(ExecutionStats{})
      .expectedState(ExecutionState::DONE)
      .expectOutput(
          {0},
          {{R"("a")"}, {R"("a")"}, {R"("a")"}, {R"("b")"}, {R"("b")"}, {R"("b")"}, {R"("c")"}, {R"("c")"}, {R"("c")"}},
          {{1, 0}, {2, 1}, {4, 0}, {5, 1}, {7, 0}, {8, 1}})
      .setCallStack(stack)
      .setInputSplitType(GetSplit())
      .run(true);
}

TEST_P(SubqueryStartExecutorTest, shadow_row_forwarding_many_inputs_not_enough_space) {
  if (GetCompatMode() == CompatibilityMode::VERSION36) {
    // This situation cannot occur
    return;
  }
  // NOTE: This test relies on batchSizes being handled correctly and we do not over-allocate memory
  // Also it tests, that ShadowRows go into place accounting of the output block (count as 1 line)

  // NOTE: Reduce batch size to 2, to enforce a too small output block, in between the shadow Rows
  ExecutionBlock::setDefaultBatchSize(2);
  TRI_DEFER(ExecutionBlock::setDefaultBatchSize(ExecutionBlock::ProductionDefaultBatchSize););
  {
    // First test: Validate that the shadowRow is not written
    // We only do a single call here
    auto helper = makeExecutorTestHelper<1, 1>();

    AqlCallStack stack = queryStack(AqlCall{}, AqlCall{});
    stack.pushCall(AqlCallList{AqlCall{}});

    addConsumer(helper);
    addConsumer(helper);

    helper.expectSkipped(0, 0, 0);

    helper.setInputValue({{R"("a")"}, {R"("b")"}, {R"("c")"}})
        .expectedStats(ExecutionStats{})
        .expectedState(ExecutionState::HASMORE)
        .expectOutput({0}, {{R"("a")"}, {R"("a")"}}, {{1, 0}})
        .setCallStack(stack)
        .setInputSplitType(GetSplit())
        .run();
  }
  {
    // Second test: Validate that the shadowRow is eventually written
    // Wedo call as many times as we need to.
    auto helper = makeExecutorTestHelper<1, 1>();

    AqlCallStack stack = queryStack(AqlCall{}, AqlCall{});
    stack.pushCall(AqlCallList{AqlCall{}});

    addConsumer(helper);
    addConsumer(helper);

    helper.expectSkipped(0, 0, 0);

    helper.setInputValue({{R"("a")"}, {R"("b")"}, {R"("c")"}})
        .expectedStats(ExecutionStats{})
        .expectedState(ExecutionState::DONE)
        .expectOutput(
            {0},
            {{R"("a")"}, {R"("a")"}, {R"("a")"}, {R"("b")"}, {R"("b")"}, {R"("b")"}, {R"("c")"}, {R"("c")"}, {R"("c")"}},
            {{1, 0}, {2, 1}, {4, 0}, {5, 1}, {7, 0}, {8, 1}})
        .setCallStack(stack)
        .setInputSplitType(GetSplit())
        .run(true);
  }
}

TEST_P(SubqueryStartExecutorTest, skip_in_outer_subquery) {
  if (GetCompatMode() == CompatibilityMode::VERSION37 && GetUpsertStyle() == false) {
   auto helper =  makeExecutorTestHelper<1, 1>();
        addConsumer(helper)
        .setInputValue({{R"("a")"}, {R"("b")"}})
        .expectedStats(ExecutionStats{})
        .expectedState(ExecutionState::DONE)
        .expectOutput({0}, {{R"("b")"}, {R"("b")"}}, {{1, 0}})
        .expectSkipped(1, 0)
        .setCallStack(queryStack(AqlCall{1, false, AqlCall::Infinity{}}, AqlCall{}))
        .setInputSplitType(GetSplit())
        .run();
  } else {
    // The feature is not available in 3.6 or earlier.
  }
}

TEST_P(SubqueryStartExecutorTest, DISABLED_skip_only_in_outer_subquery) {
  if (GetCompatMode() == CompatibilityMode::VERSION37 && GetUpsertStyle() == false) {
    auto helper = makeExecutorTestHelper<1, 1>();
    addConsumer(helper)
        .setInputValue({{R"("a")"}, {R"("b")"}})
        .expectedStats(ExecutionStats{})
        .expectedState(ExecutionState::DONE)
        .expectOutput({0}, {})
        .expectSkipped(1, 0)
        .setCallStack(queryStack(AqlCall{1, false}, AqlCall{}))
        .setInputSplitType(GetSplit())
        .run();
  } else {
    // The feature is not available in 3.7 or earlier.
  }
}

TEST_P(SubqueryStartExecutorTest, fullCount_in_outer_subquery) {
  if (GetCompatMode() == CompatibilityMode::VERSION37 && GetUpsertStyle() == false) {
   auto helper = makeExecutorTestHelper<1, 1>();
   addConsumer(helper)
       .setInputValue({{R"("a")"}, {R"("b")"}, {R"("c")"}, {R"("d")"}, {R"("e")"}, {R"("f")"}})
       .expectedStats(ExecutionStats{})
       .expectedState(ExecutionState::DONE)
       .expectOutput({0}, {})
       .expectSkipped(6, 0)
       .setCallStack(queryStack(AqlCall{0, true, 0, AqlCall::LimitType::HARD}, AqlCall{}))
       .setInputSplitType(GetSplit())
       .run();
  } else {
    // The feature is not available in 3.7 or earlier.
  }
}

TEST_P(SubqueryStartExecutorTest, fastForward_in_inner_subquery) {
  if (GetCompatMode() == CompatibilityMode::VERSION37) {
    auto helper = makeExecutorTestHelper<1, 1>();
        addConsumer(helper)
        .setInputValue({{R"("a")"}, {R"("b")"}, {R"("c")"}, {R"("d")"}, {R"("e")"}, {R"("f")"}})
        .expectedStats(ExecutionStats{})
        .expectedState(ExecutionState::DONE)
        .expectOutput({0}, {{R"("a")"}, {R"("b")"}, {R"("c")"}, {R"("d")"}, {R"("e")"}, {R"("f")"}},
                      {{0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}})
        .expectSkipped(0, 0)
        .setCallStack(queryStack(AqlCall{0, false, AqlCall::Infinity{}},
                                 AqlCall{0, false, 0, AqlCall::LimitType::HARD}))
        .setInputSplitType(GetSplit())
        .run();
  } else {
    // The feature is not available in 3.7 or earlier.
  }
}

TEST_P(SubqueryStartExecutorTest, skip_out_skip_in) {
  if (GetCompatMode() == CompatibilityMode::VERSION37 && GetUpsertStyle() == false) {
   auto helper = makeExecutorTestHelper<1, 1>();
        addConsumer(helper)
        .setInputValue({{R"("a")"}, {R"("b")"}, {R"("c")"}, {R"("d")"}, {R"("e")"}, {R"("f")"}})
        .expectedStats(ExecutionStats{})
        .expectedState(ExecutionState::HASMORE)
        .expectOutput({0}, {{R"("c")"}}, {{0, 0}})
        .expectSkipped(2, 1)
        .setCallStack(queryStack(AqlCall{2, false, AqlCall::Infinity{}},
                                 AqlCall{10, false, AqlCall::Infinity{}}))
        .setInputSplitType(GetSplit())
        .run();
  } else {
    // The feature is not available in 3.7 or earlier.
  }
}

TEST_P(SubqueryStartExecutorTest, fullbypass_in_outer_subquery) {
  if (GetCompatMode() == CompatibilityMode::VERSION37 && GetUpsertStyle() == false) {
    auto helper = makeExecutorTestHelper<1, 1>();
    addConsumer(helper)
        .setInputValue({{R"("a")"}, {R"("b")"}, {R"("c")"}, {R"("d")"}, {R"("e")"}, {R"("f")"}})
        .expectedStats(ExecutionStats{})
        .expectedState(ExecutionState::DONE)
        .expectOutput({0}, {})
        .expectSkipped(0, 0)
        .setCallStack(queryStack(AqlCall{0, false, 0, AqlCall::LimitType::HARD}, AqlCall{}))
        .setInputSplitType(GetSplit())
        .run();
  } else {
    // The feature is not available in 3.7 or earlier.
  }
}
