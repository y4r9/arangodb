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

#include "LogEntry.h"

#include "Basics/Endian.h"

namespace arangodb {
namespace lifeboat {

void LogEntry::constructSingleInsert(TRI_voc_cid_t cid, char const* ptr, size_t size) {
  this->type = LogType::SingleInsert;
  this->cid = cid;
  this->data.assign(ptr, size);
}
  
void LogEntry::constructSingleRemove(TRI_voc_cid_t cid, char const* ptr, size_t size) {
  this->type = LogType::SingleInsert;
  this->cid = cid;
  this->data.assign(ptr, size);
}

void LogEntry::constructTrxBegin(TRI_voc_tid_t tid, std::vector<TRI_voc_cid_t> collections) {
  this->type = LogType::SingleInsert;
  this->cid = cid;
  this->data.assign(ptr, size);
  for (TRI_voc_cid_t cid : collections) {
    
  }
}
  
void LogEntry::constructTrxEnd(TRI_voc_tid_t tid, std::vector<TRI_voc_cid_t> collections);
void LogEntry::constructTrxInsert(TRI_voc_tid_t, TRI_voc_cid_t cid, char const* ptr, size_t size);
void LogEntry::constructTrxRemove(TRI_voc_tid_t, TRI_voc_cid_t cid, char const* ptr, size_t size);


} // namespace lifeboat
} // namespace arangodb
