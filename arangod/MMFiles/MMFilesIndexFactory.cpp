////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2014-2016 ArangoDB GmbH, Cologne, Germany
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

#include "MMFilesIndexFactory.h"
#include "Basics/Exceptions.h"
#include "Basics/StaticStrings.h"
#include "Basics/VelocyPackHelper.h"
#include "Indexes/Index.h"
#include "MMFiles/MMFilesEdgeIndex.h"
#include "MMFiles/MMFilesFulltextIndex.h"
#include "MMFiles/MMFilesGeoIndex.h"
#include "MMFiles/MMFilesHashIndex.h"
#include "MMFiles/MMFilesPersistentIndex.h"
#include "MMFiles/MMFilesPrimaryIndex.h"
#include "MMFiles/MMFilesSkiplistIndex.h"
#include "MMFiles/MMFilesTtlIndex.h"
#include "MMFiles/mmfiles-fulltext-index.h"
#include "VocBase/LogicalCollection.h"
#include "VocBase/voc-types.h"

#include <velocypack/Builder.h>
#include <velocypack/Iterator.h>
#include <velocypack/Slice.h>
#include <velocypack/velocypack-aliases.h>

#ifdef USE_IRESEARCH
#include "IResearch/IResearchMMFilesLink.h"
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

/// @brief enhances the json of a geo, geo1 or geo2, index
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
      return Result(TRI_ERROR_BAD_PARAMETER, "minLength attribute must be numeric");
    }

    builder.add("minLength", VPackValue(minWordLength));
  }

  return res;
}

MMFilesIndexFactory::MMFilesIndexFactory() {
  emplaceFactory("edge", [](
    LogicalCollection& collection,
    velocypack::Slice const& definition,
    TRI_idx_iid_t id,
    bool isClusterConstructor
  )->std::shared_ptr<Index> {
    if (!isClusterConstructor) {
      // this indexes cannot be created directly
      THROW_ARANGO_EXCEPTION_MESSAGE(
        TRI_ERROR_INTERNAL, "cannot create edge index"
      );
    }

    return std::make_shared<MMFilesEdgeIndex>(id, collection);
  });

  emplaceFactory("fulltext", [](
    LogicalCollection& collection,
    velocypack::Slice const& definition,
    TRI_idx_iid_t id,
    bool isClusterConstructor
  )->std::shared_ptr<Index> {
    return std::make_shared<MMFilesFulltextIndex>(id, collection, definition);
  });

  emplaceFactory("geo1", [](
    LogicalCollection& collection,
    velocypack::Slice const& definition,
    TRI_idx_iid_t id,
    bool isClusterConstructor
  )->std::shared_ptr<Index> {
    return std::make_shared<MMFilesGeoIndex>(id, collection, definition, "geo1");
  });

  emplaceFactory("geo2", [](
    LogicalCollection& collection,
    velocypack::Slice const& definition,
    TRI_idx_iid_t id,
    bool isClusterConstructor
  )->std::shared_ptr<Index> {
    return std::make_shared<MMFilesGeoIndex>(id, collection, definition, "geo2");
  });

  emplaceFactory("geo", [](
    LogicalCollection& collection,
    velocypack::Slice const& definition,
    TRI_idx_iid_t id,
    bool isClusterConstructor
    )->std::shared_ptr<Index> {
    return std::make_shared<MMFilesGeoIndex>(id, collection, definition, "geo");
  });

  emplaceFactory("hash", [](
    LogicalCollection& collection,
    velocypack::Slice const& definition,
    TRI_idx_iid_t id,
    bool isClusterConstructor
  )->std::shared_ptr<Index> {
    return std::make_shared<MMFilesHashIndex>(id, collection, definition);
  });

  emplaceFactory("persistent", [](
    LogicalCollection& collection,
    velocypack::Slice const& definition,
    TRI_idx_iid_t id,
    bool isClusterConstructor
  )->std::shared_ptr<Index> {
    return std::make_shared<MMFilesPersistentIndex>(id, collection, definition);
  });

  emplaceFactory("primary", [](
    LogicalCollection& collection,
    velocypack::Slice const& definition,
    TRI_idx_iid_t id,
    bool isClusterConstructor
  )->std::shared_ptr<Index> {
    if (!isClusterConstructor) {
      // this indexes cannot be created directly
      THROW_ARANGO_EXCEPTION_MESSAGE(
        TRI_ERROR_INTERNAL, "cannot create primary index"
      );
    }

    return std::make_shared<MMFilesPrimaryIndex>(collection);
  });

  emplaceFactory("skiplist", [](
    LogicalCollection& collection,
    velocypack::Slice const& definition,
    TRI_idx_iid_t id,
    bool isClusterConstructor
  )->std::shared_ptr<Index> {
    return std::make_shared<MMFilesSkiplistIndex>(id, collection, definition);
  });
  
  emplaceFactory("ttl", [](
    LogicalCollection& collection,
    velocypack::Slice const& definition,
    TRI_idx_iid_t id,
    bool isClusterConstructor
  )->std::shared_ptr<Index> {
    return std::make_shared<MMFilesTtlIndex>(id, collection, definition);
  });

  emplaceNormalizer("edge", [](
    velocypack::Builder& normalized,
    velocypack::Slice definition,
    bool isCreation
  )->arangodb::Result {
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

  emplaceNormalizer("fulltext", [](
    velocypack::Builder& normalized,
    velocypack::Slice definition,
    bool isCreation
  )->arangodb::Result {
    TRI_ASSERT(normalized.isOpenObject());
    normalized.add(
      arangodb::StaticStrings::IndexType,
      arangodb::velocypack::Value(
        Index::oldtypeName(Index::TRI_IDX_TYPE_FULLTEXT_INDEX)
      )
    );

    return EnhanceJsonIndexFulltext(definition, normalized, isCreation);
  });

  emplaceNormalizer("geo", [](
    velocypack::Builder& normalized,
    velocypack::Slice definition,
    bool isCreation
  )->arangodb::Result {
    TRI_ASSERT(normalized.isOpenObject());
    normalized.add(
      arangodb::StaticStrings::IndexType,
      arangodb::velocypack::Value(
        Index::oldtypeName(Index::TRI_IDX_TYPE_GEO_INDEX)
      )
    );

    return EnhanceJsonIndexGeo(definition, normalized, isCreation, 1, 2);
  });

  emplaceNormalizer("geo1", [](
    velocypack::Builder& normalized,
    velocypack::Slice definition,
    bool isCreation
  )->arangodb::Result {
    TRI_ASSERT(normalized.isOpenObject());
    normalized.add(
      arangodb::StaticStrings::IndexType,
      arangodb::velocypack::Value(
        Index::oldtypeName(Index::TRI_IDX_TYPE_GEO_INDEX)
      )
    );

    return EnhanceJsonIndexGeo(definition, normalized, isCreation, 1, 1);
  });

  emplaceNormalizer("geo2", [](
    velocypack::Builder& normalized,
    velocypack::Slice definition,
    bool isCreation
  )->arangodb::Result {
    TRI_ASSERT(normalized.isOpenObject());
    normalized.add(
      arangodb::StaticStrings::IndexType,
      arangodb::velocypack::Value(
        Index::oldtypeName(Index::TRI_IDX_TYPE_GEO_INDEX)
      )
    );

    return EnhanceJsonIndexGeo(definition, normalized, isCreation, 2, 2);
  });

  emplaceNormalizer("hash", [](
    velocypack::Builder& normalized,
    velocypack::Slice definition,
    bool isCreation
  )->arangodb::Result {
    TRI_ASSERT(normalized.isOpenObject());
    normalized.add(
      arangodb::StaticStrings::IndexType,
      arangodb::velocypack::Value(
        Index::oldtypeName(Index::TRI_IDX_TYPE_HASH_INDEX)
      )
    );

    return EnhanceJsonIndexGeneric(definition, normalized, isCreation);
  });

  emplaceNormalizer("primary", [](
    velocypack::Builder& normalized,
    velocypack::Slice definition,
    bool isCreation
  )->arangodb::Result {
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

  emplaceNormalizer("persistent", [](
    velocypack::Builder& normalized,
    velocypack::Slice definition,
    bool isCreation
  )->arangodb::Result {
    TRI_ASSERT(normalized.isOpenObject());
    normalized.add(
      arangodb::StaticStrings::IndexType,
      arangodb::velocypack::Value(
        Index::oldtypeName(Index::TRI_IDX_TYPE_PERSISTENT_INDEX)
      )
    );

    return EnhanceJsonIndexGeneric(definition, normalized, isCreation);
  });

  emplaceNormalizer("rocksdb", [](
    velocypack::Builder& normalized,
    velocypack::Slice definition,
    bool isCreation
  )->arangodb::Result {
    TRI_ASSERT(normalized.isOpenObject());
    normalized.add(
      arangodb::StaticStrings::IndexType,
      arangodb::velocypack::Value(
        Index::oldtypeName(Index::TRI_IDX_TYPE_PERSISTENT_INDEX)
      )
    );

    return EnhanceJsonIndexGeneric(definition, normalized, isCreation);
  });

  emplaceNormalizer("skiplist", [](
    velocypack::Builder& normalized,
    velocypack::Slice definition,
    bool isCreation
  )->arangodb::Result {
    TRI_ASSERT(normalized.isOpenObject());
    normalized.add(
      arangodb::StaticStrings::IndexType,
      arangodb::velocypack::Value(
        Index::oldtypeName(Index::TRI_IDX_TYPE_SKIPLIST_INDEX)
      )
    );

    return EnhanceJsonIndexGeneric(definition, normalized, isCreation);
  });
  
  emplaceNormalizer("ttl", [](
    velocypack::Builder& normalized,
    velocypack::Slice definition,
    bool isCreation
  )->arangodb::Result {
    TRI_ASSERT(normalized.isOpenObject());
    normalized.add(
      arangodb::StaticStrings::IndexType,
      arangodb::velocypack::Value(
        Index::oldtypeName(Index::TRI_IDX_TYPE_TTL_INDEX)
      )
    );

    return EnhanceJsonIndexTtl(definition, normalized, isCreation);
  });
}

void MMFilesIndexFactory::fillSystemIndexes(
    arangodb::LogicalCollection& col,
    std::vector<std::shared_ptr<arangodb::Index>>& systemIndexes
) const {
    // create primary index
    systemIndexes.emplace_back(std::make_shared<arangodb::MMFilesPrimaryIndex>(col));
    
    // create edges index
    if (TRI_COL_TYPE_EDGE == col.type()) {
      systemIndexes.emplace_back(std::make_shared<arangodb::MMFilesEdgeIndex>(1, col));
    }
  }

void MMFilesIndexFactory::prepareIndexes(
    LogicalCollection& col,
    arangodb::velocypack::Slice const& indexesSlice,
    std::vector<std::shared_ptr<arangodb::Index>>& indexes
) const {
  for (auto const& v : VPackArrayIterator(indexesSlice)) {
    if (basics::VelocyPackHelper::getBooleanValue(v, "error", false)) {
      // We have an error here.
      // Do not add index.
      continue;
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
