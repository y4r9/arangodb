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

#ifndef ARANGOD_LIFEBOAT_LOG_ENTRY_H
#define ARANGOD_LIFEBOAT_LOG_ENTRY_H 1

#include "VocBase/voc-types.h"

namespace arangodb {
namespace lifeboat {
  
enum class LogType {
  SingleInsert = 1,
  SingleDelete = 2,
  TrxBegin = 3,
  TrxEnd = 4,
  Insert = 5,
  Delete = 6
};
  
/// OpLog entry
struct LogEntry {
  uint64_t raftTerm;
  uint64_t raftIndex;
  TRI_voc_tick_t logTick; /// local log tick
  
  
  LogType type;
  TRI_voc_cid_t cid; /// optional CID this belongs to
  std::string data;
  
  void constructSingleInsert(TRI_voc_cid_t, char const* ptr, size_t size);
  void constructSingleRemove(TRI_voc_cid_t, char const* ptr, size_t size);

  void constructTrxBegin(TRI_voc_tid_t, std::vector<TRI_voc_cid_t> collections);
  void constructTrxEnd(TRI_voc_tid_t, std::vector<TRI_voc_cid_t> collections);
  void constructTrxInsert(TRI_voc_tid_t, TRI_voc_cid_t, char const* ptr, size_t size);
  void constructTrxRemove(TRI_voc_tid_t, TRI_voc_cid_t, char const* ptr, size_t size);
};
  
} // namespace lifeboat
} // namespace arangodb

#endif
