////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2014-2021 ArangoDB GmbH, Cologne, Germany
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
/// @author Simon Grätzer
////////////////////////////////////////////////////////////////////////////////

#include "ClusterRestCollectionHandler.h"
#include "ClusterEngine/RocksDBMethods.h"
#include "Cluster/ClusterFeature.h"
#include "Cluster/ClusterInfo.h"
#include "Cluster/ServerState.h"
#include "Network/Methods.h"
#include "VocBase/LogicalCollection.h"

using namespace arangodb;

ClusterRestCollectionHandler::ClusterRestCollectionHandler(application_features::ApplicationServer& server,
                                                           GeneralRequest* request,
                                                           GeneralResponse* response)
    : RestCollectionHandler(server, request, response) {}

Result ClusterRestCollectionHandler::handleExtraCommandPut(std::shared_ptr<LogicalCollection> coll,
                                                           std::string const& suffix,
                                                           velocypack::Builder& builder) {
  if (suffix == "recalculateCount") {
    Result res =
        arangodb::rocksdb::recalculateCountsOnAllDBServers(server(), _vocbase.name(),
                                                           coll->name());
    if (res.ok()) {
      VPackObjectBuilder guard(&builder);
      builder.add("result", VPackValue(true));
    }
    return res;
  } else if (suffix == "dropShard") {
    std::string const& shard = _request->value("shard");

    if (shard.empty()) {
      return {TRI_ERROR_BAD_PARAMETER};
    }

    network::RequestOptions reqOpts;
    reqOpts.database = _vocbase.name();
    reqOpts.timeout = network::Timeout(1200.0);

    std::vector<futures::Future<network::Response>> futures;
                              
    VPackBuffer<uint8_t> b;
    b.append(VPackSlice::emptyObjectSlice().start(), 1);

    auto* pool = _vocbase.server().getFeature<NetworkFeature>().pool();
    for (auto const& s : _vocbase.server().getFeature<ClusterFeature>().clusterInfo().getCurrentDBServers()) {
      auto future =
         network::sendRequest(pool, "server:" + s, fuerte::RestVerb::Delete,
                              "/_api/collection/" + shard,
                              b, reqOpts);
      futures.emplace_back(std::move(future));
    }
       
    auto responses = futures::collectAll(futures).get();
    Result res;
    for (auto const& it : responses) {
      auto& resp = it.get();
      res.reset(resp.combinedResult());
      if (res.is(TRI_ERROR_ARANGO_DATA_SOURCE_NOT_FOUND)) {
        res.reset();
        continue;
      }

      if (res.fail()) {
        break;
      }
    }
 
    return res;
  }

  return TRI_ERROR_NOT_IMPLEMENTED;
}
