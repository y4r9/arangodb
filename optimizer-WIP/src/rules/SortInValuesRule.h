#ifndef ARANGODB_OPTIMIZER_RULES_SORT_IN_VALUES_H
#define ARANGODB_OPTIMIZER_RULES_SORT_IN_VALUES_H 1


#include <memory>


namespace arangodb {
namespace aql {
class Optimizer;
class ExecutionPlan;
class OptimizerRule;


static void sortInValuesRule_Pattern(Optimizer* opt, std::unique_ptr<ExecutionPlan> plan,
                              OptimizerRule const& rule);
}
}  // namespace arangodb


#endif