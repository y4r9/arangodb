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

#ifndef ARANGOD_AQL_TEST_EXECUTOR_H
#define ARANGOD_AQL_TEST_EXECUTOR_H

#include "Aql/ExecutionState.h"
#include "Aql/ExecutorInfos.h"
#include "Aql/OutputAqlItemRow.h"
#include "Aql/Stats.h"
#include "Aql/types.h"

#include <memory>

namespace arangodb {
namespace aql {

class InputAqlItemRow;
class ExecutorInfos;
template <BlockPassthrough>
class SingleRowFetcher;

class TestExecutorHelperInfos : public ExecutorInfos {
 public:
  TestExecutorHelperInfos(RegisterId inputRegister, RegisterId nrInputRegisters,
                          RegisterId nrOutputRegisters,
                          std::unordered_set<RegisterId> registersToClear,
                          std::unordered_set<RegisterId> registersToKeep);

  TestExecutorHelperInfos() = delete;
  TestExecutorHelperInfos(TestExecutorHelperInfos&&) = default;
  TestExecutorHelperInfos(TestExecutorHelperInfos const&) = delete;
  ~TestExecutorHelperInfos() = default;

  RegisterId getInputRegister() const noexcept { return _inputRegister; };

 private:
  // This is exactly the value in the parent member ExecutorInfo::_inRegs,
  // respectively getInputRegisters().
  RegisterId _inputRegister;
};

class TestExecutorHelper {
 public:
  struct Properties {
    static const bool preservesOrder = true;
    static const BlockPassthrough allowsBlockPassthrough = BlockPassthrough::Disable;
    static const bool inputSizeRestrictsOutputSize = false;
  };
  using Fetcher = SingleRowFetcher<Properties::allowsBlockPassthrough>;
  using Infos = TestExecutorHelperInfos;
  using Stats = FilterStats;

  TestExecutorHelper() = delete;
  TestExecutorHelper(TestExecutorHelper&&) = default;
  TestExecutorHelper(TestExecutorHelper const&) = delete;
  TestExecutorHelper(Fetcher& fetcher, Infos&);
  ~TestExecutorHelper();

  /**
   * @brief produce the next Row of Aql Values.
   *
   * @return ExecutionState, and if successful exactly one new Row of AqlItems.
   */
  std::pair<ExecutionState, Stats> produceRows(OutputAqlItemRow& output);

 public:
  Infos& _infos;

 private:
  Fetcher& _fetcher;
  bool _returnedDone = false;
};

class TestExecutorHelperSkipInFetcher {
 public:
  struct Properties {
    static const bool preservesOrder = true;
    static const BlockPassthrough allowsBlockPassthrough = BlockPassthrough::Enable;
    static const bool inputSizeRestrictsOutputSize = false;
  };
  using Fetcher = SingleRowFetcher<Properties::allowsBlockPassthrough>;
  using Infos = TestExecutorHelperInfos;
  using Stats = NoStats;

  TestExecutorHelperSkipInFetcher() = delete;
  TestExecutorHelperSkipInFetcher(TestExecutorHelperSkipInFetcher&&) = default;
  TestExecutorHelperSkipInFetcher(TestExecutorHelperSkipInFetcher const&) = delete;
  TestExecutorHelperSkipInFetcher(Fetcher& fetcher, Infos&);
  ~TestExecutorHelperSkipInFetcher();

  /**
   * @brief produce the next Row of Aql Values.
   *
   * @return ExecutionState, and if successful exactly one new Row of AqlItems.
   */
  std::pair<ExecutionState, Stats> produceRows(OutputAqlItemRow& output);

  std::tuple<ExecutionState, Stats, SharedAqlItemBlockPtr> fetchBlockForPassthrough(size_t atMost);

  void AssertCallsToFunctions(size_t didSkip, bool didWait);

 public:
  Infos& _infos;

 private:
  Fetcher& _fetcher;
};

class TestExecutorHelperSkipInExecutor {
 public:
  struct Properties {
    static const bool preservesOrder = true;
    static const BlockPassthrough allowsBlockPassthrough = BlockPassthrough::Disable;
    static const bool inputSizeRestrictsOutputSize = false;
  };
  using Fetcher = SingleRowFetcher<Properties::allowsBlockPassthrough>;
  using Infos = TestExecutorHelperInfos;
  using Stats = NoStats;

  TestExecutorHelperSkipInExecutor() = delete;
  TestExecutorHelperSkipInExecutor(TestExecutorHelperSkipInExecutor&&) = default;
  TestExecutorHelperSkipInExecutor(TestExecutorHelperSkipInExecutor const&) = delete;
  TestExecutorHelperSkipInExecutor(Fetcher& fetcher, Infos&);
  ~TestExecutorHelperSkipInExecutor();

  /**
   * @brief produce the next Row of Aql Values.
   *
   * @return ExecutionState, and if successful exactly one new Row of AqlItems.
   */
  std::pair<ExecutionState, Stats> produceRows(OutputAqlItemRow& output);
  std::tuple<ExecutionState, Stats, size_t> skipRows(size_t toSkip);

  void AssertCallsToFunctions(size_t didSkip, bool didWait);

 public:
  Infos& _infos;

 private:
  Fetcher& _fetcher;

  size_t _skipRowsCalls;
};

class TestExecutorHelperSkipAsGetSomeExecutor {
 public:
  struct Properties {
    static const bool preservesOrder = true;
    static const BlockPassthrough allowsBlockPassthrough = BlockPassthrough::Disable;
    static const bool inputSizeRestrictsOutputSize = false;
  };
  using Fetcher = SingleRowFetcher<Properties::allowsBlockPassthrough>;
  using Infos = TestExecutorHelperInfos;
  using Stats = NoStats;

  TestExecutorHelperSkipAsGetSomeExecutor() = delete;
  TestExecutorHelperSkipAsGetSomeExecutor(TestExecutorHelperSkipAsGetSomeExecutor&&) = default;
  TestExecutorHelperSkipAsGetSomeExecutor(TestExecutorHelperSkipAsGetSomeExecutor const&) = delete;
  TestExecutorHelperSkipAsGetSomeExecutor(Fetcher& fetcher, Infos&);
  ~TestExecutorHelperSkipAsGetSomeExecutor();

  /**
   * @brief produce the next Row of Aql Values.
   *
   * @return ExecutionState, and if successful exactly one new Row of AqlItems.
   */
  std::pair<ExecutionState, Stats> produceRows(OutputAqlItemRow& output);

  void AssertCallsToFunctions(size_t didSkip, bool didWait);

 public:
  Infos& _infos;

 private:
  Fetcher& _fetcher;

  size_t _produceRowsCalls;
};

}  // namespace aql
}  // namespace arangodb

#endif
