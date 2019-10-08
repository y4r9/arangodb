////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2019-2019 ArangoDB GmbH, Cologne, Germany
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

#include "ShadowAqlItemRow.h"

using namespace arangodb;
using namespace arangodb::aql;

/// @brief get the number of data registers in the underlying block.
///        Not all of these registers are necessarily filled by this
///        ShadowRow. There might be empty registers on deeper levels.
std::size_t ShadowAqlItemRow::getNrRegisters() const noexcept {
  return block().getNrRegs();
}

/// @brief a ShadowRow is relevant iff it indicates an end of subquery block on the subquery context
///        we are in right now. This will only be of importance on nested subqueries.
///        Within the inner subquery all shadowrows of this inner are relavant. All shadowRows
///        of the outer subquery are NOT relevant
///        Also note: There is a guarantee that a non-relevant shadowrow, can only be encountered
///        right after a shadowrow. And only in descending nesting level. (eg 1. inner most, 2. inner, 3. outer most)
bool ShadowAqlItemRow::isRelevant() const noexcept { return getDepth() == 0; }

/// @brief Test if this shadow row is initialized, eg has a block and has a valid depth.
bool ShadowAqlItemRow::isInitialized() const {
#ifdef ARANGODB_ENABLE_MAINTAINER_MODE
  if (_block != nullptr) {
    // The value needs to always be a positive integer.
    auto depthVal = block().getShadowRowDepth(_baseIndex);
    TRI_ASSERT(depthVal.isNumber());
    TRI_ASSERT(depthVal.toInt64() >= 0);
  }
#endif
  return _block != nullptr;
}

#ifdef ARANGODB_ENABLE_MAINTAINER_MODE
/**
 * @brief Compare the underlying block. Only for assertions.
 */
bool ShadowAqlItemRow::internalBlockIs(SharedAqlItemBlockPtr const& other) const {
  return _block == other;
}
#endif

/**
 * @brief Get a reference to the value of the given Variable Nr
 *
 * @param registerId The register ID of the variable to read.
 *
 * @return Reference to the AqlValue stored in that variable.
 */
AqlValue const& ShadowAqlItemRow::getValue(RegisterId registerId) const {
  TRI_ASSERT(isInitialized());
  TRI_ASSERT(registerId < getNrRegisters());
  return block().getValueReference(_baseIndex, registerId);
}

/// @brief get the depthValue of the shadow row as AqlValue
AqlValue const& ShadowAqlItemRow::getShadowDepthValue() const {
  TRI_ASSERT(isInitialized());
  return block().getShadowRowDepth(_baseIndex);
}

/// @brief get the depthValue of the shadow row as int64_t >= 0
///        NOTE: Innermost query will have depth 0. Outermost query wil have highest depth.
uint64_t ShadowAqlItemRow::getDepth() const {
  TRI_ASSERT(isInitialized());
  auto value = block().getShadowRowDepth(_baseIndex);
  TRI_ASSERT(value.toInt64() >= 0);
  return static_cast<uint64_t>(value.toInt64());
}

AqlItemBlock& ShadowAqlItemRow::block() noexcept {
  TRI_ASSERT(_block != nullptr);
  return *_block;
}

AqlItemBlock const& ShadowAqlItemRow::block() const noexcept {
  TRI_ASSERT(_block != nullptr);
  return *_block;
}