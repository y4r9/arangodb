////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2014-2018 ArangoDB GmbH, Cologne, Germany
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
/// @author Dr. Frank Celler
/// @author Achim Brandt
////////////////////////////////////////////////////////////////////////////////

#include "LaneScheduler.h"

#include <velocypack/Builder.h>
#include <velocypack/velocypack-aliases.h>

#include <thread>
#include <unordered_set>

#include "Basics/MutexLocker.h"
#include "Basics/ScopeGuard.h"
#include "Basics/StringUtils.h"
#include "Basics/Thread.h"
#include "GeneralServer/RestHandler.h"
#include "Logger/Logger.h"
#include "Random/RandomGenerator.h"
#include "Rest/GeneralResponse.h"
#include "Scheduler/JobGuard.h"
#include "Statistics/RequestStatistics.h"
#include "Utils/ExecContext.h"

using namespace arangodb;
using namespace arangodb::basics;
using namespace arangodb::rest;

std::unordered_set<void*> gActiveStrandMap;
Mutex gActiveStrandMapMutex;

namespace {
// controls how fast excess threads to io_context get pruned.
//  60 known to slow down tests that use single client thread (matthewv)
constexpr double MIN_SECONDS = 30.0;
}  // namespace

// -----------------------------------------------------------------------------
// --SECTION--                                            SchedulerManagerThread
// -----------------------------------------------------------------------------

namespace {
class SchedulerManagerThread final : public Thread {
 public:
  explicit SchedulerManagerThread(LaneScheduler* scheduler)
      : Thread("SchedulerManager", true), _scheduler(scheduler) {}
  ~SchedulerManagerThread() { shutdown(); }

  void run() override { _scheduler->runSupervisor(); };

 private:
  LaneScheduler* _scheduler;
};
}  // namespace

// -----------------------------------------------------------------------------
// --SECTION--                                                   SchedulerThread
// -----------------------------------------------------------------------------

class arangodb::SchedulerThread : public Thread {
 public:
  SchedulerThread(LaneScheduler* scheduler)
      : Thread("Scheduler", true), _scheduler(scheduler) {}
  ~SchedulerThread() { shutdown(); }

  void run() override { _scheduler->runWorker(); };

 private:
  LaneScheduler* _scheduler;
};

// -----------------------------------------------------------------------------
// --SECTION--                                                     LaneScheduler
// -----------------------------------------------------------------------------

LaneScheduler::LaneScheduler(uint64_t nrMinimum, uint64_t nrMaximum,
                             uint64_t fifo1Size, uint64_t fifo2Size)
    : _counters(0),
      _maxFifoSize{fifo1Size, fifo2Size, fifo2Size},
      _fifo1(_maxFifoSize[FIFO1]),
      _fifo2(_maxFifoSize[FIFO2]),
      _fifo3(_maxFifoSize[FIFO3]),
      _fifos{&_fifo1, &_fifo2, &_fifo3},
      _minThreads(nrMinimum),
      _maxThreads(nrMaximum),
      _lastAllBusyStamp(0.0) {
  LOG_TOPIC("91231", DEBUG, Logger::THREADS)
      << "Scheduler configuration min: " << nrMinimum << " max: " << nrMaximum;
  _fifoSize[FIFO1] = 0;
  _fifoSize[FIFO2] = 0;
  _fifoSize[FIFO3] = 0;
}

LaneScheduler::~LaneScheduler() {
  stopRebalancer();

  _managerGuard.reset();
  _managerContext.reset();

  FifoJob* job = nullptr;

  for (int i = 0; i < NUMBER_FIFOS; ++i) {
    _fifoSize[i] = 0;

    while (_fifos[i]->pop(job) && job != nullptr) {
      delete job;
      job = nullptr;
    }
  }
}

bool LaneScheduler::queue(RequestLane lane, std::function<void()> callback,
                          bool allowDirectHandling) {
  RequestPriority prio = PriorityRequestLane(lane);
  bool ok = true;

  switch (prio) {
    // If there is anything in the fifo1 or if the scheduler
    // queue is already full, then append it to the fifo1.
    // Otherwise directly queue it.
    //
    // This does not care if there is anything in fifo2 or
    // fifo8 because these queue have lower priority.
    case RequestPriority::HIGH:
      if (0 < _fifoSize[FIFO1] || !canPostDirectly(lane, prio, allowDirectHandling)) {
        ok = pushToFifo(FIFO1, callback);
      } else {
        post(callback);
      }
      break;

    // If there is anything in the fifo1, fifo2, fifo8
    // or if the scheduler queue is already full, then
    // append it to the fifo2. Otherewise directly queue
    // it.
    case RequestPriority::MED:
      if (0 < _fifoSize[FIFO1] || 0 < _fifoSize[FIFO2] ||
          !canPostDirectly(lane, prio, allowDirectHandling)) {
        ok = pushToFifo(FIFO2, callback);
      } else {
        post(callback);
      }
      break;

    // If there is anything in the fifo1, fifo2, fifo3
    // or if the scheduler queue is already full, then
    // append it to the fifo2. Otherwise directly queue
    // it.
    case RequestPriority::LOW:
      if (0 < _fifoSize[FIFO1] || 0 < _fifoSize[FIFO2] || 0 < _fifoSize[FIFO3] ||
          !canPostDirectly(lane, prio, allowDirectHandling)) {
        ok = pushToFifo(FIFO3, callback);
      } else {
        post(callback);
      }
      break;

    default:
      TRI_ASSERT(false);
      break;
  }

  return ok;
}

bool LaneScheduler::start() {
  // start the I/O
  startIoService();

  TRI_ASSERT(0 < _minThreads);
  TRI_ASSERT(_minThreads <= _maxThreads);

  for (uint64_t i = 0; i < _minThreads; ++i) {
    {
      MUTEX_LOCKER(locker, _threadCreateLock);
      incRunning();
    }
    try {
      startNewThread();
    } catch (...) {
      MUTEX_LOCKER(locker, _threadCreateLock);
      decRunning();
      throw;
    }
  }

  startManagerThread();
  startRebalancer();

  LOG_TOPIC("6fba9", TRACE, arangodb::Logger::FIXME)
      << "all scheduler threads are up and running";

  return Scheduler::start();
}

void LaneScheduler::shutdown() {
  if (isStopping()) {
    return;
  }

  stopRebalancer();
  _threadManager.reset();

  _managerGuard.reset();
  _managerContext->stop();

  _serviceGuard.reset();
  _ioContext->stop();

  // set the flag AFTER stopping the threads
  setStopping();

  // shutdown super class first
  Scheduler::shutdown();

  // and now ourselves
  while (true) {
    uint64_t const counters = _counters.load();

    if (numRunning(counters) == 0 && numWorking(counters) == 0) {
      break;
    }

    std::this_thread::yield();

    // we can be quite generous here with waiting...
    // as we are in the shutdown already, we do not care if we need to wait
    // for a bit longer
    std::this_thread::sleep_for(std::chrono::microseconds(20000));
  }

  // One has to clean up the ioContext here, because there could a lambda
  // in its queue, that requires for it finalization some object (for example
  // vocbase) that would already be destroyed
  _managerContext.reset();
  _ioContext.reset();
}

void LaneScheduler::runSupervisor() {
  while (isStopping()) {
    try {
      _managerContext->run_one();
    } catch (...) {
      LOG_TOPIC("f3f76", ERR, Logger::THREADS)
          << "manager loop caught an error, restarting";
    }
  }
}

void LaneScheduler::runWorker() {
  constexpr size_t everyXSeconds = size_t(MIN_SECONDS);

  // when we enter this method,
  // _nrRunning has already been increased for this thread
  velocypack::Builder b;
  toVelocyPack(b);
  LOG_TOPIC("a23bc", DEBUG, Logger::THREADS) << "started thread: " << b.toJson();

  // some random delay value to avoid all initial threads checking for
  // their deletion at the very same time
  double const randomWait = static_cast<double>(
      RandomGenerator::interval(int64_t(0), static_cast<int64_t>(MIN_SECONDS * 0.5)));

  double start = TRI_microtime() + randomWait;
  size_t counter = 0;
  bool doDecrement = true;

  while (!isStopping()) {
    try {
      _ioContext->run_one();
    } catch (std::exception const& ex) {
      LOG_TOPIC("bac12", ERR, Logger::THREADS)
          << "scheduler loop caught exception: " << ex.what();
    } catch (...) {
      LOG_TOPIC("8bacd", ERR, Logger::THREADS)
          << "scheduler loop caught unknown exception";
    }

    try {
      drain();
    } catch (...) {
    }

    if (++counter > everyXSeconds) {
      counter = 0;

      double const now = TRI_microtime();

      if (now - start > MIN_SECONDS) {
        // test if we should stop this thread, if this returns true,
        // numRunning
        // will have been decremented by one already
        if (threadShouldStop(now)) {
          // nrRunning was decremented already. now exit thread
          doDecrement = false;
          break;
        }

        // use new start time
        start = now;
      }
    }
  }

  toVelocyPack(b);
  LOG_TOPIC("81823", DEBUG, Logger::THREADS) << "stopped (" << b.toJson() << ")";

  if (doDecrement) {
    // only decrement here if this wasn't already done above
    threadHasStopped();
  }
}

// do not pass callback by reference, might get deleted before execution
void LaneScheduler::post(std::function<void()> const callback) {
  // increment number of queued and guard against exceptions
  uint64_t old = incQueued();
  auto guardQueue = scopeGuard([this]() { decQueued(); });

  old += _fifoSize[FIFO1] + _fifoSize[FIFO2] + _fifoSize[FIFO3];

  if (old < 2) {
    JobGuard jobGuard(this);
    jobGuard.work();

    callback();

    drain();
  } else {
    // capture without self, ioContext will not live longer than scheduler
    _ioContext->post([this, callback]() {
      auto guardQueue = scopeGuard([this]() { decQueued(); });

      JobGuard jobGuard(this);
      jobGuard.work();

      callback();
    });

    // no exception happened, cancel guard
    guardQueue.cancel();
  }
}

void LaneScheduler::drain() {
  bool found = true;

  while (found && canPostDirectly(RequestLane::CLIENT_FAST, RequestPriority::HIGH, true)) {
    found = popFifo(FIFO1);
  }

  found = true;

  while (found && canPostDirectly(RequestLane::CLIENT_SLOW, RequestPriority::LOW, false)) {
    found = popFifo(FIFO1);

    if (!found) {
      found = popFifo(FIFO2);
    }

    if (!found) {
      found = popFifo(FIFO3);
    }
  }
}

void LaneScheduler::toVelocyPack(velocypack::Builder& b) const {
  auto counters = getCounters();

  // if you change the names of these attributes, please make sure to
  // also change them in StatisticsWorker.cpp:computePerSeconds
  b.add("scheduler-threads", VPackValue(numRunning(counters)));
  b.add("in-progress", VPackValue(numWorking(counters)));
  b.add("queued", VPackValue(numQueued(counters)));
  b.add("current-fifo1", VPackValue(_fifoSize[FIFO1]));
  b.add("fifo1-size", VPackValue(_maxFifoSize[FIFO1]));
  b.add("current-fifo2", VPackValue(_fifoSize[FIFO2]));
  b.add("fifo2-size", VPackValue(_maxFifoSize[FIFO2]));
  b.add("current-fifo3", VPackValue(_fifoSize[FIFO3]));
  b.add("fifo3-size", VPackValue(_maxFifoSize[FIFO3]));
}

Scheduler::QueueStatistics LaneScheduler::queueStatistics() const {
  auto counters = getCounters();

  // TODO replace by velocypack object
  return QueueStatistics{numRunning(counters), 0, numQueued(counters),
                         numWorking(counters), 0};
}

bool LaneScheduler::canPostDirectly(RequestLane lane, RequestPriority prio,
                                    bool allowDirectHandling) const noexcept {
  if (!allowDirectHandling) {
    return false;
  }

  if (isDirectDeadlockLane(lane)) {
    return false;
  }

  auto counters = getCounters();
  auto nrQueued = numQueued(counters);

  switch (prio) {
    case RequestPriority::HIGH:
      return nrQueued < _maxThreads;

    case RequestPriority::MED:
    case RequestPriority::LOW:
      // the "/ 2" is an assumption that HIGH is typically responses
      // to our outbound messages where MED & LOW are incoming requests.
      // Keep half the threads processing our work and half their work.
      return nrQueued < _maxThreads / 2;
  }

  return false;
}

bool LaneScheduler::pushToFifo(int64_t fifo, std::function<void()> callback) {
  LOG_TOPIC("76189", TRACE, Logger::THREADS) << "Push element on fifo: " << fifo;
  TRI_ASSERT(0 <= fifo && fifo < NUMBER_FIFOS);

  size_t p = static_cast<size_t>(fifo);
  auto job = std::make_unique<FifoJob>(callback);

  try {
    if (0 < _maxFifoSize[p] && (int64_t)_maxFifoSize[p] <= _fifoSize[p]) {
      return false;
    }

    if (!_fifos[p]->push(job.get())) {
      return false;
    }

    job.release();
    ++_fifoSize[p];

    // then check, otherwise we might miss to wake up a thread
    auto counters = getCounters();
    auto nrQueued = numQueued(counters);

    if (0 == nrQueued) {
      post(
          []() {
            LOG_TOPIC("56125", DEBUG, Logger::THREADS) << "Wakeup alarm";
            /*wakeup call for scheduler thread*/
          });
    }
  } catch (...) {
    return false;
  }

  return true;
}

bool LaneScheduler::popFifo(int64_t fifo) {
  LOG_TOPIC("f1f98", TRACE, Logger::THREADS) << "Popping a job from fifo: " << fifo;
  TRI_ASSERT(0 <= fifo && fifo < NUMBER_FIFOS);

  size_t p = static_cast<size_t>(fifo);
  FifoJob* job = nullptr;

  bool ok = _fifos[p]->pop(job) && job != nullptr;

  if (ok) {
    auto guard = scopeGuard([job]() {
      if (job) {
        delete job;
      }
    });

    post(job->_callback);

    --_fifoSize[p];
  }

  return ok;
}

void LaneScheduler::startIoService() {
  _ioContext.reset(new asio_ns::io_context());
  _serviceGuard.reset(new asio_ns::io_context::work(*_ioContext));

  _managerContext.reset(new asio_ns::io_context());
  _managerGuard.reset(new asio_ns::io_context::work(*_managerContext));
}

void LaneScheduler::startManagerThread() {
  auto thread = new SchedulerManagerThread(this);
  if (!thread->start()) {
    THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_FAILED,
                                   "unable to start rebalancer thread");
  }
}

void LaneScheduler::startRebalancer() {
  std::chrono::milliseconds interval(100);
  _threadManager.reset(new asio_ns::steady_timer(*_managerContext));

  _threadHandler = [this, interval](const asio_ns::error_code& error) {
    if (error || isStopping()) {
      return;
    }

    try {
      rebalanceThreads();
    } catch (...) {
      // continue if this fails.
      // we can try rebalancing again in the next round
    }

    if (_threadManager != nullptr) {
      _threadManager->expires_from_now(interval);
      _threadManager->async_wait(_threadHandler);
    }
  };

  _threadManager->expires_from_now(interval);
  _threadManager->async_wait(_threadHandler);
}

void LaneScheduler::stopRebalancer() noexcept {
  if (_threadManager != nullptr) {
    try {
      _threadManager->cancel();
    } catch (...) {
    }
  }
}

//
// This routine tries to keep only the most likely needed count of threads running:
//  - asio io_context runs less efficiently if it has too many threads, but
//  - there is a latency hit to starting a new thread.
//
void LaneScheduler::rebalanceThreads() {
  static uint64_t count = 0;

  ++count;

  if (count % 50 == 0) {
    velocypack::Builder b;
    toVelocyPack(b);
    LOG_TOPIC("d8181", DEBUG, Logger::THREADS) << "rebalancing threads: " << b.toJson();
  } else if (count % 5 == 0) {
    velocypack::Builder b;
    toVelocyPack(b);
    LOG_TOPIC("48132", TRACE, Logger::THREADS) << "rebalancing threads: " << b.toJson();
  }

  while (true) {
    double const now = TRI_microtime();

    {
      MUTEX_LOCKER(locker, _threadCreateLock);

      uint64_t const counters = _counters.load();

      if (isStopping(counters)) {
        // do not start any new threads in case we are already shutting down
        break;
      }

      uint64_t const nrRunning = numRunning(counters);
      uint64_t const nrWorking = numWorking(counters);

      // some threads are idle
      if (nrWorking < nrRunning) {
        break;
      }

      // all threads are maxed out
      _lastAllBusyStamp = now;

      // reached maximum
      if (nrRunning >= _maxThreads) {
        break;
      }

      // increase nrRunning by one here already, while holding the lock
      incRunning();
    }

    // create thread and sleep without holding the mutex
    try {
      startNewThread();
    } catch (...) {
      // cannot create new thread or start new thread
      // if this happens, we have to rollback the increase of nrRunning again
      {
        MUTEX_LOCKER(locker, _threadCreateLock);
        decRunning();
      }
      // add an extra sleep so the system has a chance to recover and provide
      // the needed resources
      std::this_thread::sleep_for(std::chrono::microseconds(20000));
    }

    std::this_thread::sleep_for(std::chrono::microseconds(5000));
  }
}

void LaneScheduler::threadHasStopped() {
  MUTEX_LOCKER(locker, _threadCreateLock);
  decRunning();
}

bool LaneScheduler::threadShouldStop(double now) {
  // make sure no extra threads are created while we check the timestamp
  // and while we modify nrRunning

  MUTEX_LOCKER(locker, _threadCreateLock);

  // fetch all counters in one atomic operation
  uint64_t counters = _counters.load();
  uint64_t const nrRunning = numRunning(counters);

  if (nrRunning <= _minThreads) {
    // don't stop a thread if we already reached the minimum
    // number of threads
    return false;
  }

  if (_lastAllBusyStamp + 1.25 * MIN_SECONDS >= now) {
    // last time all threads were busy is less than x seconds ago
    return false;
  }

  // decrement nrRunning by one already in here while holding the lock
  decRunning();
  return true;
}

void LaneScheduler::startNewThread() {
  TRI_IF_FAILURE("LaneScheduler::startNewThread") {
    LOG_TOPIC("5168c", WARN, Logger::FIXME)
        << "Debug: preventing thread from starting";
    THROW_ARANGO_EXCEPTION(TRI_ERROR_DEBUG);
  }
  auto thread = new SchedulerThread(this);
  if (!thread->start()) {
    THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_FAILED,
                                   "unable to start scheduler thread");
  }
}
