////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2018 ArangoDB GmbH, Cologne, Germany
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
/// @author Tobias Goedderz
/// @author Michael Hackstein
/// @author Heiko Kernbach
/// @author Jan Christoph Uhde
////////////////////////////////////////////////////////////////////////////////

#include "TestExecutorHelper.h"
#include "gtest/gtest.h"

#include "Basics/Common.h"

#include "Aql/AqlValue.h"
#include "Aql/ExecutorInfos.h"
#include "Aql/InputAqlItemRow.h"
#include "Aql/SingleRowFetcher.h"
#include "Logger/LogMacros.h"

#include <utility>

using namespace arangodb;
using namespace arangodb::aql;

TestExecutorHelper::TestExecutorHelper(Fetcher& fetcher, Infos& infos)
    : _infos(infos), _fetcher(fetcher){};
TestExecutorHelper::~TestExecutorHelper() = default;

std::pair<ExecutionState, FilterStats> TestExecutorHelper::produceRows(OutputAqlItemRow& output) {
  TRI_IF_FAILURE("TestExecutorHelper::produceRows") {
    THROW_ARANGO_EXCEPTION(TRI_ERROR_DEBUG);
  }
  ExecutionState state;
  FilterStats stats{};
  InputAqlItemRow input{CreateInvalidInputRowHint{}};

  while (true) {
    std::tie(state, input) = _fetcher.fetchRow();

    if (state == ExecutionState::WAITING) {
      if (_returnedDone) {
        return {ExecutionState::DONE, stats};
      }
      return {state, stats};
    }

    if (!input) {
      TRI_ASSERT(state == ExecutionState::DONE);
      _returnedDone = true;
      return {state, stats};
    }
    TRI_ASSERT(input.isInitialized());

    output.copyRow(input);
    return {state, stats};
    // stats.incrFiltered();
  }
}

TestExecutorHelperInfos::TestExecutorHelperInfos(RegisterId inputRegister_,
                                                 RegisterId nrInputRegisters,
                                                 RegisterId nrOutputRegisters,
                                                 std::unordered_set<RegisterId> registersToClear,
                                                 std::unordered_set<RegisterId> registersToKeep)
    : ExecutorInfos(std::make_shared<std::unordered_set<RegisterId>>(inputRegister_),
                    nullptr, nrInputRegisters, nrOutputRegisters,
                    std::move(registersToClear), std::move(registersToKeep)),
      _inputRegister(inputRegister_) {}

TestExecutorHelperSkipInFetcher::TestExecutorHelperSkipInFetcher(Fetcher& fetcher, Infos& infos)
    : _infos(infos), _fetcher(fetcher){};
TestExecutorHelperSkipInFetcher::~TestExecutorHelperSkipInFetcher() = default;

std::pair<ExecutionState, NoStats> TestExecutorHelperSkipInFetcher::produceRows(OutputAqlItemRow& output) {
  // This Executor is supposed to test SKIP only
  // If producedRows is called something is wrong
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

std::tuple<ExecutionState, NoStats, SharedAqlItemBlockPtr>
TestExecutorHelperSkipInFetcher::fetchBlockForPassthrough(size_t atMost) {
  auto rv = _fetcher.fetchBlockForPassthrough(atMost);
  return {rv.first, {}, std::move(rv.second)};
}

void TestExecutorHelperSkipInFetcher::AssertCallsToFunctions(size_t didSkip, bool didWait) {
  // This is always ok
  EXPECT_TRUE(true);
}

TestExecutorHelperSkipInExecutor::TestExecutorHelperSkipInExecutor(Fetcher& fetcher, Infos& infos)
    : _infos(infos), _fetcher(fetcher), _skipRowsCalls{0} {};
TestExecutorHelperSkipInExecutor::~TestExecutorHelperSkipInExecutor() = default;

std::pair<ExecutionState, NoStats> TestExecutorHelperSkipInExecutor::produceRows(OutputAqlItemRow& output) {
  // This Executor is supposed to test SKIP only
  // If producedRows is called something is wrong
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

std::tuple<ExecutionState, NoStats, size_t> TestExecutorHelperSkipInExecutor::skipRows(size_t toSkip) {
  _skipRowsCalls++;
  // actually forward to fetcher
  auto res = _fetcher.skipRows(toSkip, 0);
  return {res.first, {}, res.second};
}

void TestExecutorHelperSkipInExecutor::AssertCallsToFunctions(size_t didSkip, bool didWait) {
  if (didWait) {
    EXPECT_EQ(2, _skipRowsCalls);
  } else {
    EXPECT_EQ(1, _skipRowsCalls);
  }
  _skipRowsCalls = 0;
}

TestExecutorHelperSkipAsGetSomeExecutor::TestExecutorHelperSkipAsGetSomeExecutor(Fetcher& fetcher,
                                                                                 Infos& infos)
    : _infos(infos), _fetcher(fetcher), _produceRowsCalls{0} {};
TestExecutorHelperSkipAsGetSomeExecutor::~TestExecutorHelperSkipAsGetSomeExecutor() = default;

std::pair<ExecutionState, NoStats> TestExecutorHelperSkipAsGetSomeExecutor::produceRows(OutputAqlItemRow& output) {
  // This Executor is bascially copying the rows one by one (like IdExecutor)
  auto res = _fetcher.fetchRow();
  if (res.second.isInitialized()) {
    output.copyRow(res.second);
  }
  return {res.first, {}};
}

void TestExecutorHelperSkipAsGetSomeExecutor::AssertCallsToFunctions(size_t didSkip,
                                                                     bool didWait) {
  if (didWait) {
    EXPECT_EQ(didSkip + 1, _produceRowsCalls);
  } else {
    EXPECT_EQ(didSkip, _produceRowsCalls);
  }
  _produceRowsCalls = 0;
}