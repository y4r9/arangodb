////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2014-2020 ArangoDB GmbH, Cologne, Germany
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
/// @author Kaveh Vahedipour
////////////////////////////////////////////////////////////////////////////////

#ifndef ARANGODB_REST_SERVER_METRICS_H
#define ARANGODB_REST_SERVER_METRICS_H 1

#include <atomic>
#include <cassert>
#include <cmath>
#include <deque>
#include <iostream>
#include <limits>
#include <memory>
#include <numeric>
#include <string>
#include <unordered_map>
#include <vector>

#include "Basics/VelocyPackHelper.h"
#include "Logger/LogMacros.h"
#include "counter.h"

class Metric {
 public:
  Metric(std::string const& name, std::string const& help, std::string const& labels);
  virtual ~Metric();
  std::string const& help() const;
  std::string const& name() const;
  std::string const& labels() const;
  virtual void toPrometheus(std::string& result) const = 0;
  void header(std::string& result) const;
 protected:
  std::string const _name;
  std::string const _help;
  std::string const _labels;
};

class PeriodicMetric {
 public:
  virtual ~PeriodicMetric() = default;
  virtual void setSnapshot() = 0;
};

struct Metrics {
  using counter_type = gcl::counter::simplex<uint64_t, gcl::counter::atomicity::full>;
  using hist_type = gcl::counter::simplex_array<uint64_t, gcl::counter::atomicity::full>;
  using buffer_type = gcl::counter::buffer<uint64_t, gcl::counter::atomicity::full, gcl::counter::atomicity::full>;
};


/**
 * @brief Counter functionality
 */
class Counter : public Metric {
 public:
  Counter(uint64_t const& val, std::string const& name, std::string const& help,
          std::string const& labels = std::string());
  Counter(Counter const&) = delete;
  ~Counter();
  std::ostream& print (std::ostream&) const;
  Counter& operator++();
  Counter& operator++(int);
  Counter& operator+=(uint64_t const&);
  Counter& operator=(uint64_t const&);
  void count();
  void count(uint64_t);
  uint64_t load() const;
  void store(uint64_t const&);
  void push();
  virtual void toPrometheus(std::string&) const override;
 private:
  mutable Metrics::counter_type _c;
  mutable Metrics::buffer_type _b;
};


template<typename T> class Gauge : public Metric {
 public:
  Gauge() = delete;
  Gauge(T const& val, std::string const& name, std::string const& help,
        std::string const& labels = std::string())
    : Metric(name, help, labels) {
    _g.store(val);
  }
  Gauge(Gauge const&) = delete;
  ~Gauge() = default;
  std::ostream& print (std::ostream&) const;
  Gauge<T>& operator+=(T const& t) {
    T tmp;
    do {
      tmp = _g.load(std::memory_order_acquire);
    } while (!_g.compare_exchange_weak(tmp, tmp + t, std::memory_order_acq_rel,
                                       std::memory_order_acquire));
    return *this;
  }
  Gauge<T>& operator-=(T const& t) {
    T tmp;
    do {
      tmp = _g.load(std::memory_order_acquire);
    } while (!_g.compare_exchange_weak(tmp, tmp - t, std::memory_order_acq_rel,
                                       std::memory_order_acquire));
    return *this;
  }
  Gauge<T>& operator=(T const& t) {
    _g.store(t, std::memory_order_release);
    return *this;
  }
  T load() const { return _g.load(std::memory_order_acquire); }
  virtual void toPrometheus(std::string& result) const override {
    result += "\n#TYPE " + name() + " gauge\n";
    result += "#HELP " + name() + " " + help() + "\n";
    result += name() + "{" + labels() + "} " + std::to_string(load()) + "\n";
  };
 private:
  std::atomic<T> _g;
};

template <typename T>
class PeriodicStatistics;

template <typename T>
class Statistics : public Metric {
 public:
  class Snapshot {
    friend class Statistics<T>;
    friend class PeriodicStatistics<T>;

    std::size_t _count = 0;
    T _sum = {};
    T _min = {};
    T _max = {};

   public:
    T min() const { return _min; };
    T max() const { return _max; };
    T median() const {
      return _count > 0 ? _sum / static_cast<T>(_count) : T{};
    }
  };

  Statistics() = delete;
  Statistics(std::string const& name, std::string const& help,
             std::string const& labels = std::string())
      : Metric(name, help, labels) {}
  Statistics(Statistics const&) = delete;
  virtual ~Statistics() = default;

  std::ostream& print(std::ostream&) const;

  void count(T& sample) {
    Snapshot exp = load();
    while (true) {
      Snapshot mod = exp;
      if (mod._count == 0) {
        mod._min = sample;
        mod._max = sample;
      } else {
        mod._min = std::min(sample, mod._min);
        mod._max = std::max(sample, mod._max);
      }
      mod._sum += sample;
      ++mod._count;
      if (_snapshot.compare_exchange_strong(exp, mod, std::memory_order_acq_rel,
                                            std::memory_order_acquire)) {
        break;
      }
    }
  }

  Snapshot load() const { return _snapshot.load(std::memory_order_acquire); }
  virtual void toPrometheus(std::string& result) const override {
    Snapshot snap = load();
    result += "\n#TYPE " + name() + "_min gauge\n";
    result += "#HELP " + name() + "_min " + help() + "\n";
    result +=
        name() + "_min{" + labels() + "} " + std::to_string(snap.min()) + "\n";
    result += "\n#TYPE " + name() + "_max gauge\n";
    result += "#HELP " + name() + +"_max " + help() + "\n";
    result +=
        name() + "_max{" + labels() + "} " + std::to_string(snap.max()) + "\n";
    result += "\n#TYPE " + name() + "_median gauge\n";
    result += "#HELP " + name() + "_median " + help() + "\n";
    result += name() + "_median{" + labels() + "} " +
              std::to_string(snap.median()) + "\n";
  };

 protected:
  std::atomic<Snapshot> _snapshot;
};

template <typename T>
class PeriodicStatistics : public Statistics<T>, public PeriodicMetric {
 public:
  using Snapshot = typename Statistics<T>::Snapshot;

  PeriodicStatistics() = delete;

  PeriodicStatistics(std::size_t historyCount, std::string const& name,
                     std::string const& help, std::string const& labels = std::string())
      : Statistics<T>(name, help, labels), _historyCount(historyCount) {}

  void setSnapshot() override {
    std::unique_lock<std::mutex> guard(_lock);
    Snapshot reset;
    Snapshot exp = Statistics<T>::load();
    while (!this->_snapshot.compare_exchange_strong(exp, reset, std::memory_order_acq_rel,
                                                    std::memory_order_acquire)) {
      continue;
    }
    _history.emplace_front(exp);
    while (_history.size() > _historyCount) {
      _history.pop_back();
    }
  }

  Snapshot since(std::size_t steps = 0) const {
    std::unique_lock<std::mutex> guard(_lock);
    Snapshot s;
    for (std::size_t i = 0; i <= steps && i < _history.size(); ++i) {
      if (s._count == 0) {
        s = _history[i];
      } else if (_history[i]._count > 0) {
        s._min = std::min(s._min, _history[i]._min);
        s._max = std::max(s._max, _history[i]._max);
        s._sum += _history[i]._sum;
        s._count += _history[i]._count;
      }
    }
    return s;
  }

 private:
  mutable std::mutex _lock;
  std::size_t _historyCount;
  std::deque<Snapshot> _history;
};

std::ostream& operator<< (std::ostream&, Metrics::hist_type const&);

enum ScaleType { FIXED, LINEAR, LOGARITHMIC };

template<typename T>
struct scale_t {
 public:

  using value_type = T;

  scale_t(T const& low, T const& high, size_t n) :
    _low(low), _high(high), _n(n) {
    TRI_ASSERT(n > 1);
    _delim.resize(n - 1);
  }
  virtual ~scale_t() = default;
  /**
   * @brief number of buckets
   */
  size_t n() const {
    return _n;
  }
  /**
   * @brief number of buckets
   */
  T low() const {
    return _low;
  }
  /**
   * @brief number of buckets
   */
  T high() const {
    return _high;
  }
  /**
   * @brief number of buckets
   */
  std::string const delim(size_t const& s) const {
    return (s < _n - 1) ? std::to_string(_delim.at(s)) : "+Inf";
  }
  /**
   * @brief number of buckets
   */
  std::vector<T> const& delims() const {
    return _delim;
  }
  /**
   * @brief dump to builder
   */
  virtual void toVelocyPack(VPackBuilder& b) const {
    TRI_ASSERT(b.isOpenObject());
    b.add("lower-limit", VPackValue(_low));
    b.add("upper-limit", VPackValue(_high));
    b.add("value-type", VPackValue(typeid(T).name()));
    b.add(VPackValue("range"));
    VPackArrayBuilder abb(&b);
    for (auto const& i : _delim) {
      b.add(VPackValue(i));
    }
  }
  /**
   * @brief dump to
   */
  std::ostream& print(std::ostream& o) const {
    VPackBuilder b;
    {
      VPackObjectBuilder bb(&b);
      this->toVelocyPack(b);
    }
    o << b.toJson();
    return o;
  }

 protected:
  T _low, _high;
  std::vector<T> _delim;
  size_t _n;
};

template<typename T>
std::ostream& operator<< (std::ostream& o, scale_t<T> const& s) {
  return s.print(o);
}

template<typename T>
struct log_scale_t : public scale_t<T> {
 public:

  using value_type = T;
  static constexpr ScaleType scale_type = LOGARITHMIC;

  log_scale_t(T const& base, T const& low, T const& high, size_t n) :
    scale_t<T>(low, high, n), _base(base) {
    TRI_ASSERT(base > T(0));
    double nn = -1.0 * (n - 1);
    for (auto& i : this->_delim) {
      i = static_cast<T>(
        static_cast<double>(high - low) *
        std::pow(static_cast<double>(base), static_cast<double>(nn++)) + static_cast<double>(low));
    }
    _div = this->_delim.front() - low;
    TRI_ASSERT(_div > T(0));
    _lbase = log(_base);
  }
  virtual ~log_scale_t() = default;
  /**
   * @brief index for val
   * @param val value
   * @return    index
   */
  size_t pos(T const& val) const {
    return static_cast<size_t>(1+std::floor(log((val - this->_low)/_div)/_lbase));
  }
  /**
   * @brief Dump to builder
   * @param b Envelope
   */
  virtual void toVelocyPack(VPackBuilder& b) const override {
    b.add("scale-type", VPackValue("logarithmic"));
    b.add("base", VPackValue(_base));
    scale_t<T>::toVelocyPack(b);
  }
  /**
   * @brief Base
   * @return base
   */
  T base() const {
    return _base;
  }
 
 private:
  T _base, _div;
  double _lbase;
};

template<typename T>
struct lin_scale_t : public scale_t<T> {
 public:

  using value_type = T;
  static constexpr ScaleType scale_type = LINEAR;

  lin_scale_t(T const& low, T const& high, size_t n) :
    scale_t<T>(low, high, n) {
    this->_delim.resize(n-1);
    _div = (high - low) / (T)n;
    if (_div <= 0) {
    }
    TRI_ASSERT(_div > 0);
    T le = low;
    for (auto& i : this->_delim) {
      le += _div;
      i = le;
    }
  }
  virtual ~lin_scale_t() = default;
  /**
   * @brief index for val
   * @param val value
   * @return    index
   */
  size_t pos(T const& val) const {
    return static_cast<size_t>(std::floor((val - this->_low)/ _div));
  }

  virtual void toVelocyPack(VPackBuilder& b) const override {
    b.add("scale-type", VPackValue("linear"));
    scale_t<T>::toVelocyPack(b);
  }
 
 private:
  T _base, _div;
};

template <typename T>
struct fixed_scale_t : public scale_t<T> {
 public:
  using value_type = T;
  static constexpr ScaleType scale_type = FIXED;

  fixed_scale_t(T const& low, T const& high, std::initializer_list<T> const& list)
      : scale_t<T>(low, high, list.size() + 1) {
    this->_delim = list;
  }
  virtual ~fixed_scale_t() = default;
  /**
   * @brief index for val
   * @param val value
   * @return    index
   */
  size_t pos(T const& val) const {
    for (std::size_t i = 0; i < this->_delim.size(); ++i) {
      if (val <= this->_delim[i]) {
        return i;
      }
    }
    return this->_delim.size();
  }

  virtual void toVelocyPack(VPackBuilder& b) const override {
    b.add("scale-type", VPackValue("fixed"));
    scale_t<T>::toVelocyPack(b);
  }

 private:
  T _base, _div;
};

template<typename ... Args>
std::string strfmt (std::string const& format, Args ... args) {
  size_t size = snprintf( nullptr, 0, format.c_str(), args ... ) + 1;
  if( size <= 0 ) {
    throw std::runtime_error( "Error during formatting." );
  }
  std::unique_ptr<char[]> buf(new char[size]);
  snprintf(buf.get(), size, format.c_str(), args ...);
  return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}



/**
 * @brief Histogram functionality
 */
template<typename Scale> class Histogram : public Metric {

 public:

  using value_type = typename Scale::value_type;

  Histogram() = delete;

  Histogram(Scale&& scale, std::string const& name, std::string const& help,
            std::string const& labels = std::string())
    : Metric(name, help, labels), _c(Metrics::hist_type(scale.n())), _scale(std::move(scale)),
      _lowr(std::numeric_limits<value_type>::max()),
      _highr(std::numeric_limits<value_type>::min()),
      _n(_scale.n() - 1) {}

  Histogram(Scale const& scale, std::string const& name, std::string const& help,
            std::string const& labels = std::string())
    : Metric(name, help, labels), _c(Metrics::hist_type(scale.n())), _scale(scale),
      _lowr(std::numeric_limits<value_type>::max()),
      _highr(std::numeric_limits<value_type>::min()),
      _n(_scale.n() - 1) {}

  ~Histogram() = default;

  void records(value_type const& val) {
    if (val < _lowr) {
      _lowr = val;
    } else if (val > _highr) {
      _highr = val;
    }
  }

  Scale const& scale() {
    return _scale;
  }

  size_t pos(value_type const& t) const {
    return _scale.pos(t);
  }

  void count(value_type const& t) {
    count(t, 1);
  }

  void count(value_type const& t, uint64_t n) {
    if (t < _scale.delims().front()) {
      _c[0] += n;
    } else if (t >= _scale.delims().back()) {
      _c[_n] += n;
    } else {
      _c[pos(t)] += n;
    }
    records(t);
  }

  value_type const& low() const { return _scale.low(); }
  value_type const& high() const { return _scale.high(); }

  Metrics::hist_type::value_type& operator[](size_t n) {
    return _c[n];
  }

  std::vector<uint64_t> load() const {
    std::vector<uint64_t> v(size());
    for (size_t i = 0; i < size(); ++i) {
      v[i] = load(i);
    }
    return v;
  }

  uint64_t load(size_t i) const { return _c.load(i); };

  size_t size() const { return _c.size(); }

  virtual void toPrometheus(std::string& result) const override {
    result += "\n#TYPE " + name() + " histogram\n";
    result += "#HELP " + name() + " " + help() + "\n";
    std::string lbs = labels();
    auto const haveLabels = !lbs.empty();
    auto const separator = haveLabels && lbs.back() != ',';
    uint64_t sum(0);
    for (size_t i = 0; i < size(); ++i) {
      uint64_t n = load(i);
      sum += n;
      result += name() + "_bucket{";
      if (haveLabels) {
        result += lbs;
      }
      if (separator) {
        result += ",";
      }
      result += "le=\"" + _scale.delim(i) + "\"} " + std::to_string(n) + "\n";
    }
    result += name() + "_count{" + labels() + "} " + std::to_string(sum) + "\n";
  }

  std::ostream& print(std::ostream& o) const {
    o << name() << " scale: " <<  _scale << " extremes: [" << _lowr << ", " << _highr << "]";
    return o;
  }

 private:
  Metrics::hist_type _c;
  Scale _scale;
  value_type _lowr, _highr;
  size_t _n;

};

template <typename Scale>
class Heatmap : public Histogram<Scale>, public PeriodicMetric {
 public:
  Heatmap() = delete;

  Heatmap(std::size_t historyCount, Scale const& scale, std::string const& name,
          std::string const& help, std::string const& labels = std::string())
      : Histogram<Scale>(scale, name, help, labels), _historyCount(historyCount) {}

  void setSnapshot() override {
    std::unique_lock<std::mutex> guard(_lock);
    _history.emplace_front(Histogram<Scale>::load());
    while (_history.size() > _historyCount) {
      _history.pop_back();
    }
  }

  std::vector<uint64_t> diff(std::size_t steps = 1) const {
    std::size_t const size = Histogram<Scale>::size();
    std::vector<uint64_t> v(size, 0);
    std::unique_lock<std::mutex> guard(_lock);
    if (_history.size() == 1) {
      return _history[0];  // nothing to subtract
    } else if (!_history.empty()) {
      std::size_t const offset = std::min(steps, _history.size() - 1);
      for (std::size_t i = 0; i < size; ++i) {
        v[i] = _history[0][i] - _history[offset][i];
      }
    }
    return v;
  }

  std::vector<double> normalizedDiff(std::size_t steps = 1) const {
    std::size_t const size = Histogram<Scale>::size();
    std::vector<double> n(size, 0.0);
    std::vector<uint64_t> h = diff(steps);
    uint64_t sum = std::reduce(h.begin(), h.end());
    for (std::size_t i = 0; i < size; ++i) {
      n[i] = static_cast<double>(h[i]) / static_cast<double>(sum);
    }
    return n;
  }

 private:
  mutable std::mutex _lock;
  std::size_t _historyCount;
  std::deque<std::vector<uint64_t>> _history;
};

std::ostream& operator<< (std::ostream&, Metrics::counter_type const&);
template<typename T>
std::ostream& operator<<(std::ostream& o, Histogram<T> const& h) {
  return h.print(o);
}

#endif
