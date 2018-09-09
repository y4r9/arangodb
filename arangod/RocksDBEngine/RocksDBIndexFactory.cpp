////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2014-2017 ArangoDB GmbH, Cologne, Germany
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
/// @author Michael Hackstein
////////////////////////////////////////////////////////////////////////////////

#include "RocksDBIndexFactory.h"
#include "Basics/StaticStrings.h"
#include "Basics/StringUtils.h"
#include "Basics/VelocyPackHelper.h"
#include "Cluster/ServerState.h"
#include "Indexes/Index.h"
#include "RocksDBEngine/RocksDBEdgeIndex.h"
#include "RocksDBEngine/RocksDBEngine.h"
#include "RocksDBEngine/RocksDBFulltextIndex.h"
#include "RocksDBEngine/RocksDBGeoIndex.h"
#include "RocksDBEngine/RocksDBHashIndex.h"
#include "RocksDBEngine/RocksDBPersistentIndex.h"
#include "RocksDBEngine/RocksDBPrimaryIndex.h"
#include "RocksDBEngine/RocksDBSkiplistIndex.h"
#include "RocksDBEngine/RocksDBTtlIndex.h"
#include "VocBase/LogicalCollection.h"
#include "VocBase/ticks.h"
#include "VocBase/voc-types.h"

#include <velocypack/Builder.h>
#include <velocypack/Iterator.h>
#include <velocypack/Slice.h>
#include <velocypack/velocypack-aliases.h>

#ifdef USE_IRESEARCH
#include "IResearch/IResearchRocksDBLink.h"
#endif

using namespace arangodb;

/// @brief enhances the json of a hash, skiplist or persistent index
static Result EnhanceJsonIndexGeneric(VPackSlice definition,
                                      VPackBuilder& builder, bool create) {
  Result res = IndexFactory::processIndexFields(definition, builder, 1, INT_MAX, create, true);

  if (res.ok()) {
    IndexFactory::processIndexSparseFlag(definition, builder, create);
    IndexFactory::processIndexUniqueFlag(definition, builder);
    IndexFactory::processIndexDeduplicateFlag(definition, builder);
  }

  return res;
}

/// @brief enhances the json of a ttl index
static Result EnhanceJsonIndexTtl(VPackSlice definition,
                                 VPackBuilder& builder, bool create) {
  Result res = IndexFactory::processIndexFields(definition, builder, 1, 1, create, false);

  if (res.ok()) {
    builder.add(
      arangodb::StaticStrings::IndexSparse,
      arangodb::velocypack::Value(true)
    );
    builder.add(
      arangodb::StaticStrings::IndexUnique,
      arangodb::velocypack::Value(false)
    );
    VPackSlice v = definition.get(StaticStrings::IndexExpireAfter);
    if (!v.isNumber()) {
      return Result(TRI_ERROR_BAD_PARAMETER, "expireAfter attribute must be a number");
    }
    double d = v.getNumericValue<double>();
    if (d <= 0.0) {
      return Result(TRI_ERROR_BAD_PARAMETER, "expireAfter attribute must be a positive number");
    }
    builder.add(
      arangodb::StaticStrings::IndexExpireAfter, v);
  }

  return res;
}

/// @brief enhances the json of a geo, geo1 or geo2 index
static Result EnhanceJsonIndexGeo(VPackSlice definition,
                                  VPackBuilder& builder, bool create,
                                  int minFields, int maxFields) {
  Result res = IndexFactory::processIndexFields(definition, builder, minFields, maxFields, create, false);

  if (res.ok()) {
    builder.add(
      arangodb::StaticStrings::IndexSparse,
      arangodb::velocypack::Value(true)
    );
    builder.add(
      arangodb::StaticStrings::IndexUnique,
      arangodb::velocypack::Value(false)
    );
    IndexFactory::processIndexGeoJsonFlag(definition, builder);
  }

  return res;
}

/// @brief enhances the json of a fulltext index
static Result EnhanceJsonIndexFulltext(VPackSlice definition,
                                       VPackBuilder& builder, bool create) {
  Result res = IndexFactory::processIndexFields(definition, builder, 1, 1, create, false);

  if (res.ok()) {
    // hard-coded defaults
    builder.add(
      arangodb::StaticStrings::IndexSparse,
      arangodb::velocypack::Value(true)
    );
    builder.add(
      arangodb::StaticStrings::IndexUnique,
      arangodb::velocypack::Value(false)
    );

    // handle "minLength" attribute
    int minWordLength = TRI_FULLTEXT_MIN_WORD_LENGTH_DEFAULT;
    VPackSlice minLength = definition.get("minLength");

    if (minLength.isNumber()) {
      minWordLength = minLength.getNumericValue<int>();
    } else if (!minLength.isNull() && !minLength.isNone()) {
      return TRI_ERROR_BAD_PARAMETER;
    }

    builder.add("minLength", VPackValue(minWordLength));
  }

  return res;
}

RocksDBIndexFactory::RocksDBIndexFactory() {
  emplaceFactory("edge",
                 [](LogicalCollection& collection,
                    velocypack::Slice const& definition, TRI_idx_iid_t id,
                    bool isClusterConstructor) -> std::shared_ptr<Index> {
                   if (!isClusterConstructor) {
                     // this indexes cannot be created directly
                     THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_INTERNAL,
                                                    "cannot create edge index");
                   }

                   auto fields =
                     definition.get(arangodb::StaticStrings::IndexFields);
                   TRI_ASSERT(fields.isArray() && fields.length() == 1);
                   auto direction = fields.at(0).copyString();
                   TRI_ASSERT(direction == StaticStrings::FromString ||
                              direction == StaticStrings::ToString);

                   return std::make_shared<RocksDBEdgeIndex>(
                       id, collection, definition, direction);
                 });

  emplaceFactory("fulltext",
                 [](LogicalCollection& collection,
                    velocypack::Slice const& definition, TRI_idx_iid_t id,
                    bool isClusterConstructor) -> std::shared_ptr<Index> {
                   return std::make_shared<RocksDBFulltextIndex>(id, collection,
                                                                 definition);
                 });

  emplaceFactory("geo1",
                 [](LogicalCollection& collection,
                    velocypack::Slice const& definition, TRI_idx_iid_t id,
                    bool isClusterConstructor) -> std::shared_ptr<Index> {
                   return std::make_shared<RocksDBGeoIndex>(id, collection,
                                                            definition, "geo1");
                 });

  emplaceFactory("geo2",
                 [](LogicalCollection& collection,
                    velocypack::Slice const& definition, TRI_idx_iid_t id,
                    bool isClusterConstructor) -> std::shared_ptr<Index> {
                   return std::make_shared<RocksDBGeoIndex>(id, collection,
                                                            definition, "geo2");
                 });

  emplaceFactory("geo",
                 [](LogicalCollection& collection,
                    velocypack::Slice const& definition, TRI_idx_iid_t id,
                    bool isClusterConstructor) -> std::shared_ptr<Index> {
                   return std::make_shared<RocksDBGeoIndex>(id, collection,
                                                            definition, "geo");
                 });

  emplaceFactory("hash",
                 [](LogicalCollection& collection,
                    velocypack::Slice const& definition, TRI_idx_iid_t id,
                    bool isClusterConstructor) -> std::shared_ptr<Index> {
                   return std::make_shared<RocksDBHashIndex>(id, collection,
                                                             definition);
                 });

  emplaceFactory("persistent",
                 [](LogicalCollection& collection,
                    velocypack::Slice const& definition, TRI_idx_iid_t id,
                    bool isClusterConstructor) -> std::shared_ptr<Index> {
                   return std::make_shared<RocksDBPersistentIndex>(
                       id, collection, definition);
                 });

  emplaceFactory("primary",
                 [](LogicalCollection& collection,
                    velocypack::Slice const& definition, TRI_idx_iid_t id,
                    bool isClusterConstructor) -> std::shared_ptr<Index> {
                   if (!isClusterConstructor) {
                     // this indexes cannot be created directly
                     THROW_ARANGO_EXCEPTION_MESSAGE(
                         TRI_ERROR_INTERNAL, "cannot create primary index");
                   }

                   return std::make_shared<RocksDBPrimaryIndex>(collection,
                                                                definition);
                 });

  emplaceFactory("skiplist",
                 [](LogicalCollection& collection,
                    velocypack::Slice const& definition, TRI_idx_iid_t id,
                    bool isClusterConstructor) -> std::shared_ptr<Index> {
                   return std::make_shared<RocksDBSkiplistIndex>(id, collection,
                                                                 definition);
                 });
  
  emplaceFactory("ttl",
                 [](LogicalCollection& collection,
                    velocypack::Slice const& definition, TRI_idx_iid_t id,
                    bool isClusterConstructor) -> std::shared_ptr<Index> {
                   return std::make_shared<RocksDBTtlIndex>(id, collection, definition);
                 });

  emplaceNormalizer(
      "edge",
      [](velocypack::Builder& normalized, velocypack::Slice definition,
         bool isCreation) -> arangodb::Result {
        if (isCreation) {
          // creating these indexes yourself is forbidden
          return TRI_ERROR_FORBIDDEN;
        }

        TRI_ASSERT(normalized.isOpenObject());
        normalized.add(
          arangodb::StaticStrings::IndexType,
          arangodb::velocypack::Value(
            Index::oldtypeName(Index::TRI_IDX_TYPE_EDGE_INDEX)
          )
        );

        return TRI_ERROR_INTERNAL;
      });

  emplaceNormalizer(
      "fulltext",
      [](velocypack::Builder& normalized, velocypack::Slice definition,
         bool isCreation) -> arangodb::Result {
        TRI_ASSERT(normalized.isOpenObject());
        normalized.add(
          arangodb::StaticStrings::IndexType,
            VPackValue(Index::oldtypeName(Index::TRI_IDX_TYPE_FULLTEXT_INDEX)));

        if (isCreation && !ServerState::instance()->isCoordinator() &&
            !definition.hasKey("objectId")) {
          normalized.add("objectId", velocypack::Value(
                                         std::to_string(TRI_NewTickServer())));
        }

        return EnhanceJsonIndexFulltext(definition, normalized, isCreation);
      });

  emplaceNormalizer("geo", [](velocypack::Builder& normalized, velocypack::Slice definition,
         bool isCreation) -> arangodb::Result {
        TRI_ASSERT(normalized.isOpenObject());
        normalized.add(
          arangodb::StaticStrings::IndexType,
          arangodb::velocypack::Value(
            Index::oldtypeName(Index::TRI_IDX_TYPE_GEO_INDEX)
          )
        );

        if (isCreation && !ServerState::instance()->isCoordinator() &&
            !definition.hasKey("objectId")) {
          normalized.add("objectId", velocypack::Value(
                                         std::to_string(TRI_NewTickServer())));
        }

        return EnhanceJsonIndexGeo(definition, normalized, isCreation, 1, 2);
      });

  emplaceNormalizer(
      "geo1",
      [](velocypack::Builder& normalized, velocypack::Slice definition,
         bool isCreation) -> arangodb::Result {
        TRI_ASSERT(normalized.isOpenObject());
        normalized.add(
          arangodb::StaticStrings::IndexType,
          arangodb::velocypack::Value(
            Index::oldtypeName(Index::TRI_IDX_TYPE_GEO_INDEX)
          )
        );

        if (isCreation && !ServerState::instance()->isCoordinator() &&
            !definition.hasKey("objectId")) {
          normalized.add("objectId", velocypack::Value(
                                         std::to_string(TRI_NewTickServer())));
        }

        return EnhanceJsonIndexGeo(definition, normalized, isCreation, 1, 1);
      });

  emplaceNormalizer(
      "geo2",
      [](velocypack::Builder& normalized, velocypack::Slice definition,
         bool isCreation) -> arangodb::Result {
        TRI_ASSERT(normalized.isOpenObject());
        normalized.add(
          arangodb::StaticStrings::IndexType,
          arangodb::velocypack::Value(
            Index::oldtypeName(Index::TRI_IDX_TYPE_GEO_INDEX)
          )
        );

        if (isCreation && !ServerState::instance()->isCoordinator() &&
            !definition.hasKey("objectId")) {
          normalized.add("objectId", velocypack::Value(
                                         std::to_string(TRI_NewTickServer())));
        }

        return EnhanceJsonIndexGeo(definition, normalized, isCreation, 2, 2);
      });

  emplaceNormalizer(
      "hash",
      [](velocypack::Builder& normalized, velocypack::Slice definition,
         bool isCreation) -> arangodb::Result {
        TRI_ASSERT(normalized.isOpenObject());
        normalized.add(
          arangodb::StaticStrings::IndexType,
          arangodb::velocypack::Value(
            Index::oldtypeName(Index::TRI_IDX_TYPE_HASH_INDEX)
          )
        );

        if (isCreation && !ServerState::instance()->isCoordinator() &&
            !definition.hasKey("objectId")) {
          normalized.add("objectId", velocypack::Value(
                                         std::to_string(TRI_NewTickServer())));
        }

        return EnhanceJsonIndexGeneric(definition, normalized, isCreation);
      });

  emplaceNormalizer(
      "primary",
      [](velocypack::Builder& normalized, velocypack::Slice definition,
         bool isCreation) -> arangodb::Result {
        if (isCreation) {
          // creating these indexes yourself is forbidden
          return TRI_ERROR_FORBIDDEN;
        }

        TRI_ASSERT(normalized.isOpenObject());
        normalized.add(
          arangodb::StaticStrings::IndexType,
          arangodb::velocypack::Value(
            Index::oldtypeName(Index::TRI_IDX_TYPE_PRIMARY_INDEX)
          )
        );

        return TRI_ERROR_INTERNAL;
      });

  emplaceNormalizer(
      "persistent",
      [](velocypack::Builder& normalized, velocypack::Slice definition,
         bool isCreation) -> arangodb::Result {
        TRI_ASSERT(normalized.isOpenObject());
        normalized.add(
          arangodb::StaticStrings::IndexType,
          arangodb::velocypack::Value(
            Index::oldtypeName(Index::TRI_IDX_TYPE_PERSISTENT_INDEX)
          )
        );

        if (isCreation && !ServerState::instance()->isCoordinator() &&
            !definition.hasKey("objectId")) {
          normalized.add("objectId", velocypack::Value(
                                         std::to_string(TRI_NewTickServer())));
        }

        return EnhanceJsonIndexGeneric(definition, normalized, isCreation);
      });

  emplaceNormalizer(
      "rocksdb",
      [](velocypack::Builder& normalized, velocypack::Slice definition,
         bool isCreation) -> arangodb::Result {
        TRI_ASSERT(normalized.isOpenObject());
        normalized.add(
          arangodb::StaticStrings::IndexType,
          arangodb::velocypack::Value(
            Index::oldtypeName(Index::TRI_IDX_TYPE_PERSISTENT_INDEX)
          )
        );

        if (isCreation && !ServerState::instance()->isCoordinator() &&
            !definition.hasKey("objectId")) {
          normalized.add("objectId", velocypack::Value(
                                         std::to_string(TRI_NewTickServer())));
        }

        return EnhanceJsonIndexGeneric(definition, normalized, isCreation);
      });

  emplaceNormalizer(
      "skiplist",
      [](velocypack::Builder& normalized, velocypack::Slice definition,
         bool isCreation) -> arangodb::Result {
        TRI_ASSERT(normalized.isOpenObject());
        normalized.add(
          arangodb::StaticStrings::IndexType,
          arangodb::velocypack::Value(
            Index::oldtypeName(Index::TRI_IDX_TYPE_SKIPLIST_INDEX)
          )
        );

        if (isCreation && !ServerState::instance()->isCoordinator() &&
            !definition.hasKey("objectId")) {
          normalized.add("objectId", velocypack::Value(
                                         std::to_string(TRI_NewTickServer())));
        }

        return EnhanceJsonIndexGeneric(definition, normalized, isCreation);
      });
  
  emplaceNormalizer(
      "ttl",
      [](velocypack::Builder& normalized, velocypack::Slice definition,
         bool isCreation) -> arangodb::Result {
        TRI_ASSERT(normalized.isOpenObject());
        normalized.add(
          arangodb::StaticStrings::IndexType,
          arangodb::velocypack::Value(
            Index::oldtypeName(Index::TRI_IDX_TYPE_TTL_INDEX)
          )
        );

        if (isCreation && !ServerState::instance()->isCoordinator() &&
            !definition.hasKey("objectId")) {
          normalized.add("objectId", velocypack::Value(
                                         std::to_string(TRI_NewTickServer())));
        }

        return EnhanceJsonIndexTtl(definition, normalized, isCreation);
      });
}

void RocksDBIndexFactory::fillSystemIndexes(
    arangodb::LogicalCollection& col,
    std::vector<std::shared_ptr<arangodb::Index>>& indexes
) const {
  // create primary index
  VPackBuilder builder;
  builder.openObject();
  builder.close();

  indexes.emplace_back(std::make_shared<RocksDBPrimaryIndex>(col, builder.slice()));

  // create edges indexes
  if (TRI_COL_TYPE_EDGE == col.type()) {
    indexes.emplace_back(std::make_shared<arangodb::RocksDBEdgeIndex>(1, col, builder.slice(), StaticStrings::FromString));
    indexes.emplace_back(std::make_shared<arangodb::RocksDBEdgeIndex>(2, col, builder.slice(), StaticStrings::ToString));
  }
}

/// @brief create indexes from a list of index definitions
void RocksDBIndexFactory::prepareIndexes(
    LogicalCollection& col,
    arangodb::velocypack::Slice const& indexesSlice,
    std::vector<std::shared_ptr<arangodb::Index>>& indexes
) const {
  TRI_ASSERT(indexesSlice.isArray());

  bool splitEdgeIndex = false;
  TRI_idx_iid_t last = 0;

  for (auto const& v : VPackArrayIterator(indexesSlice)) {
    if (arangodb::basics::VelocyPackHelper::getBooleanValue(v, "error",
                                                            false)) {
      // We have an error here.
      // Do not add index.
      // TODO Handle Properly
      continue;
    }

    // check for combined edge index from MMFiles; must split!
    auto value = v.get("type");

    if (value.isString()) {
      std::string tmp = value.copyString();
      arangodb::Index::IndexType const type =
      arangodb::Index::type(tmp.c_str());

      if (type == Index::IndexType::TRI_IDX_TYPE_EDGE_INDEX) {
        VPackSlice fields = v.get("fields");

        if (fields.isArray() && fields.length() == 2) {
          VPackBuilder from;

          from.openObject();

          for (auto const& f : VPackObjectIterator(v)) {
            if (arangodb::StringRef(f.key) == "fields") {
              from.add(VPackValue("fields"));
              from.openArray();
              from.add(VPackValue(StaticStrings::FromString));
              from.close();
            } else {
              from.add(f.key);
              from.add(f.value);
            }
          }

          from.close();

          VPackBuilder to;

          to.openObject();

          for (auto const& f : VPackObjectIterator(v)) {
            if (arangodb::StringRef(f.key) == "fields") {
              to.add(VPackValue("fields"));
              to.openArray();
              to.add(VPackValue(StaticStrings::ToString));
              to.close();
            } else if (arangodb::StringRef(f.key) == "id") {
              auto iid = basics::StringUtils::uint64(f.value.copyString()) + 1;

              last = iid;
              to.add("id", VPackValue(std::to_string(iid)));
            } else {
              to.add(f.key);
              to.add(f.value);
            }
          }

          to.close();

          auto idxFrom = prepareIndexFromSlice(from.slice(), false, col, true);

          if (!idxFrom) {
            LOG_TOPIC(ERR, arangodb::Logger::ENGINES)
              << "error creating index from definition '" << from.slice().toString() << "'";

            continue;
          }

          auto idxTo = prepareIndexFromSlice(to.slice(), false, col, true);

          if (!idxTo) {
            LOG_TOPIC(ERR, arangodb::Logger::ENGINES)
              << "error creating index from definition '" << to.slice().toString() << "'";

            continue;
          }

          indexes.emplace_back(std::move(idxFrom));
          indexes.emplace_back(std::move(idxTo));
          splitEdgeIndex = true;

          continue;
        }
      } else if (splitEdgeIndex) {
        VPackBuilder b;

        b.openObject();

        for (auto const& f : VPackObjectIterator(v)) {
          if (arangodb::StringRef(f.key) == "id") {
            last++;
            b.add("id", VPackValue(std::to_string(last)));
          } else {
            b.add(f.key);
            b.add(f.value);
          }
        }

        b.close();

        auto idx = prepareIndexFromSlice(b.slice(), false, col, true);

        if (!idx) {
          LOG_TOPIC(ERR, arangodb::Logger::ENGINES)
            << "error creating index from definition '" << b.slice().toString() << "'";

          continue;
        }

        indexes.emplace_back(std::move(idx));

        continue;
      }
    }

    auto idx = prepareIndexFromSlice(v, false, col, true);

    if (!idx) {
      LOG_TOPIC(ERR, arangodb::Logger::ENGINES)
        << "error creating index from definition '" << v.toString() << "'";

      continue;
    }

    indexes.emplace_back(std::move(idx));
  }
}
