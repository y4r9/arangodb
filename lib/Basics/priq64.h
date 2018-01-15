///////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2017-2018 ArangoDB GmbH, Cologne, Germany
/// Copyright 2004-2014 triAGENS GmbH, Cologne, Germany
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
/// @author Richard Parker
////////////////////////////////////////////////////////////////////////////////

#ifndef ARANGO_LIB_BASICS_PRIQ64_H
#define ARANGO_LIB_BASICS_PRIQ64_H

#include "Basics/Common.h"

namespace arangodb {
namespace basics {

#define QUEUEEMPTY 0xfffffffffffffffful
#define MEMORYFAIL 0xfffffffffffffffeul
#define MAXPRIQ64  0xffff000000000000ul

typedef struct
{
    uint64_t * heap;
    uint64_t nodect;
    uint64_t alloc;
    uint64_t memoryfail;
} PRIQ64;

uint64_t PriQ64Top(PRIQ64 * pq);
uint64_t * PriQ64Remove(PRIQ64 * pq, uint64_t biggest,
                        uint64_t maxno); 
uint64_t PriQ64Insert(PRIQ64 * pq, uint64_t * list);
PRIQ64 * PriQ64Cons(uint64_t initialsize);
void PriQ64Dest(PRIQ64 * pq);
void PriQ64Dump(PRIQ64 * pq);

}
}

#endif
