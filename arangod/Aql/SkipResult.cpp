
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

#include "SkipResult.h"

#include "Cluster/ResultT.h"

#include <velocypack/Builder.h>
#include <velocypack/Iterator.h>
#include <velocypack/velocypack-aliases.h>

using namespace arangodb::aql;

auto SkipResult::SkipBatch::fromVelocyPack(VPackSlice slice) -> arangodb::ResultT<SkipBatch> {
  if (!slice.isArray()) {
    auto message = std::string{
        "When deserializating AqlExecuteResult: When reading skipped: "
        "Unexpected type "};
    message += slice.typeName();
    return Result(TRI_ERROR_TYPE_ERROR, std::move(message));
  }
  if (slice.isEmptyArray()) {
    auto message = std::string{
        "When deserializating AqlExecuteResult: When reading skipped: "
        "Got an empty list of skipped values."};
    return Result(TRI_ERROR_TYPE_ERROR, std::move(message));
  }
  try {
    SkipBatch res;
    res._entries.clear();
    res._entries.reserve(slice.length());
    auto it = VPackArrayIterator(slice);
    while (it.valid()) {
      auto val = it.value();
      if (!val.isInteger()) {
        auto message = std::string{
            "When deserializating AqlExecuteResult: When reading skipped: "
            "Unexpected type "};
        message += slice.typeName();
        return Result(TRI_ERROR_TYPE_ERROR, std::move(message));
      }
      res._entries.emplace_back(val.getNumber<std::size_t>());
      ++it;
    }
    return {res};
  } catch (velocypack::Exception const& ex) {
    auto message = std::string{
        "When deserializating AqlExecuteResult: When reading skipped: "};
    message += ex.what();
    return Result(TRI_ERROR_TYPE_ERROR, std::move(message));
  }
}


auto SkipResult::SkipBatch::getSkipCount() const noexcept -> size_t {
  TRI_ASSERT(!_entries.empty());
  return _entries.back();
}

auto SkipResult::SkipBatch::nextSubqueryRun() -> void {
  _entries.emplace_back(0);
}

auto SkipResult::SkipBatch::didSkip(size_t skipped) -> void {
  TRI_ASSERT(!_entries.empty());
  _entries.back() += skipped;
}

auto SkipResult::SkipBatch::toVelocyPack(VPackBuilder& builder) const noexcept -> void {
  VPackArrayBuilder guard(&builder);
  TRI_ASSERT(!_entries.empty());
  for (auto const& s : _entries) {
    builder.add(VPackValue(s));
  }
}

auto SkipResult::SkipBatch::reset() -> void {
  TRI_ASSERT(!_entries.empty());
  _entries.clear();
  _entries.emplace_back(0);
}

auto SkipResult::SkipBatch::merge(SkipBatch const& other) noexcept -> void {
  // The other side can only be larger
  TRI_ASSERT(other._entries.size() >= _entries.size());
  _entries.reserve(other._entries.size());
  for (size_t i = 0; i < other._entries.size(); ++i) {
    _entries[i] += other._entries[i];
  }
}

auto SkipResult::SkipBatch::getSkipBatches() const noexcept -> MyVector<size_t> const& {
  return _entries;
}

auto SkipResult::SkipBatch::operator==(SkipBatch const& b) const noexcept -> bool {
  if (_entries.size() != b._entries.size()) {
    return false;
  }
  for (size_t i = 0; i < _entries.size(); ++i) {
    if (_entries[i] != b._entries[i]) {
      return false;
    }
  }
  return true;
}

auto SkipResult::SkipBatch::operator!=(SkipBatch const& b) const noexcept -> bool {
  return !(*this == b);
}

SkipResult::SkipResult() {}

SkipResult::SkipResult(SkipResult const& other) : _skipped{other._skipped} {}

auto SkipResult::getSkipCount() const noexcept -> size_t {
  TRI_ASSERT(!_skipped.empty());
  return _skipped.back().getSkipCount();
}

auto SkipResult::getSkipBatches() const noexcept -> MyVector<size_t> const& {
      TRI_ASSERT(!_skipped.empty());
  return _skipped.back().getSkipBatches();
}


auto SkipResult::didSkip(size_t skipped) -> void {
  TRI_ASSERT(!_skipped.empty());
  _skipped.back().didSkip(skipped);
}

auto SkipResult::didSkipSubquery(size_t skipped, size_t depth) -> void {
  TRI_ASSERT(!_skipped.empty());
  TRI_ASSERT(_skipped.size() > depth + 1);
  size_t index = _skipped.size() - depth - 2;
  _skipped.at(index).didSkip(skipped);
}

auto SkipResult::getSkipOnSubqueryLevel(size_t depth) -> size_t {
  TRI_ASSERT(!_skipped.empty());
  TRI_ASSERT(_skipped.size() > depth);
  return _skipped.at(depth).getSkipCount();
}

auto SkipResult::nothingSkipped() const noexcept -> bool {
  TRI_ASSERT(!_skipped.empty());
  return std::all_of(_skipped.begin(), _skipped.end(),
                     [](auto const& e) -> bool { return e.getSkipCount() == 0; });
}

auto SkipResult::toVelocyPack(VPackBuilder& builder) const noexcept -> void {
  VPackArrayBuilder guard(&builder);
  TRI_ASSERT(!_skipped.empty());
  for (auto const& s : _skipped) {
    s.toVelocyPack(builder);
  }
}

auto SkipResult::fromVelocyPack(VPackSlice slice) -> arangodb::ResultT<SkipResult> {
  if (!slice.isArray()) {
    auto message = std::string{
        "When deserializating AqlExecuteResult: When reading skipped: "
        "Unexpected type "};
    message += slice.typeName();
    return Result(TRI_ERROR_TYPE_ERROR, std::move(message));
  }
  if (slice.isEmptyArray()) {
    auto message = std::string{
        "When deserializating AqlExecuteResult: When reading skipped: "
        "Got an empty list of skipped values."};
    return Result(TRI_ERROR_TYPE_ERROR, std::move(message));
  }
  try {
    SkipResult res;
    res._skipped.clear();
    res._skipped.reserve(slice.length());
    auto it = VPackArrayIterator(slice);
    while (it.valid()) {
      auto batch = SkipBatch::fromVelocyPack(it.value());
      if (batch.fail()) {
        return batch.result();
      }
      res._skipped.emplace_back(std::move(batch.get()));
      ++it;
    }
    return {res};
  } catch (velocypack::Exception const& ex) {
    auto message = std::string{
        "When deserializating AqlExecuteResult: When reading skipped: "};
    message += ex.what();
    return Result(TRI_ERROR_TYPE_ERROR, std::move(message));
  }
}

auto SkipResult::incrementSubquery() -> void { _skipped.emplace_back(SkipBatch{}); }
auto SkipResult::decrementSubquery() -> void {
  TRI_ASSERT(!_skipped.empty());
  _skipped.pop_back();
  TRI_ASSERT(!_skipped.empty());
}
auto SkipResult::subqueryDepth() const noexcept -> size_t {
  TRI_ASSERT(!_skipped.empty());
  return _skipped.size();
}

auto SkipResult::reset() -> void {
  for (size_t i = 0; i < _skipped.size(); ++i) {
    _skipped[i].reset();
  }
}

auto SkipResult::nextSubqueryRun() -> void {
  _skipped.back().nextSubqueryRun();
}

auto SkipResult::merge(SkipResult const& other, bool excludeTopLevel) noexcept -> void {
  _skipped.reserve(other.subqueryDepth());
  while (other.subqueryDepth() > subqueryDepth()) {
    incrementSubquery();
  }
  TRI_ASSERT(other._skipped.size() <= _skipped.size());
  for (size_t i = 0; i < other._skipped.size(); ++i) {
    if (excludeTopLevel && i + 1 == other._skipped.size()) {
      // Do not copy top level
      continue;
    }
    _skipped[i].merge(other._skipped[i]);
  }
}

auto SkipResult::mergeOnlyTopLevel(SkipResult const& other) noexcept -> void {
  _skipped.reserve(other.subqueryDepth());
  while (other.subqueryDepth() > subqueryDepth()) {
    incrementSubquery();
  }
  _skipped.back().merge(other._skipped.back());
}

auto SkipResult::operator+=(SkipResult const& b) noexcept -> SkipResult& {
  didSkip(b.getSkipCount());
  return *this;
}

auto SkipResult::operator==(SkipResult const& b) const noexcept -> bool {
  if (_skipped.size() != b._skipped.size()) {
    return false;
  }
  for (size_t i = 0; i < _skipped.size(); ++i) {
    if (_skipped[i] != b._skipped[i]) {
      return false;
    }
  }
  return true;
}

auto SkipResult::operator!=(SkipResult const& b) const noexcept -> bool {
  return !(*this == b);
}
namespace arangodb::aql {
std::ostream& operator<<(std::ostream& stream, arangodb::aql::SkipResult const& result) {
  VPackBuilder temp;
  result.toVelocyPack(temp);
  stream << temp.toJson();
  return stream;
}
}  // namespace arangodb::aql
