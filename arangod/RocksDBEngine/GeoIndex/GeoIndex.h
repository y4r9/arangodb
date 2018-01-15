////////////////////////////////////////////////////////////////////////////////
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

#ifndef ARANGOD_ROCKSDBENGINE_GEOINDEX_GEOINDEX_H
#define ARANGOD_ROCKSDBENGINE_GEOINDEX_GEOINDEX_H 1

#include "Basics/Common.h"

#include "Basics/priq64.h"

#include "RocksDBEngine/GeoIndex/stones.h"
#include "RocksDBEngine/GeoIndex/GeoIndex.h"

using namespace arangodb::basics;

namespace arangodb {
namespace geoindex {

/*   Main GeoIndex routines header file   */

#define NOMAXMDF 3.0

typedef struct
{
    long type;    // 1=Geo (later 2=Event, 3=BigPoint)
} GeoDetails;
// will need "speed" and "radius" later


typedef struct
{
    STON * st;
}  GeoIx;

typedef struct
{

    double * latitudes;
    double * longitudes;
    uint64_t * payloads;
    float * mdfs;
    float nextmdf;
    uint64_t pointcount;
    int errorcode;
}  GeoPoints;

typedef struct
{
    long type;
    double tarlat;
    double tarlong;
    float maxmdf;
}  GeoQuery;

typedef struct
{
    uint64_t payfree;
    double latitude;
    double longitude;
}  PF3;

typedef struct
{
    GeoIx * gix;
    int type;
    double Tarlat;
    double Tarlong;
    double TX, TY, TZ;
    float maxmdf;
    PF3 * pf3;
    long pf3alloc;
    uint64_t freechain;
    PRIQ64 * ptheap;
}  GeoCursor;

GeoIx * GeoCreate(GeoParm * gp, GeoDetails * gd);
void GeoDrop(GeoParm * gp);
void GeoDisconnect(GeoIx * gix);
GeoIx * GeoConnect(GeoParm * gp, GeoDetails * gd);
int GeoIns(GeoIx * gix, void * trans, GeoPoints * gpt);
int GeoDel(GeoIx * gix, void * trans, GeoPoints * gpt);
GeoCursor * GeoIQuery(GeoIx * gix, void * trans, GeoQuery * gq);
GeoPoints * GeoReadCursor(GeoCursor * gc, uint64_t pointswanted,
                           float maxmdf);
void GeoDestCursor(GeoCursor * gq);
void GeoDestPoints(GeoPoints * gp);
void GeoMDFToMeters(float * mdf, uint64_t pct, double * meters);
float GeoMetersToMDF(double meters);

}
}

#endif
