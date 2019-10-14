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
/// @author Michael Hackstein
////////////////////////////////////////////////////////////////////////////////

#include "WaitingExecutionBlockMock.h"

#include "Aql/AqlItemBlock.h"
#include "Aql/ExecutionEngine.h"
#include "Aql/ExecutionState.h"
#include "Aql/ExecutionStats.h"
#include "Aql/ExecutorInfos.h"
#include "Aql/QueryOptions.h"
#include "Aql/ShadowAqlItemRow.h"

#include <velocypack/velocypack-aliases.h>

using namespace arangodb;
using namespace arangodb::aql;
using namespace arangodb::tests;
using namespace arangodb::tests::aql;

WaitingExecutionBlockMock::WaitingExecutionBlockMock(ExecutionEngine* engine,
                                                     ExecutionNode const* node,
                                                     std::deque<SharedAqlItemBlockPtr>&& data)
    : ExecutionBlock(engine, node),
      _data(std::move(data)),
      _resourceMonitor(),
      _inflight(0),
      _hasWaited(false),
      _skippedInBlock(0) {}

std::pair<arangodb::aql::ExecutionState, arangodb::Result> WaitingExecutionBlockMock::initializeCursor(
    arangodb::aql::InputAqlItemRow const& input) {
  if (!_hasWaited) {
    _hasWaited = true;
    return {ExecutionState::WAITING, TRI_ERROR_NO_ERROR};
  }
  _hasWaited = false;
  _inflight = 0;
  return {ExecutionState::DONE, TRI_ERROR_NO_ERROR};
}

std::pair<arangodb::aql::ExecutionState, Result> WaitingExecutionBlockMock::shutdown(int errorCode) {
  ExecutionState state;
  Result res;
  return std::make_pair(state, res);
}

std::pair<arangodb::aql::ExecutionState, SharedAqlItemBlockPtr> WaitingExecutionBlockMock::getSome(size_t atMost) {
  if (!_hasWaited && _skippedInBlock == 0) {
    _hasWaited = true;
    if (_returnedDone) {
      return {ExecutionState::DONE, nullptr};
    }
    return {ExecutionState::WAITING, nullptr};
  }
  _hasWaited = false;

  if (_data.empty()) {
    _returnedDone = true;
    return {ExecutionState::DONE, nullptr};
  }
  SharedAqlItemBlockPtr result = _data.front();
  _data.pop_front();
  if (_skippedInBlock != 0) {
    // cut-off the first rows.
    result = result->slice(_skippedInBlock, result->size());
    _skippedInBlock = 0;
  }

  if (_data.empty()) {
    _returnedDone = true;
    return {ExecutionState::DONE, std::move(result)};
  } else {
    return {ExecutionState::HASMORE, std::move(result)};
  }
}

std::pair<arangodb::aql::ExecutionState, size_t> WaitingExecutionBlockMock::skipSome(
    size_t atMost, size_t subqueryDepth) {
  traceSkipSomeBegin(atMost);
  if (!_hasWaited && _skippedInBlock == 0) {
    _hasWaited = true;
    traceSkipSomeEnd(ExecutionState::WAITING, 0);
    return {ExecutionState::WAITING, 0};
  }
  _hasWaited = false;

  if (_data.empty()) {
    traceSkipSomeEnd(ExecutionState::DONE, 0);
    return {ExecutionState::DONE, 0};
  }
  auto block = _data.front();
  size_t available = block->size();
  TRI_ASSERT(available >= _skippedInBlock);
  size_t skipped = 0;
  size_t maxSkip = (std::min)(available - _skippedInBlock, atMost);
  size_t skipTo = _skippedInBlock + maxSkip;
  if (subqueryDepth == 0) {
    skipped = maxSkip;
  }
  if (block->hasShadowRows()) {
    auto shadows = block->getShadowRowIndexes();
    for (auto const shadowRowIdx : shadows) {
      if (shadowRowIdx >= _skippedInBlock) {
        // Guarantee that shadowRows are sorted.
        // We can stop on the first that matches this condition
        if (shadowRowIdx < _skippedInBlock + maxSkip) {
          // If the shadowRow is before the skipTo target
          // We need to only jump to this row, not more.
          skipTo = shadowRowIdx;
          if (subqueryDepth == 0) {
            skipped = skipTo - _skippedInBlock;
          }
        }
        ShadowAqlItemRow row{block, skipTo};
        if (row.getDepth() >= subqueryDepth) {
          // We skipped to the end of the subquery
          break;
        } else {
          if (row.getDepth() + 1 == subqueryDepth) {
            TRI_ASSERT(subqueryDepth > 0);
            // Only count rows having level + 1
            skipped++;
          }
        }
      }
    }
  }
  TRI_ASSERT(skipTo <= available);

  if (skipTo == available) {
    // Drop the block, it is done
    _data.pop_front();
    _skippedInBlock = 0;
  } else {
    _skippedInBlock = skipTo;
  }

  if (_data.empty() || block->isShadowRow(_skippedInBlock)) {
    traceSkipSomeEnd(ExecutionState::DONE, skipped);
    return {ExecutionState::DONE, skipped};
  } else {
    traceSkipSomeEnd(ExecutionState::HASMORE, skipped);
    return {ExecutionState::HASMORE, skipped};
  }
}

/// @brief fetchShadowRow, get's the next shadowRow on the fetcher, and causes
///        the subquery to reset.
///        Returns State == DONE if we are at the end of the query and
///        State == HASMORE if there is another subquery ongoing.
///        ShadowAqlItemRow might be empty on any call, if it is
///        the execution is either DONE or at the first input on the next subquery.
std::pair<ExecutionState, ShadowAqlItemRow> WaitingExecutionBlockMock::fetchShadowRow() {
  // NOTE: This only works with SkipSome thus far!
  if (_data.empty()) {
    return {ExecutionState::DONE, ShadowAqlItemRow{CreateInvalidShadowRowHint{}}};
  }
  auto block = _data.front();
  if (!block->isShadowRow(_skippedInBlock)) {
    return {ExecutionState::HASMORE, ShadowAqlItemRow{CreateInvalidShadowRowHint{}}};
  }
  ShadowAqlItemRow row{block, _skippedInBlock};
  _skippedInBlock++;
  if (_skippedInBlock == block->size()) {
    // Drop the block, it is fully consumed now
    _data.pop_front();
    _skippedInBlock = 0;
  }
  if (_data.empty()) {
    return {ExecutionState::DONE, row};
  }
  return {ExecutionState::HASMORE, row};
}