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
/// @author Jan Steemann
////////////////////////////////////////////////////////////////////////////////

#include "RocksDBOptimizerRules.h"

#include "Aql/ClusterNodes.h"
#include "Aql/Collection.h"
#include "Aql/Condition.h"
#include "Aql/ExecutionNode.h"
#include "Aql/ExecutionPlan.h"
#include "Aql/Expression.h"
#include "Aql/Function.h"
#include "Aql/IndexHint.h"
#include "Aql/IndexNode.h"
#include "Aql/ModificationNodes.h"
#include "Aql/Optimizer.h"
#include "Aql/OptimizerRule.h"
#include "Aql/OptimizerRulesFeature.h"
#include "Aql/Query.h"
#include "Aql/SortNode.h"
#include "Aql/TraversalNode.h"
#include "Basics/StaticStrings.h"
#include "Containers/HashSet.h"
#include "Graph/BaseOptions.h"
#include "Indexes/Index.h"
#include "VocBase/LogicalCollection.h"

using namespace arangodb;
using namespace arangodb::aql;
using EN = arangodb::aql::ExecutionNode;

namespace {

std::vector<ExecutionNode::NodeType> const reduceExtractionToProjectionTypes = {
    ExecutionNode::ENUMERATE_COLLECTION, ExecutionNode::INDEX};

}  // namespace

void RocksDBOptimizerRules::registerResources(OptimizerRulesFeature& feature) {
  // simplify an EnumerationCollectionNode that fetches an entire document to a
  // projection of this document
  feature.registerRule("reduce-extraction-to-projection",
                       reduceExtractionToProjectionRule,
                       OptimizerRule::reduceExtractionToProjectionRule,
                       OptimizerRule::makeFlags(OptimizerRule::Flags::CanBeDisabled));
  
  // optimize traversals so that they also restrict vertex and edge fetching to
  // the actually required document attributes
  feature.registerRule("reduce-traversal-extraction-to-projection",
                       reduceTraversalExtractionToProjectionRule,
                       OptimizerRule::reduceTraversalExtractionToProjectionRule,
                       OptimizerRule::makeFlags(OptimizerRule::Flags::CanBeDisabled));

  // remove SORT RAND() LIMIT 1 if appropriate
  feature.registerRule("remove-sort-rand-limit-1", removeSortRandRule,
                       OptimizerRule::removeSortRandRule,
                       OptimizerRule::makeFlags(OptimizerRule::Flags::CanBeDisabled));
}

// simplify an EnumerationCollectionNode that fetches an entire document to a
// projection of this document
void RocksDBOptimizerRules::reduceExtractionToProjectionRule(
    Optimizer* opt, std::unique_ptr<ExecutionPlan> plan, OptimizerRule const& rule) {
  // These are all the nodes where we start traversing (including all
  // subqueries)
  ::arangodb::containers::SmallVector<ExecutionNode*>::allocator_type::arena_type a;
  ::arangodb::containers::SmallVector<ExecutionNode*> nodes{a};

  plan->findNodesOfType(nodes, ::reduceExtractionToProjectionTypes, true);

  bool modified = false;
  ::arangodb::containers::HashSet<Variable const*> vars;
  std::unordered_set<std::string> attributes;

  for (auto& n : nodes) {
    DocumentProducingNode* e = dynamic_cast<DocumentProducingNode*>(n);
    if (e == nullptr) {
      THROW_ARANGO_EXCEPTION_MESSAGE(
          TRI_ERROR_INTERNAL, "cannot convert node to DocumentProducingNode");
    }

    Variable const* v = e->outVariable();

    attributes.clear();
    bool stop = false;
    ExecutionNode* current = n->getFirstParent();
    while (current != nullptr) {
      if (!current->getReferencedAttributes(v, attributes)) {
        stop = true;
        break;
      }

      current = current->getFirstParent();
    }

    // projections are currently limited (arbitrarily to 5 attributes)
    if (!stop && !attributes.empty() && attributes.size() <= 5) {
      if (n->getType() == ExecutionNode::ENUMERATE_COLLECTION &&
          std::find(attributes.begin(), attributes.end(), StaticStrings::IdString) ==
              attributes.end()) {
        // the node is still an EnumerateCollection... now check if we should
        // turn it into an index scan we must never have a projection on _id, as
        // producing _id is not supported yet by the primary index iterator
        EnumerateCollectionNode const* en =
            ExecutionNode::castTo<EnumerateCollectionNode const*>(n);
        auto const& hint = en->hint();

        // now check all indexes if they cover the projection
        auto trx = plan->getAst()->query()->trx();
        std::shared_ptr<Index> picked;
        std::vector<std::shared_ptr<Index>> indexes;
        if (!trx->isInaccessibleCollection(en->collection()->getCollection()->name())) {
          indexes = en->collection()->getCollection()->getIndexes();
        }

        auto selectIndexIfPossible =
            [&picked, &attributes](std::shared_ptr<Index> const& idx) -> bool {
          if (!idx->hasCoveringIterator() || !idx->covers(attributes)) {
            // index doesn't cover the projection
            return false;
          }
          if (idx->type() != arangodb::Index::IndexType::TRI_IDX_TYPE_PRIMARY_INDEX &&
              idx->type() != arangodb::Index::IndexType::TRI_IDX_TYPE_HASH_INDEX &&
              idx->type() != arangodb::Index::IndexType::TRI_IDX_TYPE_SKIPLIST_INDEX &&
              idx->type() != arangodb::Index::IndexType::TRI_IDX_TYPE_PERSISTENT_INDEX) {
            // only the above index types are supported
            return false;
          }

          picked = idx;
          return true;
        };

        bool forced = false;
        if (hint.type() == aql::IndexHint::HintType::Simple) {
          forced = hint.isForced();
          for (std::string const& hinted : hint.hint()) {
            auto idx = en->collection()->getCollection()->lookupIndex(hinted);
            if (idx && selectIndexIfPossible(idx)) {
              break;
            }
          }
        }

        if (!picked && !forced) {
          for (auto const& idx : indexes) {
            if (selectIndexIfPossible(idx)) {
              break;
            }
          }
        }

        if (picked != nullptr) {
          // turn the EnumerateCollection node into an IndexNode now
          auto condition = std::make_unique<Condition>(plan->getAst());
          condition->normalize(plan.get());
          IndexIteratorOptions opts;
          // we have already proven that we can use the covering index optimization,
          // so force it - if we wouldn't force it here it would mean that for a
          // FILTER-less query we would be a lot less efficient for some indexes
          opts.forceProjection = true;
          auto inode = new IndexNode(plan.get(), plan->nextId(),
                                     en->collection(), en->outVariable(),
                                     std::vector<transaction::Methods::IndexHandle>{picked},
                                     std::move(condition), opts);
          en->CollectionAccessingNode::cloneInto(*inode);
          en->DocumentProducingNode::cloneInto(plan.get(), *inode);
          plan->registerNode(inode);
          plan->replaceNode(n, inode);
          if (en->isRestricted()) {
            inode->restrictToShard(en->restrictedShard());
          }
          // copy over specialization data from smart-joins rule
          inode->setPrototype(en->prototypeCollection(), en->prototypeOutVariable());
          n = inode;
          // need to update e, because it is used later
          e = dynamic_cast<DocumentProducingNode*>(n);
          if (e == nullptr) {
            THROW_ARANGO_EXCEPTION_MESSAGE(
                TRI_ERROR_INTERNAL,
                "cannot convert node to DocumentProducingNode");
          }
        }
      }

      // store projections in DocumentProducingNode
      e->projections(std::move(attributes));

      if (n->getType() == ExecutionNode::INDEX) {
        // need to update _indexCoversProjections value in an IndexNode
        ExecutionNode::castTo<IndexNode*>(n)->initIndexCoversProjections();
      }

      modified = true;
    } else if (!stop && attributes.empty() && n->getType() == ExecutionNode::ENUMERATE_COLLECTION) {
      // replace collection access with primary index access (which can be
      // faster given the fact that keys and values are stored together in
      // RocksDB, but average values are much bigger in the documents column
      // family than in the primary index colum family. thus in disk-bound
      // workloads scanning the documents via the primary index should be faster
      EnumerateCollectionNode* en = ExecutionNode::castTo<EnumerateCollectionNode*>(n);
      auto const& hint = en->hint();

      auto trx = plan->getAst()->query()->trx();
      std::shared_ptr<Index> picked;
      std::vector<std::shared_ptr<Index>> indexes;
      if (!trx->isInaccessibleCollection(en->collection()->getCollection()->name())) {
        indexes = en->collection()->getCollection()->getIndexes();
      }

      auto selectIndexIfPossible = [&picked](std::shared_ptr<Index> const& idx) -> bool {
        if (idx->type() == arangodb::Index::IndexType::TRI_IDX_TYPE_PRIMARY_INDEX) {
          picked = idx;
          return true;
        }
        return false;
      };

      bool forced = false;
      if (hint.type() == aql::IndexHint::HintType::Simple) {
        forced = hint.isForced();
        for (std::string const& hinted : hint.hint()) {
          auto idx = en->collection()->getCollection()->lookupIndex(hinted);
          if (idx && selectIndexIfPossible(idx)) {
            break;
          }
        }
      }

      if (!picked && !forced) {
        for (auto const& idx : indexes) {
          if (selectIndexIfPossible(idx)) {
            break;
          }
        }
      }

      if (picked != nullptr) {
        IndexIteratorOptions opts;
        auto condition = std::make_unique<Condition>(plan->getAst());
        condition->normalize(plan.get());
        auto inode = new IndexNode(plan.get(), plan->nextId(), en->collection(),
                                   en->outVariable(),
                                   std::vector<transaction::Methods::IndexHandle>{picked},
                                   std::move(condition), opts);
        plan->registerNode(inode);
        plan->replaceNode(n, inode);
        en->CollectionAccessingNode::cloneInto(*inode);
        en->DocumentProducingNode::cloneInto(plan.get(), *inode);
        modified = true;
      }
    }
  }

  opt->addPlan(std::move(plan), rule, modified);
}

// Optimize traversals so that they also restrict vertex and edge fetching to
// the actually required document attributes
void RocksDBOptimizerRules::reduceTraversalExtractionToProjectionRule(
    Optimizer* opt, std::unique_ptr<ExecutionPlan> plan, OptimizerRule const& rule) {
  // These are all the nodes where we start traversing (including all
  // subqueries)
  ::arangodb::containers::SmallVector<ExecutionNode*>::allocator_type::arena_type a;
  ::arangodb::containers::SmallVector<ExecutionNode*> nodes{a};
  
  plan->findNodesOfType(nodes, EN::TRAVERSAL, true);

  bool modified = false;

  auto checkVariableUsage = [](ExecutionNode const* current, Variable const* v, 
                               std::unordered_set<std::string>& attributes) {
    attributes.clear();

    while (current != nullptr) {
      if (!current->getReferencedAttributes(v, attributes)) {
        return false;
      }

      current = current->getFirstParent();
    }

    return true;
  };

  std::unordered_set<std::string> attributes;

  for (auto& n : nodes) {
    auto t = ExecutionNode::castTo<TraversalNode*>(n);

    attributes.clear();
    
    bool usesPathVertices = false; 
    bool usesPathEdges = false; 
    
    Variable const* v = t->pathOutVariable();
    if (v != nullptr) {
      // check if path.vertices and path.edges are used
      usesPathVertices = true; 
      usesPathEdges = true; 
      if (checkVariableUsage(n, v, attributes)) {
        usesPathVertices = (attributes.find("vertices") != attributes.end());
        usesPathEdges = (attributes.find("edges") != attributes.end());
      }
    
      if (t->condition() != nullptr) {
        attributes.clear();
        if (Ast::getReferencedAttributes(t->condition()->root(), v, attributes)) {
          usesPathVertices |= (attributes.find("vertices") != attributes.end());
          usesPathEdges |= (attributes.find("edges") != attributes.end());
        } else {
          // we are unsure
          usesPathVertices = true;
          usesPathEdges = true;
        }
      }
    }

    // check traversal's vertex output variable
    v = t->vertexOutVariable();
    if (v != nullptr &&
        !usesPathVertices &&
        checkVariableUsage(n->getFirstParent(), v, attributes)) {
      t->options()->setVertexProjections(std::move(attributes));
      modified = true;
    }
    
    // check traversal's edge output variable
    v = t->edgeOutVariable();
    if (v != nullptr && 
        !usesPathEdges &&
        checkVariableUsage(n->getFirstParent(), v, attributes)) {
      t->options()->setEdgeProjections(std::move(attributes));
      modified = true;
    }
  }

  opt->addPlan(std::move(plan), rule, modified);
}

/// @brief remove SORT RAND() if appropriate
void RocksDBOptimizerRules::removeSortRandRule(Optimizer* opt,
                                               std::unique_ptr<ExecutionPlan> plan,
                                               OptimizerRule const& rule) {
  ::arangodb::containers::SmallVector<ExecutionNode*>::allocator_type::arena_type a;
  ::arangodb::containers::SmallVector<ExecutionNode*> nodes{a};
  plan->findNodesOfType(nodes, EN::SORT, true);

  bool modified = false;

  for (auto const& n : nodes) {
    auto node = ExecutionNode::castTo<SortNode*>(n);
    auto const& elements = node->elements();
    if (elements.size() != 1) {
      // we're looking for "SORT RAND()", which has just one sort criterion
      continue;
    }

    auto const variable = elements[0].var;
    TRI_ASSERT(variable != nullptr);

    auto setter = plan->getVarSetBy(variable->id);

    if (setter == nullptr || setter->getType() != EN::CALCULATION) {
      continue;
    }

    auto cn = ExecutionNode::castTo<CalculationNode*>(setter);
    auto const expression = cn->expression();

    if (expression == nullptr || expression->node() == nullptr ||
        expression->node()->type != NODE_TYPE_FCALL) {
      // not the right type of node
      continue;
    }

    auto funcNode = expression->node();
    auto func = static_cast<Function const*>(funcNode->getData());

    // we're looking for "RAND()", which is a function call
    // with an empty parameters array
    if (func->name != "RAND" || funcNode->numMembers() != 1 ||
        funcNode->getMember(0)->numMembers() != 0) {
      continue;
    }

    // now we're sure we got SORT RAND() !

    // we found what we were looking for!
    // now check if the dependencies qualify
    if (!n->hasDependency()) {
      break;
    }

    auto current = n->getFirstDependency();
    ExecutionNode* collectionNode = nullptr;

    while (current != nullptr) {
      switch (current->getType()) {
        case EN::SORT:
        case EN::COLLECT:
        case EN::FILTER:
        case EN::SUBQUERY:
        case EN::ENUMERATE_LIST:
        case EN::TRAVERSAL:
        case EN::SHORTEST_PATH:
        case EN::INDEX:
        case EN::ENUMERATE_IRESEARCH_VIEW: {
          // if we found another SortNode, a CollectNode, FilterNode, a
          // SubqueryNode, an EnumerateListNode, a TraversalNode or an IndexNode
          // this means we cannot apply our optimization
          collectionNode = nullptr;
          current = nullptr;
          continue;  // this will exit the while loop
        }

        case EN::ENUMERATE_COLLECTION: {
          if (collectionNode == nullptr) {
            // note this node
            collectionNode = current;
            break;
          } else {
            // we already found another collection node before. this means we
            // should not apply our optimization
            collectionNode = nullptr;
            current = nullptr;
            continue;  // this will exit the while loop
          }
          // cannot get here
          TRI_ASSERT(false);
        }

        default: {
          // ignore all other nodes
        }
      }

      if (!current->hasDependency()) {
        break;
      }

      current = current->getFirstDependency();
    }

    if (collectionNode == nullptr || !node->hasParent()) {
      continue;  // skip
    }

    current = node->getFirstParent();
    bool valid = false;
    if (current->getType() == EN::LIMIT) {
      LimitNode* ln = ExecutionNode::castTo<LimitNode*>(current);
      if (ln->limit() == 1 && ln->offset() == 0) {
        valid = true;
      }
    }

    if (valid) {
      // we found a node to modify!
      TRI_ASSERT(collectionNode->getType() == EN::ENUMERATE_COLLECTION);
      // set the random iteration flag for the EnumerateCollectionNode
      ExecutionNode::castTo<EnumerateCollectionNode*>(collectionNode)->setRandom();

      // remove the SortNode and the CalculationNode
      plan->unlinkNode(n);
      plan->unlinkNode(setter);
      modified = true;
    }
  }

  opt->addPlan(std::move(plan), rule, modified);
}
