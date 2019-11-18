////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2019 ArangoDB GmbH, Cologne, Germany
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

#include "SubqueryStartExecutor.h"

#include "Aql/AqlCall.h"
#include "Aql/ExecutionState.h"
#include "Aql/OutputAqlItemRow.h"
#include "Aql/SingleRowFetcher.h"
#include "Aql/Stats.h"

using namespace arangodb;
using namespace arangodb::aql;

constexpr bool SubqueryStartExecutor::Properties::preservesOrder;
constexpr BlockPassthrough SubqueryStartExecutor::Properties::allowsBlockPassthrough;
constexpr bool SubqueryStartExecutor::Properties::inputSizeRestrictsOutputSize;

SubqueryStartExecutor::SubqueryStartExecutor(Fetcher& fetcher, Infos& infos)
    : _fetcher(fetcher),
      _state(ExecutionState::HASMORE),
      _input(CreateInvalidInputRowHint{}) {}
SubqueryStartExecutor::~SubqueryStartExecutor() = default;

std::pair<ExecutionState, NoStats> SubqueryStartExecutor::produceRows(OutputAqlItemRow& output) {
  while (!output.isFull()) {
    TRI_ASSERT(!output.produced());
    if (_state == ExecutionState::DONE && !_input.isInitialized()) {
      // We need to handle shadowRows now. It is the job of this node to
      // increase the shadow row depth
      ShadowAqlItemRow shadowRow{CreateInvalidShadowRowHint{}};
      std::tie(_state, shadowRow) = _fetcher.fetchShadowRow();
      if (!shadowRow.isInitialized()) {
        TRI_ASSERT(_state == ExecutionState::WAITING || _state == ExecutionState::DONE);
        // We are either fully DONE, or WAITING
        return {_state, NoStats{}};
      }
      output.increaseShadowRowDepth(shadowRow);
    } else {
      // This loop alternates between data row and shadow row
      if (_input.isInitialized()) {
        output.createShadowRow(_input);
        _input = InputAqlItemRow(CreateInvalidInputRowHint{});
      } else {
        std::tie(_state, _input) = _fetcher.fetchRow(output.numRowsLeft() / 2);
        if (!_input.isInitialized()) {
          TRI_ASSERT(_state == ExecutionState::WAITING || _state == ExecutionState::DONE);
          return {_state, NoStats{}};
        }
        TRI_ASSERT(!output.isFull());
        output.copyRow(_input);
      }
    }
    output.advanceRow();
  }
  if (_input.isInitialized()) {
    // We at least need to insert the Shadow row!
    return {ExecutionState::HASMORE, NoStats{}};
  }
  // Take state from dependency.
  return {_state, NoStats{}};
}

std::pair<ExecutionState, size_t> SubqueryStartExecutor::expectedNumberOfRows(size_t atMost) const {
  ExecutionState state{ExecutionState::HASMORE};
  size_t expected = 0;
  std::tie(state, expected) = _fetcher.preFetchNumberOfRows(atMost);
  // We will write one shadow row per input data row.
  // We might write less on all shadow rows in input, right now we do not figure this out yes.
  return {state, expected * 2};
}

[[nodiscard]] std::tuple<ExecutorState, NoStats, AqlCall> SubqueryStartExecutor::produceRows(
    size_t limit, AqlItemBlockInputRange& input, OutputAqlItemRow& output) {
  auto upstreamCall = AqlCall{};
  auto stats = NoStats{};
  auto nrOutput = size_t{0};

  // this is a slight hack, because in this loop we always need
  // two slots in output, one for the ShadowRow, and one for the
  // actual row (and if we don't we'd have to store the input row
  // across calls to produce row, because we could run out of space
  // in output).
  while ((nrOutput < limit - 1) && (input.hasMore() || input.hasShadowRow())) {
    if (input.hasMore()) {
      TRI_ASSERT(!output.produced());

      auto const& [state, row] = input.next();

      output.copyRow(row);
      output.advanceRow();
      nrOutput++;

      output.createShadowRow(row);
      output.advanceRow();
      nrOutput++;
      TRI_ASSERT(!output.isFull());

    } else if (input.hasShadowRow()) {
      auto const& [state, row] = input.nextShadowRow();
      output.increaseShadowRowDepth(row);
      output.advanceRow();
      nrOutput++;
    }
  }

  if (input.hasMore() || input.hasShadowRow()) {
    return {ExecutorState::HASMORE, stats, upstreamCall};
  } else {
    return {ExecutorState::DONE, stats, upstreamCall};
  }
}
