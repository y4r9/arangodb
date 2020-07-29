/*
  * General overview of this rule:
  * 1. Get all FilterNodes 
  * 2. For each get the creator of the referenced variable, only allowing CALCULATION
  * 3. Only continue if the condition is IN based, and deterministic
  * 4. Only continue if first member is REFERENCE or ARRAY
  * 5. Do not apply this rule, if the filter is not within a loop
  * 6. if Array.length >= SortNumberThreshold && !sorted() then array => SORTED_UNIQUE(array)
  * 7. if Reference:
  *    i. Only apply if Setter is Subquery or Calculation
  *    ii. Do not apply if Setter is in same Loop
  *    iii. if setter is a Calculation:
  *      a. if it is a function call, continue at b) with the first argument, abort if it has NoEval flag
  *      b. if it is an array with enough elements => SORTED_UNIQUE(array)
  *    iv. If setter is a Subquery
  *      a. If the estimatedNrItems is > SortNumberThreshold create a new Reference Node to subquery, apply SORTED_UNIQUE on the reference
  * 
  * 
  */

 /*
  * Pattern Alternative:
  * a) (LET x = <ARRAY> | <SUBQUERY> | <FUNC -> ARRAY>) ([^FOR]) (FOR) (*) (LET _ = _ IN x | _ NOT IN x)
  * b) ([^FOR]) (FOR) (*) (LET _ = _ IN <const array> | _ NOT IN <const array>)
  * 
  * Shall be replaced with:
  * a) (LET x = <ARRAY> | <SUBQUERY> | <FUNC -> ARRAY>) (LET tmp = SORTED_UNIQUE(x)) (*) (FOR) (*) (LET _ = _ IN tmp | _ NOT IN tmp)
  * b) (LET tmp = SORTED_UNIQUE(<const array>)) (FOR) (*) (LET _ = _ IN tmp| _ NOT IN tmp)
  * 
  * 
  * 
  * (Singleton) (*) (RETURN)
  */ 

#include "SortInValuesRule.h"
#include "Aql/ExecutionNode.h"

#include "mpark/patterns.hpp"

using namespace arangodb::aql;

#ifndef PATTERN_SEQ

#define PATTERN_SEQ(expr) \
  do {                    \
#expr;                 \
  } while (0);

#endif

#ifndef PATTERN_ONE

#define PATTERN_ONE(expr)                                             \
  while (0) {                                                         \
    std::unique_ptr<ExecutionNode> node =                             \
        std::make_unique<SingletonNode>(nullptr, ExecutionNodeId(1)); \
    (void)(match(*(node.get()))(#expr));                              \
  };

#endif

#ifndef PATTERN_ANY

#define PATTERN_ANY() \
  do {                    \
  } while (0);

#endif

#ifndef PATTERN_MANY

#define PATTERN_MANY(expr) \
  do {                    \
  expr; \
  } while (0);

#endif

namespace {

}

void arangodb::aql::sortInValuesRule_Pattern(Optimizer* opt, std::unique_ptr<ExecutionPlan> plan,
                                     OptimizerRule const& rule) {
  using namespace mpark::patterns;
  /*
  // Iterate over all branches, where the MainQuery is a branch
  // and all Subqueries each are a branch

// Matching for Alternative a)
  PATTERN_SEQ(
  (
    PATTERN_ONE((
      pattern(as<SubqueryNode>)) = [](SubqueryNode const& it) -> std::optional<SubqueryNode const> {
        // estimate items in subquery
        SubqueryNode producer = *it.value<SubqueryNode>();
        CostEstimate estimate = producer->getSubquery()->getCost();
        if (estimate.estimatedNrItems >= AstNode::SortNumberThreshold) {
          return producer;
        }
        return std::nullopt;
        
      },
      pattern(as<CalculationNode>)) = [](CalculationNode const& producer -> std::optional<ExecutionNode>) {
        AstNode const* testNode = producer.expression()->node();
        switch (testNode->type) {
          case NODE_TYPE_ARRAY:
            if (testNode->numMembers() >= AstNode::SortNumberThreshold) {
              return producer;
            }
            break;
          case NODE_TYPE_FCALL:
            // TODO, this is rather unclear.
            break;

        }
        // Unusable
        return std::nullopt;
      }
    ))

    PATTERN_MANY(PATTERN_ANY())

    // We need to have actual IN oepration in a separate loop, s.t. we would call it multiple times
    PATTERN_ONE((pattern(
      as<EnumerateCollectionNode>) = [](auto const& n) -> std::optional<ExecutionNode> {
    return n;},
      pattern(as<IndexNode>) = [](auto const& n) -> std::optional<ExecutionNode> {
    return n;},
      pattern(as<GraphNode>) = [](auto const& n) -> std::optional<ExecutionNode> {
    return n;},
      pattern(as<EnumerateCollectionNode>) = [](auto const& n) -> std::optional<ExecutionNode> {
    return n;},
      pattern(as<IResarchViewNode>) = [](auto const& n) -> std::optional<ExecutionNode> {
    return n;}
    )))

    PATTERN_MANY(PATTERN_ANY())

    PATTERN_ONE(pattern(as<CalculationNode>()) = [](CalculationNode const& consumer) -> std::optional<std::pair<CalculationNode&, Variable const&>> {
      AstNode const* testNode = producer.expression()->node();
      if (testNode->type == NODE_TYPE_OPERATOR_BINARY_IN || testNode->type == NODE_TYPE_OPERATOR_BINARY_NIN) {
        TRI_ASSERT(testeNode->numMembers(2));
        auto readingFrom = testNode->getMember(1);
        if (readingFrom->type == NODE_TYPE_REFERENCE) {
          auto var = static_cast<Variable const*>(readingFrom->getData());
          TRI_ASSERT(var != nullptr);
          return std::make_pair<consumer, *var>;
        }
      }
      return std::nullopt;
    })
  ), [](ExecutionNode const& producer, auto anyUnused, auto forUnused, auto any2Unused, std::pair<CalculationNode&, Variable const&> consumer) -> std::optional<bool> {
    if (producer.outVariable() != consumer.second) {
      // FILTER and CALCULATION are not aligned
      return std::nullopt;
    }
    return true;
  });
  */
}  