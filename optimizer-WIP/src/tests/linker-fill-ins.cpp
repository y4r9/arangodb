/**
 * @brief Filll in file to make linker happy
 * Do not include in release
 */

#include "Actions/actions.h"
#include "Basics/Exceptions.h"
#include "Cluster/AgencyCache.h"
#include "Cluster/ClusterHelpers.h"
#include "Cluster/ClusterInfo.h"
#include "Cluster/CriticalThread.h"
#include "Cluster/FollowerInfo.h"
#include "Cluster/ResignShardLeadership.h"
#include "RestHandler/RestBaseHandler.h"
#include "RestServer/AqlFeature.h"
#include "RestServer/DatabaseFeature.h"
#include "RestServer/FlushFeature.h"
#include "RestServer/MetricsFeature.h"
#include "RestServer/UpgradeFeature.h"
#include "Sharding/ShardingInfo.h"
#include "Statistics/ServerStatistics.h"
#include "Transaction/ClusterUtils.h"
#include "Transaction/Helpers.h"
#include "Transaction/Manager.h"
#include "Transaction/ManagerFeature.h"
#include "Transaction/Methods.h"
#include "Transaction/SmartContext.h"
#include "Transaction/StandaloneContext.h"
#include "Transaction/V8Context.h"
#include "V8/v8-conv.h"
#include "V8/v8-globals.h"
#include "V8/v8-utils.h"
#include "V8/v8-vpack.h"
#include "V8Server/v8-collection.h"
#include "VocBase/Methods/Collections.h"
#include "VocBase/Methods/Upgrade.h"
#include "RestServer/ServerIdFeature.h"
#include "V8Server/V8DealerFeature.h"
#include "V8Server/V8Context.h"
#include "Scheduler/SchedulerFeature.h"
#include "Cluster/ClusterTrxMethods.h"
#include "RestHandler/RestCursorHandler.h"
#include "RestServer/QueryRegistryFeature.h"
#include "RestServer/SystemDatabaseFeature.h"
#include "GeneralServer/AuthenticationFeature.h"
#include "V8/JavaScriptSecurityContext.h"
#include "Cluster/ClusterCollectionCreationInfo.h"
#include "GeneralServer/RestHandlerFactory.h"
#include "Utils/Events.h"
#include "Pregel/PregelFeature.h"
#include "Pregel/Conductor.h"
#include "Sharding/ShardingFeature.h"
#include "RestServer/DatabasePathFeature.h"


// Indirect imports
#include "Basics/StringBuffer.h"
#include "Sharding/ShardingStrategy.h"
#include "Aql/Query.h"
#include "Actions/ActionFeature.h"
#include "RestServer/ScriptFeature.h"
#include "RestServer/ServerFeature.h"
#include "RestServer/FrontendFeature.h"
#include "RestServer/BootstrapFeature.h"
#include "V8Server/FoxxQueuesFeature.h"
#include "Statistics/StatisticsWorker.h"

void TRI_VisitActions(std::function<void(TRI_action_t*)> const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

std::shared_ptr<TRI_action_t> TRI_DefineActionVocBase(std::string const&,
                                                      std::shared_ptr<TRI_action_t>) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

v8::Handle<v8::Value> TRI_VPackToV8(v8::Isolate* isolate, arangodb::velocypack::Slice const&,
                                    arangodb::velocypack::Options const* options,
                                    arangodb::velocypack::Slice const* base) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

int TRI_V8ToVPack(v8::Isolate* isolate, arangodb::velocypack::Builder& builder,
                  v8::Local<v8::Value> const value, bool keepTopLevelOpen,
                  bool convertFunctionsToNull) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

std::vector<std::shared_ptr<arangodb::LogicalCollection>> GetCollections(TRI_vocbase_t&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

bool EqualCollection(arangodb::CollectionNameResolver const* resolver,
                     std::string const& collectionName,
                     arangodb::LogicalCollection const* collection) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

arangodb::LogicalCollection* UnwrapCollection(v8::Isolate* isolate,
                                              v8::Local<v8::Object> const& holder) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

int64_t TRI_ObjectToInt64(v8::Isolate*, v8::Local<v8::Value>) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

double TRI_ObjectToDouble(v8::Isolate*, v8::Local<v8::Value>) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

std::string TRI_ObjectToString(v8::Isolate*, v8::Local<v8::Value>) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
bool TRI_ObjectToBoolean(v8::Isolate*, v8::Local<v8::Value>) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

bool TRI_AddMethodVocbase(v8::Isolate*, v8::Local<v8::ObjectTemplate>,
                          v8::Local<v8::String>,
                          void (*)(v8::FunctionCallbackInfo<v8::Value> const&), bool) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

void TRI_CreateErrorObject(v8::Isolate*, int) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

void TRI_CreateErrorObject(
    v8::Isolate*, int,
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    bool) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

void TRI_ClearObjectCacheV8(v8::Isolate*) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

bool TRI_RunGarbageCollectionV8(v8::Isolate*, double) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

bool TRI_AddGlobalFunctionVocbase(v8::Isolate*, v8::Local<v8::String>,
                                  void (*)(v8::FunctionCallbackInfo<v8::Value> const&), bool) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

auto TRI_FreeUserStructuresVocBase(TRI_vocbase_t*) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

auto TRI_CreateUserStructuresVocBase(TRI_vocbase_t*) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

auto TRI_ExpireFoxxQueueDatabaseCache(TRI_vocbase_t*) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

auto TRI_RemoveDatabaseTasksV8Dispatcher(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

TRI_Utf8ValueNFC::TRI_Utf8ValueNFC(v8::Isolate*, v8::Local<v8::Value>) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

TRI_Utf8ValueNFC::~TRI_Utf8ValueNFC() {}

arangodb::Result arangodb::ClusterInfo::agencyPlan(std::__1::shared_ptr<arangodb::velocypack::Builder>) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::Result arangodb::ClusterInfo::agencyReplan(arangodb::velocypack::Slice) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::shared_ptr<std::vector<arangodb::ShardID>> arangodb::ClusterInfo::getShardList(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::cluster::RebootTracker& arangodb::ClusterInfo::rebootTracker() noexcept {
  TRI_ASSERT(false);
  std::abort();
}

arangodb::Result arangodb::ClusterInfo::getShardServers(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    std::__1::vector<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>,
                     std::__1::allocator<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>>>&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::string arangodb::ClusterInfo::getServerEndpoint(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::Result arangodb::ClusterInfo::agencyHotBackupLock(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    double const&, bool&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::Result arangodb::ClusterInfo::dropViewCoordinator(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::vector<arangodb::ServerID> arangodb::ClusterInfo::getCurrentDBServers() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::Result arangodb::ClusterInfo::dropIndexCoordinator(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    arangodb::IndexId, double) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::AnalyzersRevision::Ptr arangodb::ClusterInfo::getAnalyzersRevision(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    bool) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::shared_ptr<std::vector<arangodb::ServerID>> arangodb::ClusterInfo::getResponsibleServer(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::ClusterInfo::loadCurrentDBServers() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::Result arangodb::ClusterInfo::agencyHotBackupUnlock(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    double const&, bool const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::Result arangodb::ClusterInfo::createViewCoordinator(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    arangodb::velocypack::Slice) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::shared_ptr<arangodb::LogicalDataSource> arangodb::ClusterInfo::getCollectionOrViewNT(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::unordered_map<arangodb::ShardID, arangodb::ServerID> arangodb::ClusterInfo::getResponsibleServers(
    std::__1::unordered_set<
        std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>,
        std::__1::hash<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>>,
        std::__1::equal_to<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>>,
        std::__1::allocator<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>>> const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::Result arangodb::ClusterInfo::ensureIndexCoordinator(
    arangodb::LogicalCollection const&, arangodb::velocypack::Slice const&,
    bool, arangodb::velocypack::Builder&, double) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::vector<arangodb::ServerID> arangodb::ClusterInfo::getCurrentCoordinators() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::Result arangodb::ClusterInfo::dropDatabaseCoordinator(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    double) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::string arangodb::ClusterInfo::getCollectionNotFoundMsg(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::Result arangodb::ClusterInfo::dropCollectionCoordinator(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    double) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::CollectionID arangodb::ClusterInfo::getCollectionNameForShard(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::QueryAnalyzerRevisions arangodb::ClusterInfo::getQueryAnalyzersRevision(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::AnalyzerModificationTransaction::Ptr arangodb::ClusterInfo::createAnalyzersCleanupTrans() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::Result arangodb::ClusterInfo::createCollectionsCoordinator(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    std::__1::vector<arangodb::ClusterCollectionCreationInfo, std::__1::allocator<arangodb::ClusterCollectionCreationInfo>>&,
    double, bool, std::__1::shared_ptr<arangodb::LogicalCollection> const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::Result arangodb::ClusterInfo::setViewPropertiesCoordinator(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    arangodb::velocypack::Slice const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::Result arangodb::ClusterInfo::setCollectionStatusCoordinator(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    TRI_vocbase_col_status_e) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::Result arangodb::ClusterInfo::cancelCreateDatabaseCoordinator(arangodb::CreateDatabaseInfo const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::Result arangodb::ClusterInfo::createFinalizeDatabaseCoordinator(arangodb::CreateDatabaseInfo const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::Result arangodb::ClusterInfo::setCollectionPropertiesCoordinator(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    arangodb::LogicalCollection const*) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::Result arangodb::ClusterInfo::createIsBuildingDatabaseCoordinator(
    arangodb::CreateDatabaseInfo const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
uint64_t arangodb::ClusterInfo::uniqid(unsigned long long) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::shared_ptr<arangodb::LogicalView> arangodb::ClusterInfo::getView(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::vector<std::shared_ptr<arangodb::LogicalView>> const arangodb::ClusterInfo::getViews(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::vector<arangodb::DatabaseID> arangodb::ClusterInfo::databases(bool) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

std::string arangodb::ServerState::roleToString(arangodb::ServerState::RoleEnum) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::string arangodb::ServerState::stateToString(arangodb::ServerState::StateEnum) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::string arangodb::ServerState::getPersistedId() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
bool arangodb::ServerState::hasPersistedId() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
bool arangodb::ServerState::writePersistedId(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::string arangodb::ServerState::generatePersistedId(arangodb::ServerState::RoleEnum const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::string arangodb::ServerState::roleToAgencyListKey(arangodb::ServerState::RoleEnum) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

arangodb::ServerState::Mode arangodb::ServerState::mode() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

void arangodb::ServerState::setRole(arangodb::ServerState::RoleEnum) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::ServerState::findHost(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::ServerState::StateEnum arangodb::ServerState::getState() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

arangodb::ServerState* arangodb::ServerState::instance() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
bool arangodb::ServerState::readOnly() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

void arangodb::transaction::CountCache::store(unsigned long long) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::transaction::CountCache::CountCache() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::transaction::SmartContext::~SmartContext() {}
arangodb::transaction::StringLeaser::StringLeaser(arangodb::transaction::Methods*) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::transaction::StringLeaser::~StringLeaser() {}

arangodb::transaction::BuilderLeaser::BuilderLeaser(arangodb::transaction::Methods*) {}
arangodb::transaction::BuilderLeaser::~BuilderLeaser() {}
std::unique_ptr<arangodb::transaction::Manager> arangodb::transaction::ManagerFeature::MANAGER =
    nullptr;

std::shared_ptr<arangodb::transaction::Context> arangodb::transaction::StandaloneContext::Create(TRI_vocbase_t&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::transaction::StandaloneContext::StandaloneContext(TRI_vocbase_t& v)
    : arangodb::transaction::SmartContext(v, arangodb::TransactionId{0}, nullptr) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::transaction::StringBufferLeaser::StringBufferLeaser(arangodb::transaction::Methods*) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::transaction::StringBufferLeaser::~StringBufferLeaser() {}
std::string* arangodb::transaction::Context::leaseString() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::transaction::Context::returnString(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>*) noexcept {
}

void arangodb::transaction::Context::returnBuilder(arangodb::velocypack::Builder*) noexcept {}

arangodb::TransactionId arangodb::transaction::Context::makeTransactionId() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

arangodb::velocypack::Options* arangodb::transaction::Context::getVPackOptionsForDump() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::unique_ptr<arangodb::velocypack::CustomTypeHandler> arangodb::transaction::Context::createCustomTypeHandler(
    TRI_vocbase_t&, arangodb::CollectionNameResolver const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::Result arangodb::transaction::Manager::abortManagedTrx(arangodb::TransactionId) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::shared_ptr<arangodb::transaction::Context> arangodb::transaction::Manager::leaseManagedTrx(
    arangodb::TransactionId, arangodb::AccessMode::Type) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::Result arangodb::transaction::Manager::createManagedTrx(
    TRI_vocbase_t&, arangodb::TransactionId,
    std::__1::vector<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>,
                     std::__1::allocator<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>>> const&,
    std::__1::vector<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>,
                     std::__1::allocator<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>>> const&,
    std::__1::vector<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>,
                     std::__1::allocator<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>>> const&,
    arangodb::transaction::Options, double) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::transaction::Manager::registerTransaction(arangodb::TransactionId, bool) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::Result arangodb::transaction::Manager::abortAllManagedWriteTrx(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    bool) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

uint64_t arangodb::transaction::Manager::getActiveTransactionCount() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::transaction::Manager::Manager(arangodb::transaction::ManagerFeature& f)
    : _feature(f) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

arangodb::futures::Future<arangodb::Result> arangodb::transaction::Methods::abortAsync() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

arangodb::futures::Future<arangodb::OperationResult> arangodb::transaction::Methods::countAsync(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    arangodb::transaction::CountType) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::futures::Future<arangodb::Result> arangodb::transaction::Methods::commitAsync() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::futures::Future<arangodb::Result> arangodb::transaction::Methods::finishAsync(
    arangodb::Result const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::futures::Future<arangodb::OperationResult> arangodb::transaction::Methods::insertAsync(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    arangodb::velocypack::Slice, arangodb::OperationOptions const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::futures::Future<arangodb::OperationResult> arangodb::transaction::Methods::removeAsync(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    arangodb::velocypack::Slice, arangodb::OperationOptions const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::futures::Future<arangodb::OperationResult> arangodb::transaction::Methods::updateAsync(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    arangodb::velocypack::Slice, arangodb::OperationOptions const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::futures::Future<arangodb::OperationResult> arangodb::transaction::Methods::replaceAsync(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    arangodb::velocypack::Slice, arangodb::OperationOptions const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::Result arangodb::transaction::Methods::addCollection(
    unsigned long long,
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    arangodb::AccessMode::Type) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::futures::Future<arangodb::OperationResult> arangodb::transaction::Methods::documentAsync(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    arangodb::velocypack::Slice, arangodb::OperationOptions&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::futures::Future<arangodb::OperationResult> arangodb::transaction::Methods::truncateAsync(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    arangodb::OperationOptions const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::string arangodb::transaction::Methods::extractIdString(arangodb::velocypack::Slice) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::Result arangodb::transaction::Methods::documentFastPath(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    arangodb::ManagedDocumentResult*, arangodb::velocypack::Slice,
    arangodb::velocypack::Builder&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::Result arangodb::transaction::Methods::documentFastPathLocal(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    arangodb::velocypack::StringRef const&, arangodb::ManagedDocumentResult&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::unique_ptr<arangodb::IndexIterator> arangodb::transaction::Methods::indexScanForCondition(
    std::__1::shared_ptr<arangodb::Index> const&, arangodb::aql::AstNode const*,
    arangodb::aql::Variable const*, arangodb::IndexIteratorOptions const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
TRI_voc_cid_t arangodb::transaction::Methods::addCollectionAtRuntime(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    arangodb::AccessMode::Type) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
TRI_voc_cid_t arangodb::transaction::Methods::addCollectionAtRuntime(
    unsigned long long,
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    arangodb::AccessMode::Type) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::futures::Future<arangodb::OperationResult> arangodb::transaction::Methods::countCoordinatorHelper(
    std::__1::shared_ptr<arangodb::LogicalCollection> const&,
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    arangodb::transaction::CountType) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
bool arangodb::transaction::Methods::addStatusChangeCallback(
    std::__1::function<void(arangodb::transaction::Methods&, arangodb::transaction::Status)> const*) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
bool arangodb::transaction::Methods::removeStatusChangeCallback(
    std::__1::function<void(arangodb::transaction::Methods&, arangodb::transaction::Status)> const*) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::transaction::Methods::addDataSourceRegistrationCallback(
    arangodb::Result (*const&)(arangodb::LogicalDataSource&, arangodb::transaction::Methods&)) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::OperationResult arangodb::transaction::Methods::all(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    unsigned long long, unsigned long long, arangodb::OperationOptions const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::OperationResult arangodb::transaction::Methods::any(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::Result arangodb::transaction::Methods::begin() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::unique_ptr<arangodb::IndexIterator> arangodb::transaction::Methods::indexScan(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    arangodb::transaction::Methods::CursorType) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::Result arangodb::transaction::Methods::resolveId(
    char const*, unsigned long, std::__1::shared_ptr<arangodb::LogicalCollection>&,
    char const*&, unsigned long&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::transaction::Methods::Methods(std::__1::shared_ptr<arangodb::transaction::Context> const&,
                                        arangodb::transaction::Options const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::transaction::Methods::Methods(
    std::__1::shared_ptr<arangodb::transaction::Context> const&,
    std::__1::vector<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>,
                     std::__1::allocator<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>>> const&,
    std::__1::vector<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>,
                     std::__1::allocator<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>>> const&,
    std::__1::vector<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>,
                     std::__1::allocator<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>>> const&,
    arangodb::transaction::Options const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

arangodb::transaction::Methods::~Methods() {}

void arangodb::transaction::Options::fromVelocyPack(arangodb::velocypack::Slice const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::transaction::Options arangodb::transaction::Options::replicationDefaults() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
uint64_t arangodb::transaction::Options::defaultMaxTransactionSize = 42;
uint64_t arangodb::transaction::Options::defaultIntermediateCommitSize = 42;
uint64_t arangodb::transaction::Options::defaultIntermediateCommitCount = 42;

void arangodb::transaction::Options::setLimits(unsigned long long, unsigned long long,
                                               unsigned long long) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::transaction::Options::Options() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

void arangodb::transaction::cluster::abortTransactions(arangodb::LogicalCollection&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::velocypack::StringRef arangodb::transaction::helpers::extractKeyPart(arangodb::velocypack::Slice) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::string arangodb::transaction::helpers::extractIdString(
    arangodb::CollectionNameResolver const*, arangodb::velocypack::Slice,
    arangodb::velocypack::Slice const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::OperationResult arangodb::transaction::helpers::buildCountResult(
    std::__1::vector<
        std::__1::pair<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>, unsigned long long>,
        std::__1::allocator<std::__1::pair<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>, unsigned long long>>> const&,
    arangodb::transaction::CountType, unsigned long long&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::velocypack::Slice arangodb::transaction::helpers::extractIdFromDocument(
    arangodb::velocypack::Slice) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::velocypack::Slice arangodb::transaction::helpers::extractToFromDocument(
    arangodb::velocypack::Slice) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::velocypack::Slice arangodb::transaction::helpers::extractKeyFromDocument(
    arangodb::velocypack::Slice) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
TRI_voc_rid_t arangodb::transaction::helpers::extractRevFromDocument(arangodb::velocypack::Slice) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

arangodb::velocypack::StringRef arangodb::transaction::helpers::extractCollectionFromId(
    arangodb::velocypack::StringRef) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::velocypack::Slice arangodb::transaction::helpers::extractFromFromDocument(
    arangodb::velocypack::Slice) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::velocypack::Slice arangodb::transaction::helpers::extractRevSliceFromDocument(
    arangodb::velocypack::Slice) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::transaction::helpers::extractKeyAndRevFromDocument(
    arangodb::velocypack::Slice, arangodb::velocypack::Slice&, unsigned long long&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::transaction::V8Context::exitV8Context() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::transaction::V8Context::enterV8Context(
    std::__1::shared_ptr<arangodb::TransactionState> const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::shared_ptr<arangodb::TransactionState> arangodb::transaction::V8Context::getParentState() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::shared_ptr<arangodb::transaction::Context> arangodb::transaction::V8Context::CreateWhenRequired(
    TRI_vocbase_t&, bool) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

TransactionStatistics::TransactionStatistics(arangodb::MetricsFeature& f)
    : _metrics(f),
      _transactionsStarted(_metrics.counter("arangodb_transactions_started", 0,
                                            "Transactions started")),
      _transactionsAborted(_metrics.counter("arangodb_transactions_aborted", 0,
                                            "Transactions aborted")),
      _transactionsCommitted(_metrics.counter("arangodb_transactions_committed",
                                              0, "Transactions committed")),
      _intermediateCommits(_metrics.counter("arangodb_intermediate_commits", 0,
                                            "Intermediate commits")) {}

bool arangodb::AqlFeature::lease() noexcept {
  TRI_ASSERT(false);
  return true;
}
void arangodb::AqlFeature::unlease() noexcept { TRI_ASSERT(false); }

arangodb::futures::Future<arangodb::Result> arangodb::AgencyCache::waitFor(unsigned long long) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

std::string const arangodb::maintenance::ResignShardLeadership::LeaderNotYetKnownString =
    "LEADER_NOT_YET_KNOWN";

arangodb::velocypack::CustomTypeHandler* arangodb::transaction::SmartContext::orderCustomTypeHandler() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::CollectionNameResolver const& arangodb::transaction::SmartContext::resolver() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::transaction::SmartContext::SmartContext(TRI_vocbase_t& v, arangodb::TransactionId,
                                                  std::__1::shared_ptr<arangodb::TransactionState>)
    : arangodb::transaction::Context(v) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

arangodb::velocypack::Options* arangodb::transaction::Context::getVPackOptions() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::transaction::Context::~Context() {}
arangodb::transaction::Manager::ManagedTrx::~ManagedTrx() {}

void arangodb::FlushFeature::registerFlushSubscription(
    std::__1::shared_ptr<arangodb::FlushSubscription> const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::Result arangodb::FollowerInfo::add(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::Result arangodb::FollowerInfo::remove(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::ShardingInfo::setShardMap(
    std::__1::shared_ptr<std::__1::unordered_map<
        std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>,
        std::__1::vector<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>,
                         std::__1::allocator<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>>>,
        std::__1::hash<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>>,
        std::__1::equal_to<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>>,
        std::__1::allocator<std::__1::pair<
            std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const,
            std::__1::vector<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>,
                             std::__1::allocator<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>>>>>>> const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::ShardingInfo::numberOfShards(unsigned long) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

int arangodb::ShardingInfo::getResponsibleShard(
    arangodb::velocypack::Slice, bool,
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>&,
    bool&, arangodb::velocypack::StringRef const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::ShardingInfo::distributeShardsLike(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    arangodb::ShardingInfo const*) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::ShardingInfo::sortShardNamesNumerically(
    std::__1::vector<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>,
                     std::__1::allocator<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>>>&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::Result arangodb::ShardingInfo::validateShardsAndReplicationFactor(
    arangodb::velocypack::Slice,
    arangodb::application_features::ApplicationServer const&, bool) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

void arangodb::ShardingInfo::setWriteConcernAndReplicationFactor(unsigned long, unsigned long) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::ShardingInfo::ShardingInfo(arangodb::velocypack::Slice,
                                     arangodb::LogicalCollection*) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::ShardingInfo::~ShardingInfo() {}

arangodb::AgencyCache& arangodb::ClusterFeature::agencyCache() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::ClusterInfo& arangodb::ClusterFeature::clusterInfo() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
bool arangodb::ClusterHelpers::compareServerLists(arangodb::velocypack::Slice,
                                                  arangodb::velocypack::Slice) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
bool arangodb::ClusterHelpers::compareServerLists(
    std::__1::vector<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>,
                     std::__1::allocator<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>>>,
    std::__1::vector<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>,
                     std::__1::allocator<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>>>) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

void arangodb::CriticalThread::crashNotification(std::exception const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

void arangodb::UpgradeFeature::addTask(arangodb::methods::Upgrade::Task&&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

int arangodb::DatabaseFeature::dropDatabase(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    bool) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::DatabaseFeature::recoveryDone() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::Result arangodb::DatabaseFeature::createDatabase(arangodb::CreateDatabaseInfo&&,
                                                           TRI_vocbase_t*&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::vector<std::string> arangodb::DatabaseFeature::getDatabaseNames() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::DatabaseFeature::enumerateDatabases(std::__1::function<void(TRI_vocbase_t&)> const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::vector<std::string> arangodb::DatabaseFeature::getDatabaseNamesForUser(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::Result arangodb::DatabaseFeature::registerPostRecoveryCallback(
    std::__1::function<arangodb::Result()>&&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::DatabaseFeature* arangodb::DatabaseFeature::DATABASE = nullptr;

void arangodb::DatabaseFeature::inventory(
    arangodb::velocypack::Builder&, unsigned long long,
    std::__1::function<bool(arangodb::LogicalCollection const*)> const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

void arangodb::RestBaseHandler::generateOk(arangodb::rest::ResponseCode,
                                           arangodb::velocypack::Slice) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::RestBaseHandler::generateOk(arangodb::rest::ResponseCode,
                                           arangodb::velocypack::Builder const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::RestBaseHandler::handleError(arangodb::basics::Exception const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

template<>
void arangodb::RestBaseHandler::generateResult<arangodb::velocypack::Slice>(
    arangodb::rest::ResponseCode, arangodb::velocypack::Slice&&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
template<>
void arangodb::RestBaseHandler::generateResult<arangodb::velocypack::Buffer<unsigned char>>(
    arangodb::rest::ResponseCode, arangodb::velocypack::Buffer<unsigned char>&&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

template<>
void arangodb::RestBaseHandler::generateResult<arangodb::velocypack::Buffer<unsigned char>>(
    arangodb::rest::ResponseCode, arangodb::velocypack::Buffer<unsigned char>&&,
    std::__1::shared_ptr<arangodb::transaction::Context>) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

arangodb::transaction::Context::Context(TRI_vocbase_t& v)
    : _vocbase(v),
      _resolver(nullptr),
      _customTypeHandler(),
      _builders{_arena},
      _stringBuffer(),
      _strings{_strArena},
      _options(arangodb::velocypack::Options::Defaults),
      _dumpOptions(arangodb::velocypack::Options::Defaults),
      _transaction{TransactionId::none(), false},
      _ownsResolver(false) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

arangodb::velocypack::Slice arangodb::RestBaseHandler::parseVPackBody(bool&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

arangodb::RestBaseHandler::RestBaseHandler(arangodb::application_features::ApplicationServer& server,
                                           arangodb::GeneralRequest* request,
                                           arangodb::GeneralResponse* response)
    : arangodb::rest::RestHandler(server, request, response) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::ServerId arangodb::ServerIdFeature::SERVERID = arangodb::ServerId(1);

void arangodb::V8DealerFeature::exitContext(arangodb::V8Context*) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::V8Context* arangodb::V8DealerFeature::enterContext(TRI_vocbase_t*,
                                             arangodb::JavaScriptSecurityContext const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
bool arangodb::V8DealerFeature::addGlobalContextMethod(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::V8DealerFeature* arangodb::V8DealerFeature::DEALER = nullptr;

arangodb::SupervisedScheduler* arangodb::SchedulerFeature::SCHEDULER = nullptr;

arangodb::Result arangodb::ViewTypesFeature::emplace(arangodb::LogicalDataSource::Type const&,
                                         arangodb::ViewFactory const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

arangodb::AnalyzersRevision::Ptr arangodb::AnalyzersRevision::getEmptyRevision() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

bool arangodb::ClusterTrxMethods::isElCheapo(arangodb::transaction::Methods const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

template<>
void arangodb::ClusterTrxMethods::addTransactionHeader<std::__1::map<
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>,
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>,
    std::__1::less<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>>,
    std::__1::allocator<std::__1::pair<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const,
                                       std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>>>>>(
    arangodb::transaction::Methods const&,
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    std::__1::map<
        std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>,
        std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>,
        std::__1::less<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>>,
        std::__1::allocator<std::__1::pair<
            std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const,
            std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>>>>&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

template<>
void arangodb::ClusterTrxMethods::addAQLTransactionHeader<std::__1::map<
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>,
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>,
    std::__1::less<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>>,
    std::__1::allocator<std::__1::pair<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const,
                                       std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>>>>>(
    arangodb::transaction::Methods const&,
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    std::__1::map<
        std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>,
        std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>,
        std::__1::less<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>>,
        std::__1::allocator<std::__1::pair<
            std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const,
            std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>>>>&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

arangodb::futures::Future<arangodb::Result> arangodb::ClusterTrxMethods::beginTransactionOnLeaders(
    arangodb::TransactionState&,
    std::__1::vector<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>,
                     std::__1::allocator<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>>> const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::RestCursorHandler::handleError(arangodb::basics::Exception const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::RestStatus arangodb::RestCursorHandler::continueExecute() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::RestCursorHandler::shutdownExecute(bool) noexcept {
  TRI_ASSERT(false);
}
arangodb::ResultT<std::pair<std::string, bool>> arangodb::RestCursorHandler::forwardingTarget() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::RestStatus arangodb::RestCursorHandler::handleQueryResult() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::RestStatus arangodb::RestCursorHandler::registerQueryOrCursor(arangodb::velocypack::Slice const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::RestCursorHandler::cancel() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::RestStatus arangodb::RestCursorHandler::execute() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::RestCursorHandler::RestCursorHandler(
    arangodb::application_features::ApplicationServer& server, arangodb::GeneralRequest* request,
    arangodb::GeneralResponse* response, arangodb::aql::QueryRegistry*) : arangodb::RestVocbaseBaseHandler(server, request, response) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::RestCursorHandler::~RestCursorHandler() {
}
arangodb::StatisticsFeature* arangodb::StatisticsFeature::STATISTICS = nullptr;

void arangodb::StatisticsFeature::toPrometheus(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>&,
    double const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
bool arangodb::SupervisedScheduler::queue(
    arangodb::RequestLane,
    fu2::abi_400::detail::function<fu2::abi_400::detail::config<true, false, fu2::capacity_default>,
                                   fu2::abi_400::detail::property<true, false, void()>>) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

std::atomic<arangodb::aql::QueryRegistry*> arangodb::QueryRegistryFeature::QUERY_REGISTRY = nullptr;

arangodb::AuthenticationFeature* arangodb::AuthenticationFeature::INSTANCE = nullptr;

void arangodb::SystemDatabaseFeature::VocbaseReleaser::operator()(TRI_vocbase_t*) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::string const& arangodb::SystemDatabaseFeature::name() noexcept {
 return arangodb::RestVocbaseBaseHandler::CURSOR_PATH;
}

arangodb::QueryAnalyzerRevisions arangodb::QueryAnalyzerRevisions::QUERY_LATEST(AnalyzersRevision::LATEST,
                                                            AnalyzersRevision::LATEST);

std::string const arangodb::RestVocbaseBaseHandler::CURSOR_PATH = "/_api/cursor";
std::string const arangodb::RestVocbaseBaseHandler::COLLECTION_PATH = "/_api/collection";

arangodb::ResultT<std::pair<std::string, bool>> arangodb::RestVocbaseBaseHandler::forwardingTarget() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::RestVocbaseBaseHandler::generateTransactionError(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    arangodb::OperationResult const&,
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    unsigned long long) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::RestVocbaseBaseHandler::RestVocbaseBaseHandler(
    arangodb::application_features::ApplicationServer& server,
    arangodb::GeneralRequest* req, arangodb::GeneralResponse* res)
    : arangodb::RestBaseHandler(server, req, res),
      _context(*static_cast<VocbaseContext*>(req->requestContext())),
      _vocbase(_context.vocbase()) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

arangodb::RestVocbaseBaseHandler::~RestVocbaseBaseHandler() {}

arangodb::JavaScriptSecurityContext arangodb::JavaScriptSecurityContext::createQueryContext() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::JavaScriptSecurityContext arangodb::JavaScriptSecurityContext::createInternalContext() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

arangodb::V8ConditionalContextGuard::V8ConditionalContextGuard(
    arangodb::Result&, v8::Isolate*& iso, TRI_vocbase_t*,
    arangodb::JavaScriptSecurityContext const&) : _isolate(iso), _context(nullptr), _active(true) {
  (void) _isolate;
  (void) _context;
  (void) _active;
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::V8ConditionalContextGuard::~V8ConditionalContextGuard() {
}

arangodb::ClusterCollectionCreationInfo::ClusterCollectionCreationInfo(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>,
    unsigned long long, unsigned long long, unsigned long long, bool,
    arangodb::velocypack::Slice const&,
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>,
    arangodb::RebootId) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}

arangodb::Result arangodb::AnalyzerModificationTransaction::start() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::Result arangodb::AnalyzerModificationTransaction::commit() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::AnalyzerModificationTransaction::AnalyzerModificationTransaction(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    arangodb::ClusterInfo*, bool) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::AnalyzerModificationTransaction::~AnalyzerModificationTransaction() {
}
arangodb::Result arangodb::auth::UserManager::updateUser(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    std::__1::function<arangodb::Result(arangodb::auth::User&)>&&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::Result arangodb::auth::UserManager::enumerateUsers(
    std::__1::function<bool(arangodb::auth::User&)>&&, bool) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::auth::Level arangodb::auth::UserManager::databaseAuthLevel(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    bool) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::auth::Level arangodb::auth::UserManager::collectionAuthLevel(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    bool) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::auth::UserManager::triggerCacheRevalidation() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::Result arangodb::auth::UserManager::storeUser(
    bool,
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    bool, arangodb::velocypack::Slice) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::auth::User::grantDatabase(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    arangodb::auth::Level) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
bool arangodb::auth::User::removeDatabase(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::auth::User::grantCollection(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    arangodb::auth::Level) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
bool arangodb::auth::User::removeCollection(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::rest::RestHandler::generateError(arangodb::rest::ResponseCode, int) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::rest::RestHandler::generateError(
    arangodb::rest::ResponseCode, int,
    std::__1::basic_string_view<char, std::__1::char_traits<char>>) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::rest::RestHandler::generateError(arangodb::Result const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::rest::RestHandler::resetResponse(arangodb::rest::ResponseCode) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
bool arangodb::rest::RestHandler::wakeupHandler() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::rest::RestHandler::handleExceptionPtr(std::exception_ptr) noexcept {
}
arangodb::rest::RestHandler::~RestHandler() {
}
void arangodb::rest::RestHandlerFactory::addPrefixHandler(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    std::__1::shared_ptr<arangodb::rest::RestHandler> (*)(
        arangodb::application_features::ApplicationServer&,
        arangodb::GeneralRequest*, arangodb::GeneralResponse*, void*),
    void*) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::events::CreateView(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    int) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::events::CreateIndex(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    arangodb::velocypack::Slice const&, int) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::events::DropDatabase(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    int) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::events::CreateDatabase(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    int) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::events::DropCollection(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    int) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::events::CreateHotbackup(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    int) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::events::DeleteHotbackup(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    int) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::events::CreateCollection(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    int) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::events::RestoreHotbackup(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    int) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::events::PropertyUpdateCollection(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    arangodb::velocypack::Slice const&) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::events::DropView(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    int) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::events::DropIndex(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    int) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::shared_ptr<arangodb::pregel::IWorker> arangodb::pregel::PregelFeature::worker(unsigned long long) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::shared_ptr<arangodb::pregel::PregelFeature> arangodb::pregel::PregelFeature::instance() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::shared_ptr<arangodb::pregel::Conductor> arangodb::pregel::PregelFeature::conductor(unsigned long long) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::pregel::Conductor::collectAQLResults(arangodb::velocypack::Builder&, bool) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::cluster::CallbackGuard arangodb::cluster::RebootTracker::callMeOnChange(
    arangodb::cluster::RebootTracker::PeerState const&, std::__1::function<void()>,
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::pair<bool, arangodb::Scheduler::WorkHandle> arangodb::Scheduler::queueDelay(
    arangodb::RequestLane,
    std::__1::chrono::duration<long long, std::__1::ratio<1l, 1000000000l>>,
    fu2::abi_400::detail::function<fu2::abi_400::detail::config<true, false, fu2::capacity_default>,
                                   fu2::abi_400::detail::property<true, false, void(bool)>>) {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::tuple<arangodb::consensus::query_t, arangodb::consensus::index_t> const arangodb::AgencyCache::read(
    std::__1::vector<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>,
                     std::__1::allocator<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>>> const&) const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::application_features::ApplicationServer& arangodb::ClusterInfo::server() const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::unordered_map<arangodb::ServerID, arangodb::RebootId> arangodb::ClusterInfo::rebootIds() const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
uint32_t arangodb::ServerState::getShortId() const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::RebootId arangodb::ServerState::getRebootId() const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::string arangodb::ServerState::getShortName() const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::string arangodb::ServerState::getId() const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
uint64_t arangodb::transaction::CountCache::get(double) const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
uint64_t arangodb::transaction::CountCache::get() const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::TransactionId arangodb::transaction::SmartContext::generateId() const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::TransactionId arangodb::transaction::Context::generateId() const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::shared_ptr<arangodb::transaction::Context> arangodb::transaction::Context::clone() const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::transaction::Status arangodb::transaction::Manager::getManagedTrxStatus(arangodb::TransactionId) const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::velocypack::Options const& arangodb::transaction::Methods::vpackOptions() const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::TransactionCollection* arangodb::transaction::Methods::trxCollection(unsigned long long,
                                                   arangodb::AccessMode::Type) const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::LogicalCollection* arangodb::transaction::Methods::documentCollection(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&) const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
bool arangodb::transaction::Methods::isSingleOperationTransaction() const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::transaction::Status arangodb::transaction::Methods::status() const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
TRI_vocbase_t& arangodb::transaction::Methods::vocbase() const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
bool arangodb::transaction::Methods::isLocked(arangodb::LogicalCollection*,
                                              arangodb::AccessMode::Type) const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::CollectionNameResolver const* arangodb::transaction::Methods::resolver() const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::transaction::Options::toVelocyPack(arangodb::velocypack::Builder&) const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
bool arangodb::ShardingInfo::isSatellite() const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::vector<std::string> const& arangodb::ShardingInfo::avoidServers() const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::ShardingInfo::toVelocyPack(arangodb::velocypack::Builder&, bool) const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
size_t arangodb::ShardingInfo::writeConcern() const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
size_t arangodb::ShardingInfo::numberOfShards() const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
size_t arangodb::ShardingInfo::replicationFactor() const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::shared_ptr<std::vector<arangodb::ShardID>> arangodb::ShardingInfo::shardListAsShardID() const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::string arangodb::ShardingInfo::distributeShardsLike() const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::string arangodb::ShardingInfo::shardingStrategyName() const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
bool arangodb::ShardingInfo::usesDefaultShardKeys() const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::shared_ptr<arangodb::ShardMap> arangodb::ShardingInfo::shardIds() const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::vector<std::string> const& arangodb::ShardingInfo::shardKeys() const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
TRI_vocbase_t* arangodb::DatabaseFeature::useDatabase(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&) const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
TRI_vocbase_t* arangodb::DatabaseFeature::useDatabase(unsigned long long) const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
TRI_vocbase_t* arangodb::DatabaseFeature::lookupDatabase(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&) const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::string arangodb::ShardingFeature::getDefaultShardingStrategyForNewCollection(
    arangodb::velocypack::Slice const&) const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::ViewFactory const& arangodb::ViewTypesFeature::factory(arangodb::LogicalDataSource::Type const&) const noexcept {
  TRI_ASSERT(false);
}
void arangodb::AnalyzersRevision::toVelocyPack(arangodb::velocypack::Builder&) const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
std::string arangodb::DatabasePathFeature::subdirectoryName(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&) const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::SystemDatabaseFeature::ptr arangodb::SystemDatabaseFeature::use() const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
void arangodb::QueryAnalyzerRevisions::toVelocyPack(arangodb::velocypack::Builder&) const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::AnalyzersRevision::Revision arangodb::QueryAnalyzerRevisions::getVocbaseRevision(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&) const noexcept {
  TRI_ASSERT(false);
}
std::unique_ptr<arangodb::transaction::Methods> arangodb::RestVocbaseBaseHandler::createTransaction(
    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&,
    arangodb::AccessMode::Type) const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::AnalyzersRevision::Revision arangodb::AnalyzerModificationTransaction::buildingRevision() const noexcept {
  TRI_ASSERT(false);
}

void arangodb::RequestStatistics::release() {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}
arangodb::rest::RestHandler::RestHandler(arangodb::application_features::ApplicationServer& server, arangodb::GeneralRequest*, arangodb::GeneralResponse*) : _server(server) {
}

arangodb::ClusterCollectionCreationInfo::CreatorInfo::CreatorInfo(std::string coordinatorId, RebootId rebootId) : _coordinatorId(coordinatorId), _rebootId(rebootId){}

void arangodb::ClusterCollectionCreationInfo::CreatorInfo::toVelocyPack(arangodb::velocypack::Builder& builder) const {
  THROW_ARANGO_EXCEPTION(TRI_ERROR_NOT_IMPLEMENTED);
}


arangodb::AgencyCallbackRegistry::~AgencyCallbackRegistry() {}
arangodb::auth::TokenCache::~TokenCache() {}



// AqlFeature
arangodb::AqlFeature::~AqlFeature() {}
void arangodb::AqlFeature::start() {}

void arangodb::AqlFeature::stop() {}

// Manager Feature
void arangodb::transaction::ManagerFeature::collectOptions(std::shared_ptr<options::ProgramOptions>) {
}
void arangodb::transaction::ManagerFeature::prepare() {}
void arangodb::transaction::ManagerFeature::start() {}
void arangodb::transaction::ManagerFeature::beginShutdown() {}
void arangodb::transaction::ManagerFeature::stop() {}
void arangodb::transaction::ManagerFeature::unprepare() {}

// Flush Feature
void arangodb::FlushFeature::collectOptions(
    std::shared_ptr<options::ProgramOptions>) {}
void arangodb::FlushFeature::validateOptions(
    std::shared_ptr<options::ProgramOptions>) {}
void arangodb::FlushFeature::prepare() {}
void arangodb::FlushFeature::start() {}
void arangodb::FlushFeature::beginShutdown() {}
void arangodb::FlushFeature::stop() {}


// Action Feature
void arangodb::ActionFeature::collectOptions(std::shared_ptr<options::ProgramOptions>) {
}
void arangodb::ActionFeature::start() {}
void arangodb::ActionFeature::unprepare() {}

// Script Feature
void arangodb::ScriptFeature::collectOptions(std::shared_ptr<options::ProgramOptions>) {
}
void arangodb::ScriptFeature::start() {}

// Server Feature

void arangodb::ServerFeature::collectOptions(
    std::shared_ptr<options::ProgramOptions>) {}
void arangodb::ServerFeature::validateOptions(
    std::shared_ptr<options::ProgramOptions>) {}
void arangodb::ServerFeature::prepare() {}
void arangodb::ServerFeature::start() {}
void arangodb::ServerFeature::beginShutdown() {}
void arangodb::ServerFeature::stop() {}

// Cluster Feature
arangodb::ClusterFeature::~ClusterFeature() {}

void arangodb::ClusterFeature::collectOptions(
    std::shared_ptr<options::ProgramOptions>) {}
void arangodb::ClusterFeature::validateOptions(
    std::shared_ptr<options::ProgramOptions>) {}
void arangodb::ClusterFeature::prepare() {}
void arangodb::ClusterFeature::start() {}
void arangodb::ClusterFeature::beginShutdown() {}
void arangodb::ClusterFeature::stop() {}
void arangodb::ClusterFeature::unprepare() {}


void arangodb::UpgradeFeature::collectOptions(
    std::shared_ptr<options::ProgramOptions>) {}
void arangodb::UpgradeFeature::validateOptions(
    std::shared_ptr<options::ProgramOptions>) {}
void arangodb::UpgradeFeature::prepare() {}
void arangodb::UpgradeFeature::start() {}

arangodb::DatabaseFeature::~DatabaseFeature() {}

void arangodb::DatabaseFeature::collectOptions(
    std::shared_ptr<options::ProgramOptions>) {}
void arangodb::DatabaseFeature::validateOptions(
    std::shared_ptr<options::ProgramOptions>) {}
void arangodb::DatabaseFeature::start() {}
void arangodb::DatabaseFeature::beginShutdown() {}
void arangodb::DatabaseFeature::stop() {}
void arangodb::DatabaseFeature::unprepare() {}

void arangodb::FrontendFeature::collectOptions(
    std::shared_ptr<options::ProgramOptions>) {}
    void arangodb::FrontendFeature::prepare() {}

void arangodb::ServerIdFeature::start() {}

void arangodb::ShardingFeature::prepare() {}
void arangodb::ShardingFeature::start() {}


void arangodb::V8DealerFeature::collectOptions(
    std::shared_ptr<options::ProgramOptions>) {}
void arangodb::V8DealerFeature::validateOptions(
    std::shared_ptr<options::ProgramOptions>) {}
void arangodb::V8DealerFeature::prepare() {}
void arangodb::V8DealerFeature::start() {}
void arangodb::V8DealerFeature::unprepare() {}

void arangodb::BootstrapFeature::collectOptions(
    std::shared_ptr<options::ProgramOptions>) {}
void arangodb::BootstrapFeature::start() {}
void arangodb::BootstrapFeature::unprepare() {}

arangodb::SchedulerFeature::~SchedulerFeature() {}
void arangodb::SchedulerFeature::collectOptions(
    std::shared_ptr<options::ProgramOptions>) {}
void arangodb::SchedulerFeature::validateOptions(
    std::shared_ptr<options::ProgramOptions>) {}
void arangodb::SchedulerFeature::prepare() {}
void arangodb::SchedulerFeature::start() {}
void arangodb::SchedulerFeature::stop() {}
void arangodb::SchedulerFeature::unprepare() {}


void arangodb::ViewTypesFeature::prepare() {}
void arangodb::ViewTypesFeature::unprepare() {}

void arangodb::FoxxQueuesFeature::collectOptions(
    std::shared_ptr<options::ProgramOptions>) {}
void arangodb::FoxxQueuesFeature::validateOptions(
    std::shared_ptr<options::ProgramOptions>) {}


arangodb::StatisticsFeature::~StatisticsFeature(){}

void arangodb::StatisticsFeature::collectOptions(
    std::shared_ptr<options::ProgramOptions>) {}
void arangodb::StatisticsFeature::validateOptions(
    std::shared_ptr<options::ProgramOptions>) {}
void arangodb::StatisticsFeature::prepare() {}
void arangodb::StatisticsFeature::start() {}
void arangodb::StatisticsFeature::stop() {}

void arangodb::DatabasePathFeature::collectOptions(
    std::shared_ptr<options::ProgramOptions>) {}
void arangodb::DatabasePathFeature::validateOptions(
    std::shared_ptr<options::ProgramOptions>) {}
void arangodb::DatabasePathFeature::prepare() {}
void arangodb::DatabasePathFeature::start() {}

void arangodb::QueryRegistryFeature::collectOptions(
    std::shared_ptr<options::ProgramOptions>) {}
void arangodb::QueryRegistryFeature::validateOptions(
    std::shared_ptr<options::ProgramOptions>) {}
void arangodb::QueryRegistryFeature::prepare() {}
void arangodb::QueryRegistryFeature::start() {}
void arangodb::QueryRegistryFeature::stop() {}
void arangodb::QueryRegistryFeature::unprepare() {}
void arangodb::QueryRegistryFeature::beginShutdown() {};

void arangodb::AuthenticationFeature::collectOptions(
    std::shared_ptr<options::ProgramOptions>) {}
void arangodb::AuthenticationFeature::validateOptions(
    std::shared_ptr<options::ProgramOptions>) {}
void arangodb::AuthenticationFeature::prepare() {}
void arangodb::AuthenticationFeature::start() {}
void arangodb::AuthenticationFeature::unprepare() {}

void arangodb::SystemDatabaseFeature::start() {}
void arangodb::SystemDatabaseFeature::unprepare() {}

// Printers
std::ostream& operator<<(std::ostream& o, 
  arangodb::transaction::Status const& r) {
  return o;
}

std::ostream& operator<<(std::ostream& o, 
  arangodb::QueryAnalyzerRevisions const& r) {
  return o;
}