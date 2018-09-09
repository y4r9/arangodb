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

#include "IndexFactory.h"
#include "Basics/AttributeNameParser.h"
#include "Basics/Exceptions.h"
#include "Basics/StaticStrings.h"
#include "Basics/StringRef.h"
#include "Basics/StringUtils.h"
#include "Basics/VelocyPackHelper.h"
#include "Cluster/ServerState.h"
#include "Indexes/Index.h"
#include "RestServer/BootstrapFeature.h"

#include <velocypack/Iterator.h>
#include <velocypack/Slice.h>
#include <velocypack/velocypack-aliases.h>

namespace arangodb {

void IndexFactory::clear() {
  _factories.clear();
  _normalizers.clear();
}

Result IndexFactory::emplaceFactory(
  std::string const& type,
  IndexTypeFactory const& factory
) {
  if (!factory) {
    return arangodb::Result(
      TRI_ERROR_BAD_PARAMETER,
      std::string("index factory undefined during index factory registration for index type '") + type + "'"
    );
  }

  auto* feature =
    arangodb::application_features::ApplicationServer::lookupFeature("Bootstrap");
  auto* bootstrapFeature = dynamic_cast<BootstrapFeature*>(feature);

  // ensure new factories are not added at runtime since that would require additional locks
  if (bootstrapFeature && bootstrapFeature->isReady()) {
    return arangodb::Result(
      TRI_ERROR_INTERNAL,
      std::string("index factory registration is only allowed during server startup")
    );
  }

  if (!_factories.emplace(type, factory).second) {
    return arangodb::Result(
      TRI_ERROR_ARANGO_DUPLICATE_IDENTIFIER,
      std::string("index factory previously registered during index factory registration for index type '") + type + "'"
    );
  }

  return arangodb::Result();
}

Result IndexFactory::emplaceNormalizer(
  std::string const& type,
  IndexNormalizer const& normalizer
) {
  if (!normalizer) {
    return arangodb::Result(
      TRI_ERROR_BAD_PARAMETER,
      std::string("index normalizer undefined during index normalizer registration for index type '") + type + "'"
    );
  }

  auto* feature =
    arangodb::application_features::ApplicationServer::lookupFeature("Bootstrap");
  auto* bootstrapFeature = dynamic_cast<BootstrapFeature*>(feature);

  // ensure new normalizers are not added at runtime since that would require additional locks
  if (bootstrapFeature && bootstrapFeature->isReady()) {
    return arangodb::Result(
      TRI_ERROR_INTERNAL,
      std::string("index normalizer registration is only allowed during server startup")
    );
  }

  if (!_normalizers.emplace(type, normalizer).second) {
    return arangodb::Result(
      TRI_ERROR_ARANGO_DUPLICATE_IDENTIFIER,
      std::string("index normalizer previously registered during index normalizer registration for index type '") + type + "'"
    );
  }

  return arangodb::Result();
}

Result IndexFactory::enhanceIndexDefinition(
  velocypack::Slice const definition,
  velocypack::Builder& normalized,
  bool isCreation,
  bool isCoordinator
) const {
  auto type = definition.get(StaticStrings::IndexType);

  if (!type.isString()) {
    return Result(TRI_ERROR_BAD_PARAMETER, "invalid index type");
  }

  auto typeString = type.copyString();
  auto itr = _normalizers.find(typeString);

  if (itr == _normalizers.end()) {
    return Result(TRI_ERROR_BAD_PARAMETER, "invalid index type");
  }

  TRI_ASSERT(ServerState::instance()->isCoordinator() == isCoordinator);
  TRI_ASSERT(normalized.isEmpty());

  try {
    velocypack::ObjectBuilder b(&normalized);
    auto idSlice = definition.get(StaticStrings::IndexId);
    uint64_t id = 0;

    if (idSlice.isNumber()) {
      id = idSlice.getNumericValue<uint64_t>();
    } else if (idSlice.isString()) {
      id = basics::StringUtils::uint64(idSlice.copyString());
    }

    if (id) {
      normalized.add(
        StaticStrings::IndexId,
        arangodb::velocypack::Value(std::to_string(id))
      );
    }

    return itr->second(normalized, definition, isCreation);
  } catch (basics::Exception const& ex) {
    return Result(ex.code(), ex.what());
  } catch (std::exception const&) {
    return TRI_ERROR_INTERNAL;
  } catch (...) {
    return TRI_ERROR_INTERNAL;
  }
}

std::shared_ptr<Index> IndexFactory::prepareIndexFromSlice(
  velocypack::Slice definition,
  bool generateKey,
  LogicalCollection& collection,
  bool isClusterConstructor
) const {
  auto id = validateSlice(definition, generateKey, isClusterConstructor);
  auto type = definition.get(StaticStrings::IndexType);

  if (!type.isString()) {
    THROW_ARANGO_EXCEPTION_MESSAGE(
      TRI_ERROR_BAD_PARAMETER, "invalid index type definition"
    );
  }

  auto typeString = type.copyString();
  auto itr = _factories.find(typeString);

  if (itr == _factories.end()) {
    THROW_ARANGO_EXCEPTION_MESSAGE(
      TRI_ERROR_NOT_IMPLEMENTED,
      std::string("invalid or unsupported index type '") + typeString + "'"
    );
  }

  return itr->second(collection, definition, id, isClusterConstructor);
}

std::vector<std::string> IndexFactory::supportedIndexes() const {
  std::vector<std::string> result;
  result.reserve(_factories.size());
  for (auto const& it : _factories) {
    result.emplace_back(it.first);
  }
  return result;
}

TRI_idx_iid_t IndexFactory::validateSlice(arangodb::velocypack::Slice info, 
                                          bool generateKey, 
                                          bool isClusterConstructor) {
  if (!info.isObject()) {
    THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_BAD_PARAMETER, "expecting object for index definition");
  }

  TRI_idx_iid_t iid = 0;
  auto value = info.get(StaticStrings::IndexId);

  if (value.isString()) {
    iid = basics::StringUtils::uint64(value.copyString());
  } else if (value.isNumber()) {
    iid = basics::VelocyPackHelper::getNumericValue<TRI_idx_iid_t>(
      info, StaticStrings::IndexId.c_str(), 0
    );
  } else if (!generateKey) {
    // In the restore case it is forbidden to NOT have id
    THROW_ARANGO_EXCEPTION_MESSAGE(
        TRI_ERROR_INTERNAL, "cannot restore index without index identifier");
  }

  if (iid == 0 && !isClusterConstructor) {
    // Restore is not allowed to generate an id
    TRI_ASSERT(generateKey);
    iid = arangodb::Index::generateId();
  }

  return iid;
}

/// @brief process the fields list, deduplicate it, and add it to the json
Result IndexFactory::processIndexFields(VPackSlice definition, VPackBuilder& builder,
                                        size_t minFields, size_t maxField, bool create,
                                        bool allowExpansion) {
  TRI_ASSERT(builder.isOpenObject());
  std::unordered_set<StringRef> fields;
  auto fieldsSlice = definition.get(arangodb::StaticStrings::IndexFields);

  builder.add(
    arangodb::velocypack::Value(arangodb::StaticStrings::IndexFields)
  );
  builder.openArray();

  if (fieldsSlice.isArray()) {
    // "fields" is a list of fields
    for (auto const& it : VPackArrayIterator(fieldsSlice)) {
      if (!it.isString()) {
        return Result(TRI_ERROR_BAD_PARAMETER, "index field names must be strings");
      }

      StringRef f(it);

      if (f.empty() || (create && f == StaticStrings::IdString)) {
        // accessing internal attributes is disallowed
        return Result(TRI_ERROR_BAD_PARAMETER, "_id attribute cannot be indexed");
      }

      if (fields.find(f) != fields.end()) {
        // duplicate attribute name
        return Result(TRI_ERROR_BAD_PARAMETER, "duplicate attribute name in index fields list");
      }

      std::vector<basics::AttributeName> temp;
      TRI_ParseAttributeString(f, temp, allowExpansion);

      fields.insert(f);
      builder.add(it);
    }
  }

  size_t cc = fields.size();
  if (cc == 0 || cc < minFields || cc > maxField) {
    return Result(TRI_ERROR_BAD_PARAMETER, "invalid number of index attributes");
  }

  builder.close();
  return Result();
}

/// @brief process the unique flag and add it to the json
void IndexFactory::processIndexUniqueFlag(VPackSlice definition,
                                          VPackBuilder& builder) {
  bool unique = basics::VelocyPackHelper::getBooleanValue(
    definition, arangodb::StaticStrings::IndexUnique.c_str(), false
  );

  builder.add(
    arangodb::StaticStrings::IndexUnique,
    arangodb::velocypack::Value(unique)
  );
}

/// @brief process the sparse flag and add it to the json
void IndexFactory::processIndexSparseFlag(VPackSlice definition,
                                          VPackBuilder& builder, bool create) {
  if (definition.hasKey(arangodb::StaticStrings::IndexSparse)) {
    bool sparseBool = basics::VelocyPackHelper::getBooleanValue(
      definition, arangodb::StaticStrings::IndexSparse.c_str(), false
    );

    builder.add(
      arangodb::StaticStrings::IndexSparse,
      arangodb::velocypack::Value(sparseBool)
    );
  } else if (create) {
    // not set. now add a default value
    builder.add(
      arangodb::StaticStrings::IndexSparse,
      arangodb::velocypack::Value(false)
    );
  }
}

/// @brief process the deduplicate flag and add it to the json
void IndexFactory::processIndexDeduplicateFlag(VPackSlice definition,
                                               VPackBuilder& builder) {
  bool dup = basics::VelocyPackHelper::getBooleanValue(definition,
                                                       "deduplicate", true);
  builder.add("deduplicate", VPackValue(dup));
}

/// @brief process the geojson flag and add it to the json
void IndexFactory::processIndexGeoJsonFlag(VPackSlice definition,
                                           VPackBuilder& builder) {
  auto fieldsSlice = definition.get(arangodb::StaticStrings::IndexFields);

  if (fieldsSlice.isArray() && fieldsSlice.length() == 1) {
    // only add geoJson for indexes with a single field (with needs to be an array)
    bool geoJson = basics::VelocyPackHelper::getBooleanValue(definition, "geoJson", false);

    builder.add("geoJson", VPackValue(geoJson));
  }
}

} // arangodb
