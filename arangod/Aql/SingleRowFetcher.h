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
/// @author Tobias Gödderz
/// @author Michael Hackstein
/// @author Heiko Kernbach
/// @author Jan Christoph Uhde
////////////////////////////////////////////////////////////////////////////////

#ifndef ARANGOD_AQL_SINGLE_ROW_FETCHER_H
#define ARANGOD_AQL_SINGLE_ROW_FETCHER_H

#include "Aql/ExecutionBlock.h"
#include "Aql/ExecutionState.h"
#include "Aql/InputAqlItemRow.h"

#include <memory>

namespace arangodb {
namespace aql {

class AqlItemBlock;
template <bool>
class DependencyProxy;

/**
 * @brief Interface for all AqlExecutors that do only need one
 *        row at a time in order to make progress.
 *        The guarantee is the following:
 *        If fetchRow returns a row the pointer to
 *        this row stays valid until the next call
 *        of fetchRow.
 */
template <bool passBlocksThrough>
class SingleRowFetcher {
 public:
  explicit SingleRowFetcher(DependencyProxy<passBlocksThrough>& executionBlock);
  TEST_VIRTUAL ~SingleRowFetcher() = default;

 protected:
  // only for testing! Does not initialize _dependencyProxy!
  SingleRowFetcher();

 public:
  /**
   * @brief Fetch one new AqlItemRow from upstream.
   *        **Guarantee**: the row returned is valid only
   *        until the next call to fetchRow.
   *
   * @param atMost may be passed if a block knows the maximum it might want to
   *        fetch from upstream (should apply only to the LimitExecutor). Will
   *        not fetch more than the default batch size, so passing something
   *        greater than it will not have any effect.
   *
   * @return A pair with the following properties:
   *         ExecutionState:
   *           WAITING => IO going on, immediatly return to caller.
   *           DONE => No more to expect from Upstream, if you are done with
   *                   this row return DONE to caller.
   *           HASMORE => There is potentially more from above, call again if
   *                      you need more input.
   *         AqlItemRow:
   *           If WAITING => Do not use this Row, it is a nullptr.
   *           If HASMORE => The Row is guaranteed to not be a nullptr.
   *           If DONE => Row can be a nullptr (nothing received) or valid.
   */
  // This is only TEST_VIRTUAL, so we ignore this lint warning:
  // NOLINTNEXTLINE google-default-arguments
  TEST_VIRTUAL std::pair<ExecutionState, InputAqlItemRow> fetchRow(
      size_t atMost = ExecutionBlock::DefaultBatchSize());

  ExecutionState nextRow(size_t atMost = ExecutionBlock::DefaultBatchSize());

  // TODO enable_if<passBlocksThrough>
  std::pair<ExecutionState, SharedAqlItemBlockPtr> fetchBlockForPassthrough(size_t atMost);

  std::pair<ExecutionState, size_t> preFetchNumberOfRows(size_t atMost) {
    if (_upstreamState != ExecutionState::DONE && (!isInitialized() || isLastRowInBlock())) {
      TRI_ASSERT(_nextBlock == nullptr);
      ExecutionState state;
      std::tie(state, _nextBlock) = fetchBlock(atMost);
      // we de not need result as local members are modified
      if (state == ExecutionState::WAITING) {
        return {state, 0};
      }
      // The internal state should be in-line with the returned state.
      TRI_ASSERT(_upstreamState == state);
      TRI_ASSERT(_nextBlock != nullptr || _upstreamState == ExecutionState::DONE);
    }

    // If upstream is done, we can calculate the number of rows that are left.
    if (_upstreamState == ExecutionState::DONE) {
      size_t const rowsInCurrentBlock = isInitialized() ? currentRow().numRowsAfterThis() : 0;
      size_t const rowsInNextBlock = _nextBlock != nullptr ? _nextBlock->size() : 0;

      // While the calculation here would still be correct, we should never have
      // fetched a new block unless we don't have more in the current one.
      TRI_ASSERT(rowsInCurrentBlock == 0 || rowsInNextBlock == 0);

      // we only have the block in hand, so we can only return that
      // many additional rows
      return {_upstreamState, (std::min)(rowsInCurrentBlock + rowsInNextBlock, atMost)};
    }

    TRI_ASSERT(_upstreamState == ExecutionState::HASMORE);
    TRI_ASSERT(_nextBlock != nullptr);
    // Here we can only assume that we have enough from upstream
    // We do not want to pull additional block
    return {_upstreamState, atMost};
  }

  InputAqlItemRow const& currentRow() const noexcept { return _currentRow; }

 private:
  /**
   * @brief Delegates to ExecutionBlock::fetchBlock()
   */
  std::pair<ExecutionState, SharedAqlItemBlockPtr> fetchBlock(size_t atMost);

  /**
   * @brief Delegates to ExecutionBlock::getNrInputRegisters()
   */
  inline RegisterId getNrInputRegisters() const {
    return _dependencyProxy->getNrInputRegisters();
  }

  inline bool isLastRowInBlock() const noexcept {
    return _currentRow.isLastRowInBlock();
  }

  inline bool isInitialized() const noexcept {
    return _currentRow.isInitialized();
  }

 private:
  DependencyProxy<passBlocksThrough>* _dependencyProxy;

  /**
   * @brief Holds state returned by the last fetchBlock() call.
   *        This is similar to ExecutionBlock::_upstreamState, but can also be
   *        WAITING.
   *        Part of the Fetcher, and may be moved if the Fetcher implementations
   *        are moved into separate classes.
   */
  ExecutionState _upstreamState;

  /**
   * @brief If not nullptr, this is the next block to use, set by preFetchNumberOfRows().
   */
  SharedAqlItemBlockPtr _nextBlock;

  /**
   * @brief The current row, as returned last by fetchRow(). Must stay valid
   *        until the next nextRow() call.
   */
  InputAqlItemRow _currentRow;
};

template <bool passBlocksThrough>
// NOLINTNEXTLINE google-default-arguments
std::pair<ExecutionState, InputAqlItemRow> SingleRowFetcher<passBlocksThrough>::fetchRow(size_t atMost) {
  arangodb::aql::ExecutionState state = nextRow(atMost);
  return {state, _currentRow};
}

template <bool passBlocksThrough>
ExecutionState SingleRowFetcher<passBlocksThrough>::nextRow(size_t atMost) {
  // Fetch a new block iff necessary
  if (ADB_UNLIKELY(!isInitialized() || isLastRowInBlock())) {
    if (_nextBlock != nullptr) {
      // Block was prefetched
      _currentRow.reset(std::move(_nextBlock));
      TRI_ASSERT(_nextBlock == nullptr);
    } else {
      // This returns the AqlItemBlock to the ItemBlockManager before fetching a
      // new one, so we might reuse it immediately!
      // Also sets the index to 0.
      _currentRow.reset(nullptr);

      std::tie(std::ignore, _currentRow.blockPtrRef()) = fetchBlock(atMost);
    }

    if (!isInitialized()) {
      TRI_ASSERT(_upstreamState == ExecutionState::DONE);
      return ExecutionState::DONE;
    } else if (isLastRowInBlock()) {
      return _upstreamState;
    } else {
      return ExecutionState::HASMORE;
    }
  }

  TRI_ASSERT(isInitialized() && !isLastRowInBlock());
  bool const rowStillValid = _currentRow.moveToNextRow();
  TRI_ASSERT(rowStillValid);
  TRI_ASSERT(_currentRow.isInitialized());

  TRI_ASSERT(_upstreamState != ExecutionState::WAITING);
  if (ADB_UNLIKELY(isLastRowInBlock() && _upstreamState == ExecutionState::DONE)) {
    return ExecutionState::DONE;
  } else {
    return ExecutionState::HASMORE;
  }
}

}  // namespace aql
}  // namespace arangodb

#endif  // ARANGOD_AQL_SINGLE_ROW_FETCHER_H
