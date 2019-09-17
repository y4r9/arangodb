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
/// @author Simon Grätzer
////////////////////////////////////////////////////////////////////////////////

#ifndef ARANGOD_GENERAL_SERVER_GENERAL_COMM_TASK_H
#define ARANGOD_GENERAL_SERVER_GENERAL_COMM_TASK_H 1

#include "GeneralServer/AsioSocket.h"
#include "GeneralServer/CommTask.h"

namespace arangodb {
namespace rest {

template <SocketType T>
class GeneralCommTask : public CommTask {
  GeneralCommTask(GeneralCommTask const&) = delete;
  GeneralCommTask const& operator=(GeneralCommTask const&) = delete;

 public:
  GeneralCommTask(GeneralServer& server, char const* name, ConnectionInfo,
                  std::unique_ptr<AsioSocket<T>>);

  virtual ~GeneralCommTask();

  void start() override;
  
protected:

  void close(asio_ns::error_code const& ec);

  /// read from socket
  void asyncReadSome();
  /// called to process data in _readBuffer, return false to stop
  virtual bool readCallback(asio_ns::error_code ec) = 0;
  
  template <typename AllocT>
  bool doSyncWrite(std::vector<asio_ns::const_buffer, AllocT>& buffers,
                   asio_ns::error_code& ec) {
    size_t written = 0;
    const size_t total = asio_ns::buffer_size(buffers);
    
    do {
      size_t nwrite = this->_protocol->socket.write_some(buffers, ec);
      written += nwrite;
      if (total == written) {
        ec.clear();
        return true;
      }
      
      while (nwrite > 0) {
        asio_ns::const_buffer b = buffers.front();
        if (nwrite <= b.size()) {
          buffers[0] = asio_ns::buffer(static_cast<const char*>(b.data()) + nwrite,
                                       b.size() - nwrite);
          nwrite = 0;
          break;
        } else {
          nwrite -= b.size();
          buffers.erase(buffers.begin());
        }
      }
      TRI_ASSERT(nwrite == 0);
      
    } while(!ec && written < total);
    
    if (ec == asio_ns::error::would_block ||
        ec == asio_ns::error::try_again) {
      ec.clear();
    }
    TRI_ASSERT(written <= total);
    return total == written;
  }

  /// default max chunksize is 30kb in arangodb (each read fits)
  static constexpr size_t ReadBlockSize = 1024 * 32;

 protected:
  std::unique_ptr<AsioSocket<T>> _protocol;
};
}  // namespace rest
}  // namespace arangodb

#endif
