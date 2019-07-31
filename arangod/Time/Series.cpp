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
/// @author Simon Grätzer
////////////////////////////////////////////////////////////////////////////////

#include "Series.h"

#include "Aql/AstNode.h"
#include "Basics/Common.h"
#include "Basics/datetime.h"
#include "Basics/Exceptions.h"

#include <cmath>

#include <velocypack/Iterator.h>
#include <velocypack/velocypack-aliases.h>

using namespace arangodb::time;

LabelInfo::LabelInfo(VPackSlice info)
  : name(info.get("name").copyString()),
    numBuckets(info.get("buckets").getNumber<uint16_t>()) {}

void LabelInfo::toVelocyPack(VPackBuilder& b) const {
  b.openObject(/*unindexed*/true);
  b.add("name", VPackValue(name));
  b.add("buckets", VPackValue(numBuckets));
  b.close();
}

Series::Series(VPackSlice info)
: labels() {
  
  VPackSlice bs = info.get("labels");
  if (bs.isNone()) {
    return;
  }
  if (!bs.isArray()) {
    THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_BAD_PARAMETER, "labels needs to be an array");
  }
  uint64_t prod = 1;
  for (VPackSlice slice : VPackArrayIterator(bs)) {
    labels.emplace_back(slice);
    if (labels.back().numBuckets == 0) {
      THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_BAD_PARAMETER, "wrong bucket size");
    }
    prod *= labels.back().numBuckets;
    if (prod >= std::numeric_limits<uint16_t>::max()) {
      THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_BAD_PARAMETER, "too many buckets");
    }
  }
}

void Series::toVelocyPack(VPackBuilder& b) const {
  b.add(VPackValue("labels"));
  b.openArray(/*unindexed*/true);
  for (LabelInfo const& i : labels) {
    i.toVelocyPack(b);
  }
  b.close();
}


/// calculate 2-byte bucket ID
uint16_t Series::bucketId(arangodb::velocypack::Slice slice) const {
  TRI_ASSERT(slice.isObject());

  uint64_t bucketId = 0;
  uint64_t prodBuckets = 1;
  for (LabelInfo const& label : labels) {
    VPackSlice key = slice.get(label.name);
    uint64_t hash = 0;
    if (!key.isNone() && !key.isNull()) {
      hash = key.normalizedHash();
      hash = hash % label.numBuckets;
    }
    bucketId += hash * prodBuckets;
    prodBuckets *= label.numBuckets;
  }
  TRI_ASSERT(bucketId <= std::numeric_limits<uint16_t>::max());
  
  return bucketId;
}

// std::vector<Series::Range>
uint16_t Series::bucketId(arangodb::aql::AstNode const& node) const {
  TRI_ASSERT(node.isSimple());
  VPackBuilder b;
  node.toVelocyPackValue(b);
  
  if (labels.size() == 1) {
    return static_cast<uint16_t>(b.slice().normalizedHash() % labels[0].numBuckets);
  }
  TRI_ASSERT(false);
  
  return 0;
}

namespace arangodb {
namespace time {

uint64_t to_timevalue(arangodb::aql::AstNode const* node) {
  using namespace arangodb::aql;
  if (node->isStringValue()) {
    // FIXME do not copy
    auto str = node->getString();
    arangodb::tp_sys_clock_ms tp;  // unused
    bool success = arangodb::basics::parseDateTime(str, tp);
    
    if (success) {
      return std::chrono::duration_cast<std::chrono::nanoseconds>(tp.time_since_epoch()).count();
    }
    return 0;
  } else if(node->isNumericValue()) {
    return node->getIntValue() * 1e9;
  }
  THROW_ARANGO_EXCEPTION(TRI_ERROR_BAD_PARAMETER);
  return 0;
}
  
uint64_t to_timevalue(arangodb::velocypack::Slice const slice) {
  using namespace arangodb::aql;
  if (slice.isString()) {
    // FIXME do not copy
    auto str = slice.copyString();
    arangodb::tp_sys_clock_ms tp;  // unused
    bool success = arangodb::basics::parseDateTime(str, tp);
    
    if (success) {
      return std::chrono::duration_cast<std::chrono::nanoseconds>(tp.time_since_epoch()).count();
    }
    return 0;
  } else if (slice.isDouble()) {
    return static_cast<uint64_t>(slice.getNumber<double>() * 1e9);
  } else if (slice.isInteger()) {
    return static_cast<uint64_t>(slice.getInt() * 1e9);
  }
  THROW_ARANGO_EXCEPTION(TRI_ERROR_BAD_PARAMETER);
  return 0;
}

}
}
