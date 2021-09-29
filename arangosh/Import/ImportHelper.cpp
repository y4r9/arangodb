////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2014-2021 ArangoDB GmbH, Cologne, Germany
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

#include "ImportHelper.h"
#include "Basics/ConditionLocker.h"
#include "Basics/MutexLocker.h"
#include "Basics/ScopeGuard.h"
#include "Basics/StringUtils.h"
#include "Basics/VPackStringBufferAdapter.h"
#include "Basics/files.h"
#include "Basics/system-functions.h"
#include "Basics/tri-strings.h"
#include "Import/SenderThread.h"
#include "Logger/Logger.h"
#include "Rest/GeneralResponse.h"
#include "Shell/ClientFeature.h"
#include "SimpleHttpClient/SimpleHttpClient.h"
#include "SimpleHttpClient/SimpleHttpResult.h"
#include "Utils/ManagedDirectory.h"

#include <velocypack/Builder.h>
#include <velocypack/Dumper.h>
#include <velocypack/Iterator.h>
#include <velocypack/velocypack-aliases.h>

#ifdef TRI_HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace arangodb;
using namespace arangodb::basics;
using namespace arangodb::httpclient;
using namespace std::literals::string_literals;

namespace arangodb::import {

ImportStatistics::ImportStatistics(application_features::ApplicationServer& server)
    : _histogram(server) {}

////////////////////////////////////////////////////////////////////////////////
/// initialize step value for progress reports
////////////////////////////////////////////////////////////////////////////////

double const ImportHelper::ProgressStep = 3.0;

////////////////////////////////////////////////////////////////////////////////
/// the server has a built-in limit for the batch size
///  and will reject bigger HTTP request bodies
////////////////////////////////////////////////////////////////////////////////

unsigned const ImportHelper::MaxBatchSize = 768 * 1024 * 1024;

////////////////////////////////////////////////////////////////////////////////
/// constructor and destructor
////////////////////////////////////////////////////////////////////////////////

ImportHelper::ImportHelper(ClientFeature const& client, std::string const& endpoint,
                           httpclient::SimpleHttpClientParams const& params,
                           uint64_t maxUploadSize, uint32_t threadCount, bool autoUploadSize)
    : _clientFeature(client),
      _httpClient(client.createHttpClient(endpoint, params)),
      _maxUploadSize(maxUploadSize),
      _periodByteCount(0),
      _threadCount(threadCount),
      _tempBuffer(false),
      _separator(","),
      _quote("\""),
      _createCollectionType("document"),
      _autoUploadSize(autoUploadSize),
      _useBackslash(false),
      _convert(true),
      _createCollection(false),
      _overwrite(false),
      _progress(false),
      _firstChunk(true),
      _ignoreMissing(false),
      _skipValidation(false),
      _numberLines(0),
      _stats(client.server()),
      _rowsRead(0),
      _rowOffset(0),
      _rowsToSkip(0),
      _keyColumn(-1),
      _onDuplicateAction("error"),
      _outputBuffer(false),
      _firstLine(&_options),
      _lineBuilder(&_options),
      _parser(_singleValueBuilder),
      _hasError(false),
      _headersSeen(false) {
  for (uint32_t i = 0; i < threadCount; i++) {
    auto http = client.createHttpClient(endpoint, params);
    _senderThreads.emplace_back(
        std::make_unique<SenderThread>(client.server(), std::move(http), &_stats, [this]() {
          CONDITION_LOCKER(guard, _threadsCondition);
          guard.signal();
        }));
    _senderThreads.back()->start();
  }

  // should self tuning code activate?
  if (_autoUploadSize) {
    _autoTuneThread = std::make_unique<AutoTuneThread>(client.server(), *this);
    _autoTuneThread->start();
  } 

  // wait until all sender threads are ready
  while (true) {
    uint32_t numReady = 0;
    for (auto const& t : _senderThreads) {
      if (t->isIdle()) {
        numReady++;
      }
    }
    if (numReady == _senderThreads.size()) {
      break;
    }
  }
 
  _options.paddingBehavior = arangodb::velocypack::Options::PaddingBehavior::UsePadding;
  _options.buildUnindexedArrays = true;
  _options.escapeUnicode = false;
  _options.escapeForwardSlashes = false;
  
}

ImportHelper::~ImportHelper() {
  for (auto const& t : _senderThreads) {
    t->beginShutdown();
  }
}

// read headers from separate file
bool ImportHelper::readHeadersFile(std::string const& headersFile,
                                   DelimitedImportType typeImport,
                                   char separator) {
  TRI_ASSERT(!headersFile.empty());
  TRI_ASSERT(!_headersSeen);
  
  ManagedDirectory directory(_clientFeature.server(), TRI_Dirname(headersFile),
                             false, false, false);
  if (directory.status().fail()) {
    _errorMessages.emplace_back(directory.status().errorMessage());
    return false;
  }

  std::string fileName(TRI_Basename(headersFile.c_str()));
  std::unique_ptr<arangodb::ManagedDirectory::File> fd = directory.readableFile(fileName.c_str(), 0);
  if (!fd) {
    _errorMessages.push_back(TRI_LAST_ERROR_STR);
    return false;
  }

  // make a copy of _rowsToSkip
  size_t rowsToSkip = _rowsToSkip;
  _rowsToSkip = 0;

  TRI_csv_parser_t parser;
  TRI_InitCsvParser(&parser, ProcessCsvBegin, ProcessCsvAdd, ProcessCsvEnd, nullptr);
  TRI_SetSeparatorCsvParser(&parser, separator);
  TRI_UseBackslashCsvParser(&parser, _useBackslash);
  
  // in csv, we'll use the quote char if set
  // in tsv, we do not use the quote char
  if (typeImport == ImportHelper::CSV && _quote.size() > 0) {
    TRI_SetQuoteCsvParser(&parser, _quote[0], true);
  } else {
    TRI_SetQuoteCsvParser(&parser, '\0', false);
  }
  parser._dataAdd = this;

  auto guard = scopeGuard([&parser]() noexcept {
    TRI_DestroyCsvParser(&parser);
  });
  
  constexpr int BUFFER_SIZE = 16384;
  char buffer[BUFFER_SIZE];

  while (!_hasError) {
    ssize_t n = fd->read(buffer, sizeof(buffer));

    if (n < 0) {
      _errorMessages.push_back(TRI_LAST_ERROR_STR);
      return false;
    } 
    if (n == 0) {
      // we have read the entire file
      // now have the CSV parser parse an additional new line so it
      // will definitely process the last line of the input data if
      // it did not end with a newline
      TRI_ParseCsvString(&parser, "\n", 1);
      break;
    }

    TRI_ParseCsvString(&parser, buffer, n);
  }

  if (_rowsRead > 2) {
    _errorMessages.push_back("headers file '" + headersFile + "' contained more than a single line of headers");
    return false;
  }

  // reset our state properly
  _headersSeen = true;
  _rowOffset = 0;
  _rowsRead = 0;
  _numberLines = 0;
  // restore copy of _rowsToSkip 
  _rowsToSkip = rowsToSkip;

  _lineBuilder.clear();

  return true;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief imports a delimited file
////////////////////////////////////////////////////////////////////////////////

bool ImportHelper::importDelimited(std::string const& collectionName,
                                   std::string const& pathName,
                                   std::string const& headersFile,
                                   DelimitedImportType typeImport) {
  ManagedDirectory directory(_clientFeature.server(), TRI_Dirname(pathName),
                             false, false, true);
  if (directory.status().fail()) {
    _errorMessages.emplace_back(directory.status().errorMessage());
    return false;
  }

  std::string fileName(TRI_Basename(pathName.c_str()));
  _collectionName = collectionName;
  _hasError = false;
  _headersSeen = false;
  _rowOffset = 0;
  _rowsRead = 0;
  _numberLines = 0;

  if (!checkCreateCollection()) {
    return false;
  }
  if (!collectionExists()) {
    return false;
  }
 
  // handle separator
  char separator;
  {
    size_t separatorLength;
    char* s = TRI_UnescapeUtf8String(_separator.c_str(), _separator.size(),
                                     &separatorLength, true);

    if (s == nullptr) {
      _errorMessages.push_back("out of memory");
      return false;
    }
 
    separator = s[0];
    TRI_Free(s);
  }

  if (!headersFile.empty() &&
      !readHeadersFile(headersFile, typeImport, separator)) {
    return false;
  }

  // read and convert
  int64_t totalLength;
  std::unique_ptr<arangodb::ManagedDirectory::File> fd;

  if (fileName == "-") {
    // we don't have a filesize
    totalLength = 0;
    fd = directory.readableFile(STDIN_FILENO);
  } else {
    // read filesize
    totalLength = TRI_SizeFile(pathName.c_str());
    fd = directory.readableFile(fileName.c_str(), 0);

    if (!fd) {
      _errorMessages.push_back(TRI_LAST_ERROR_STR);
      return false;
    }
  }

  // progress display control variables
  double nextProgress = ProgressStep;

  TRI_csv_parser_t parser;
  TRI_InitCsvParser(&parser, ProcessCsvBegin, ProcessCsvAdd, ProcessCsvEnd, nullptr);
  TRI_SetSeparatorCsvParser(&parser, separator);
  TRI_UseBackslashCsvParser(&parser, _useBackslash);

  // in csv, we'll use the quote char if set
  // in tsv, we do not use the quote char
  if (typeImport == ImportHelper::CSV && _quote.size() > 0) {
    TRI_SetQuoteCsvParser(&parser, _quote[0], true);
  } else {
    TRI_SetQuoteCsvParser(&parser, '\0', false);
  }
  parser._dataAdd = this;
  
  auto guard = scopeGuard([&parser]() noexcept {
    TRI_DestroyCsvParser(&parser);
  });

  constexpr int BUFFER_SIZE = 262144;
  char buffer[BUFFER_SIZE];

  while (!_hasError) {
    ssize_t n = fd->read(buffer, sizeof(buffer));

    if (n < 0) {
      _errorMessages.push_back(TRI_LAST_ERROR_STR);
      return false;
    } else if (n == 0) {
      // we have read the entire file
      // now have the CSV parser parse an additional new line so it
      // will definitely process the last line of the input data if
      // it did not end with a newline
      TRI_ParseCsvString(&parser, "\n", 1);
      break;
    }

    reportProgress(totalLength, fd->offset(), nextProgress);

    TRI_ParseCsvString(&parser, buffer, n);
  }

  if (_outputBuffer.length() > 0) {
    sendCsvBuffer();
  }

  waitForSenders();
  reportProgress(totalLength, fd->offset(), nextProgress);

  return !_hasError;
}

bool ImportHelper::importJson(std::string const& collectionName,
                              std::string const& pathName, bool assumeLinewise) {
  ManagedDirectory directory(_clientFeature.server(), TRI_Dirname(pathName),
                             false, false, true);
  std::string fileName(TRI_Basename(pathName.c_str()));
  _collectionName = collectionName;
  _hasError = false;

  if (!checkCreateCollection()) {
    return false;
  }
  if (!collectionExists()) {
    return false;
  }

  // read and convert
  int64_t totalLength;
  std::unique_ptr<arangodb::ManagedDirectory::File> fd;

  if (fileName == "-") {
    // we don't have a filesize
    totalLength = 0;
    fd = directory.readableFile(STDIN_FILENO);
  } else {
    // read filesize
    totalLength = TRI_SizeFile(pathName.c_str());
    fd = directory.readableFile(fileName.c_str(), 0);

    if (!fd) {
      _errorMessages.push_back(TRI_LAST_ERROR_STR);
      return false;
    }
  }

  bool isObject = false;
  bool checkedFront = false;

  if (assumeLinewise) {
    checkedFront = true;
    isObject = false;
  }

  // progress display control variables
  double nextProgress = ProgressStep;

  constexpr int BUFFER_SIZE = 1048576;

  while (!_hasError) {
    // reserve enough room to read more data
    if (_outputBuffer.reserve(BUFFER_SIZE) == TRI_ERROR_OUT_OF_MEMORY) {
      _errorMessages.emplace_back(TRI_errno_string(TRI_ERROR_OUT_OF_MEMORY));

      return false;
    }

    // read directly into string buffer
    ssize_t n = fd->read(_outputBuffer.end(), BUFFER_SIZE - 1);

    if (n < 0) {
      _errorMessages.push_back(TRI_LAST_ERROR_STR);
      return false;
    } else if (n == 0) {
      // we're done
      break;
    }

    // adjust size of the buffer by the size of the chunk we just read
    _outputBuffer.increaseLength(n);

    if (!checkedFront) {
      // detect the import file format (single lines with individual JSON
      // objects or a JSON array with all documents)
      char const* p = _outputBuffer.begin();
      char const* e = _outputBuffer.end();

      while (p < e && (*p == ' ' || *p == '\r' || *p == '\n' || *p == '\t' ||
                       *p == '\f' || *p == '\b')) {
        ++p;
      }

      isObject = (*p == '[');
      checkedFront = true;
    }

    reportProgress(totalLength, fd->offset(), nextProgress);

    uint64_t maxUploadSize = getMaxUploadSize();

    if (_outputBuffer.length() > maxUploadSize) {
      if (isObject) {
        _errorMessages.push_back(
            "import file is too big. please increase the value of --batch-size "
            "(currently " +
            StringUtils::itoa(maxUploadSize) + ")");
        return false;
      }

      // send all data before last '\n'
      char const* first = _outputBuffer.c_str();
      char const * pos = static_cast<char const*>(memrchr(first, '\n', _outputBuffer.length()));

      if (pos != nullptr) {
        size_t len = pos - first + 1;
        char const * cursor = first;
        do {
          ++cursor;
          cursor = static_cast<char const*>(memchr(cursor, '\n', pos - cursor));
          ++_rowsRead;
        } while (nullptr != cursor);
        sendJsonBuffer(first, len, isObject);
        _outputBuffer.erase_front(len);
        _rowOffset = _rowsRead;
      }
    }
  }

  if (_outputBuffer.length() > 0) {
    ++_rowsRead;
    sendJsonBuffer(_outputBuffer.c_str(), _outputBuffer.length(), isObject);
  }

  waitForSenders();
  reportProgress(totalLength, fd->offset(), nextProgress);

  MUTEX_LOCKER(guard, _stats._mutex);
  // this is an approximation only. _numberLines is more meaningful for CSV
  // imports
  _numberLines = _stats._numberErrors + _stats._numberCreated +
                 _stats._numberIgnored + _stats._numberUpdated;
  _outputBuffer.clear();
  return !_hasError;
}

////////////////////////////////////////////////////////////////////////////////
/// private functions
////////////////////////////////////////////////////////////////////////////////

void ImportHelper::reportProgress(int64_t totalLength, int64_t totalRead, double& nextProgress) {
  if (!_progress) {
    return;
  }

  using arangodb::basics::StringUtils::formatSize;

  if (totalLength == 0) {
    // length of input is unknown
    // in this case we cannot report the progress as a percentage
    // instead, report every 10 MB processed
    static int64_t nextProcessed = 10 * 1000 * 1000;

    if (totalRead >= nextProcessed) {
      LOG_TOPIC("c0e6e", INFO, arangodb::Logger::FIXME)
          << "processed " << formatSize(totalRead) << " of input file";
      nextProcessed += 10 * 1000 * 1000;
    }
  } else {
    double pct = 100.0 * ((double)totalRead / (double)totalLength);

    if (pct >= nextProgress && totalLength >= 1024) {
      LOG_TOPIC("9ddf3", INFO, arangodb::Logger::FIXME)
          << "processed " << formatSize(totalRead) << " (" << (int)nextProgress
          << "%) of input file";
      nextProgress = (double)((int)(pct + ProgressStep));
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
/// @brief return the collection-related URL part
////////////////////////////////////////////////////////////////////////////////

std::string ImportHelper::getCollectionUrlPart() const {
  return std::string("collection=" + StringUtils::urlEncode(_collectionName));
}

////////////////////////////////////////////////////////////////////////////////
/// @brief start a new csv line
////////////////////////////////////////////////////////////////////////////////

void ImportHelper::ProcessCsvBegin(TRI_csv_parser_t* parser, size_t row) {
  static_cast<ImportHelper*>(parser->_dataAdd)->beginLine(row);
}

void ImportHelper::beginLine(size_t row) {
  ++_numberLines;
  _lineBuilder.clear();
}

////////////////////////////////////////////////////////////////////////////////
/// @brief adds a new CSV field
////////////////////////////////////////////////////////////////////////////////

void ImportHelper::ProcessCsvAdd(TRI_csv_parser_t* parser, char const* field, size_t fieldLength,
                                 size_t row, size_t column, bool escaped) {
  auto importHelper = static_cast<ImportHelper*>(parser->_dataAdd);
  importHelper->addField(field, fieldLength, row, column, escaped);
}

void ImportHelper::addField(char const* field, size_t fieldLength, size_t row,
                            size_t column, bool escaped) {
  if (_rowsRead < _rowsToSkip) {
    // still some rows left to skip over
    return;
  }
  // we are reading the first line if we get here
  if (row == _rowsToSkip && !_headersSeen) {
    std::string name = std::string(field, fieldLength);
    if (fieldLength > 0) {  // translate field
      auto it = _translations.find(name);
      if (it != _translations.end()) {
        field = (*it).second.c_str();
        fieldLength = (*it).second.size();
      }
    }
    _columnNames.push_back(std::move(name));
  }

  // skip removable attributes
  if (!_removeAttributes.empty() && column < _columnNames.size() &&
      _removeAttributes.find(_columnNames[column]) != _removeAttributes.end()) {
    return;
  }

  // we will write out this attribute!

  // prepare Builder
  if (_lineBuilder.isEmpty()) {
    _lineBuilder.openArray(true);
  }

  if (_keyColumn == -1 && row == _rowsToSkip && !_headersSeen && fieldLength == 4 &&
      memcmp(field, "_key", 4) == 0) {
    _keyColumn = column;
  }
  
  // check if a datatype was forced for this attribute
  auto itTypes = _datatypes.end();
  if (!_datatypes.empty() && column < _columnNames.size()) {
    itTypes = _datatypes.find(_columnNames[column]);
  }

  if ((row == _rowsToSkip && !_headersSeen) ||
      (escaped && itTypes == _datatypes.end()) ||
      _keyColumn == static_cast<decltype(_keyColumn)>(column)) {
    // headline or escaped value
    _lineBuilder.add(VPackValuePair(reinterpret_cast<uint8_t const*>(field), fieldLength, VPackValueType::String));
    return;
  }
  
  // check if a datatype was forced for this attribute
  if (itTypes != _datatypes.end()) {
    std::string const& datatype = (*itTypes).second;
    if (datatype == "number") {
      // try to parse value
      if (fieldLength > 0 &&
          (*field != ' ' && *field != '\t' && *field != '\n' && *field != '\r')) {
        try {
          _parser.parse(field, fieldLength);
          if (_singleValueBuilder.slice().isNumber()) {
            // use result only if it was a number
            _lineBuilder.add(_singleValueBuilder.slice());
            return;
          }
        } catch (...) {
        }
      }
      // couldn't interpret input value as number, so add 0.
      _lineBuilder.add(VPackValue(0));
    } else if (datatype == "boolean") {
      if ((fieldLength == 5 && memcmp(field, "false", 5) == 0) ||
          (fieldLength == 4 && memcmp(field, "null", 4) == 0) ||
          (fieldLength == 1 && *field == '0')) {
        _lineBuilder.add(VPackValue(false));
      } else {
        _lineBuilder.add(VPackValue(true));
      }
    } else if (datatype == "null") {
      _lineBuilder.add(VPackValue(VPackValueType::Null));
    } else {
      // string
      TRI_ASSERT(datatype == "string");
      _lineBuilder.add(VPackValuePair(reinterpret_cast<uint8_t const*>(field), fieldLength, VPackValueType::String));
    }
    return;
  }

  if (*field == '\0' || fieldLength == 0) {
    // do nothing
    _lineBuilder.add(VPackValue(VPackValueType::Null));
    return;
  }

  // automatic detection of datatype based on value (--convert)
  if (_convert &&
      /* true, false, null, number */
      (*field == 't' || *field == 'f' || *field == 'n' || *field == '-' || *field == '+' || (*field >= '0' && *field <= '9'))) {
    // check for literals null, false, true and numbers
    try {
      _singleValueBuilder.clear();
      _parser.parse(field, fieldLength);
      _lineBuilder.add(_singleValueBuilder.slice());
      return;
    } catch (...) {
    }
  }

  _lineBuilder.add(VPackValuePair(reinterpret_cast<uint8_t const*>(field), fieldLength, VPackValueType::String));
}

////////////////////////////////////////////////////////////////////////////////
/// @brief ends a CSV line
////////////////////////////////////////////////////////////////////////////////

void ImportHelper::ProcessCsvEnd(TRI_csv_parser_t* parser, char const* field, size_t fieldLength,
                                 size_t row, size_t column, bool escaped) {
  auto importHelper = static_cast<ImportHelper*>(parser->_dataAdd);

  if (importHelper->getRowsRead() >= importHelper->getRowsToSkip()) {
    importHelper->addLastField(field, fieldLength, row, column, escaped);
  }
  importHelper->incRowsRead();
}

void ImportHelper::addLastField(char const* field, size_t fieldLength,
                                size_t row, size_t column, bool escaped) {
  if (column == 0 && *field == '\0') {
    // ignore empty line
    return;
  }

  addField(field, fieldLength, row, column, escaped);
  
  // we have now processed a complete line

  if (row > _rowsToSkip && _firstLine.isEmpty()) {
    // error
    MUTEX_LOCKER(guard, _stats._mutex);
    ++_stats._numberErrors;
    return;
  }
  
  if (_lineBuilder.isEmpty()) {
    if (row >= _rowsToSkip) {
      MUTEX_LOCKER(guard, _stats._mutex);
      ++_stats._numberErrors;
    }
    return;
  }

  if (_lineBuilder.isOpenArray()) {
    _lineBuilder.close();
  }

  if (row == _rowsToSkip && _firstLine.isEmpty()) {
    // save the first line values (headers)
    _firstLine = _lineBuilder;
    return;
  }
  
  arangodb::basics::VPackStringBufferAdapter adapter(_outputBuffer.stringBuffer());
  arangodb::velocypack::Dumper dumper(&adapter, &_options);

  if (_outputBuffer.empty()) {
    // append headers
    TRI_ASSERT(_firstLine.slice().isArray());
    dumper.dump(_firstLine.slice());
    _outputBuffer.appendChar('\n');
  }
    
  // append the actual line data
  TRI_ASSERT(_lineBuilder.slice().isArray());
  dumper.dump(_lineBuilder.slice());
  _outputBuffer.appendChar('\n');

  if (_outputBuffer.length() > getMaxUploadSize()) {
    sendCsvBuffer();
  }
}

bool ImportHelper::collectionExists() {
  std::string const url("/_api/collection/" + StringUtils::urlEncode(_collectionName));
  std::unique_ptr<SimpleHttpResult> result(
      _httpClient->request(rest::RequestType::GET, url, nullptr, 0));

  if (result == nullptr) {
    return false;
  }

  auto code = static_cast<rest::ResponseCode>(result->getHttpReturnCode());
  if (code == rest::ResponseCode::OK || code == rest::ResponseCode::CREATED ||
      code == rest::ResponseCode::ACCEPTED) {
    // collection already exists or was created successfully
    return true;
  }

  std::shared_ptr<velocypack::Builder> bodyBuilder(result->getBodyVelocyPack());
  velocypack::Slice error = bodyBuilder->slice();
  if (!error.isNone()) {
    auto errorNum = error.get(StaticStrings::ErrorNum).getUInt();
    auto errorMsg = error.get(StaticStrings::ErrorMessage).copyString();
    LOG_TOPIC("f2c4a", ERR, arangodb::Logger::FIXME)
        << "unable to access collection '" << _collectionName
        << "', server returned status code: " << static_cast<int>(code)
        << "; error [" << errorNum << "] " << errorMsg;
  } else {
    LOG_TOPIC("57d57", ERR, arangodb::Logger::FIXME)
        << "unable to accesss collection '" << _collectionName
        << "', server returned status code: " << static_cast<int>(code)
        << "; server returned message: "
        << Logger::CHARS(result->getBody().c_str(), result->getBody().length());
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief check if we must create the target collection, and create it if
/// required
////////////////////////////////////////////////////////////////////////////////

bool ImportHelper::checkCreateCollection() {
  if (!_createCollection) {
    return true;
  }

  std::string const url("/_api/collection");
  VPackBuilder builder;

  builder.openObject();
  builder.add(arangodb::StaticStrings::DataSourceName,
              arangodb::velocypack::Value(_collectionName));
  builder.add(arangodb::StaticStrings::DataSourceType,
              arangodb::velocypack::Value(_createCollectionType == "edge" ? 3 : 2));
  builder.close();

  std::string data = builder.slice().toJson();
  std::unique_ptr<SimpleHttpResult> result(
      _httpClient->request(rest::RequestType::POST, url, data.c_str(), data.size()));

  if (result == nullptr) {
    return false;
  }

  auto code = static_cast<rest::ResponseCode>(result->getHttpReturnCode());
  if (code == rest::ResponseCode::CONFLICT || code == rest::ResponseCode::OK ||
      code == rest::ResponseCode::CREATED || code == rest::ResponseCode::ACCEPTED) {
    // collection already exists or was created successfully
    return true;
  }

  std::shared_ptr<velocypack::Builder> bodyBuilder(result->getBodyVelocyPack());
  velocypack::Slice error = bodyBuilder->slice();
  if (!error.isNone()) {
    auto errorNum = error.get(StaticStrings::ErrorNum).getUInt();
    auto errorMsg = error.get(StaticStrings::ErrorMessage).copyString();
    LOG_TOPIC("09478", ERR, arangodb::Logger::FIXME)
        << "unable to create collection '" << _collectionName
        << "', server returned status code: " << static_cast<int>(code)
        << "; error [" << errorNum << "] " << errorMsg;
  } else {
    LOG_TOPIC("2211f", ERR, arangodb::Logger::FIXME)
        << "unable to create collection '" << _collectionName
        << "', server returned status code: " << static_cast<int>(code)
        << "; server returned message: "
        << Logger::CHARS(result->getBody().c_str(), result->getBody().length());
  }
  _hasError = true;
  return false;
}

bool ImportHelper::truncateCollection() {
  if (!_overwrite) {
    return true;
  }

  std::string const url = "/_api/collection/" + _collectionName + "/truncate";
  std::unique_ptr<SimpleHttpResult> result(
      _httpClient->request(rest::RequestType::PUT, url, "", 0));

  if (result == nullptr) {
    return false;
  }

  auto code = static_cast<rest::ResponseCode>(result->getHttpReturnCode());
  if (code == rest::ResponseCode::CONFLICT || code == rest::ResponseCode::OK ||
      code == rest::ResponseCode::CREATED || code == rest::ResponseCode::ACCEPTED) {
    // collection already exists or was created successfully
    return true;
  }

  LOG_TOPIC("f8ae4", ERR, arangodb::Logger::FIXME)
      << "unable to truncate collection '" << _collectionName
      << "', server returned status code: " << static_cast<int>(code);
  _hasError = true;
  _errorMessages.push_back("Unable to overwrite collection");
  return false;
}

void ImportHelper::sendCsvBuffer() {
  if (_hasError) {
    return;
  }

  std::string url("/_api/import?" + getCollectionUrlPart() +
                  "&line=" + StringUtils::itoa(_rowOffset) +
                  "&details=true&onDuplicate=" + StringUtils::urlEncode(_onDuplicateAction) +
                  "&ignoreMissing=" + (_ignoreMissing ? "true" : "false"));

  if (!_fromCollectionPrefix.empty()) {
    url += "&fromPrefix=" + StringUtils::urlEncode(_fromCollectionPrefix);
  }
  if (!_toCollectionPrefix.empty()) {
    url += "&toPrefix=" + StringUtils::urlEncode(_toCollectionPrefix);
  }
  if (_skipValidation) {
    url += "&"s + StaticStrings::SkipDocumentValidation + "=true";
  }
  if (_firstChunk && _overwrite) {
    truncateCollection();
  }
  _firstChunk = false;
 
  SenderThread* t = findIdleSender();
  if (t != nullptr) {
    uint64_t tmpLength = _outputBuffer.length();
    t->sendData(url, &_outputBuffer, _rowOffset + 1, _rowsRead);
    addPeriodByteCount(tmpLength + url.length());
  }
  
  _outputBuffer.reset();
  _rowOffset = _rowsRead;
}

void ImportHelper::sendJsonBuffer(char const* str, size_t len, bool isObject) {
  if (_hasError) {
    return;
  }

  // build target url
  std::string url("/_api/import?" + getCollectionUrlPart() +
                  "&details=true&onDuplicate=" + StringUtils::urlEncode(_onDuplicateAction));
  if (isObject) {
    url += "&type=array";
  } else {
    url += "&type=documents";
  }

  if (!_fromCollectionPrefix.empty()) {
    url += "&fromPrefix=" + StringUtils::urlEncode(_fromCollectionPrefix);
  }
  if (!_toCollectionPrefix.empty()) {
    url += "&toPrefix=" + StringUtils::urlEncode(_toCollectionPrefix);
  }
  if (_firstChunk && _overwrite) {
    // url += "&overwrite=true";
    truncateCollection();
  }
  if (_skipValidation) {
    url += "&"s + StaticStrings::SkipDocumentValidation + "=true";
  }

  _firstChunk = false;

  SenderThread* t = findIdleSender();
  if (t != nullptr) {
    _tempBuffer.reset();
    _tempBuffer.appendText(str, len);
    t->sendData(url, &_tempBuffer, _rowOffset + 1, _rowsRead);
    addPeriodByteCount(len + url.length());
  }
}

/// Should return an idle sender, collect all errors
/// and return nullptr, if there was an error
SenderThread* ImportHelper::findIdleSender() {
  if (_autoUploadSize) {
    _autoTuneThread->paceSends();
  } 

  while (!_senderThreads.empty()) {
    for (auto const& t : _senderThreads) {
      if (t->hasError()) {
        _hasError = true;
        _errorMessages.push_back(t->errorMessage());
        return nullptr;
      } else if (t->isIdle()) {
        return t.get();
      }
    }

    CONDITION_LOCKER(guard, _threadsCondition);
    guard.wait(10000);
  }
  return nullptr;
}

/// Busy wait for all sender threads to finish
void ImportHelper::waitForSenders() {
  while (!_senderThreads.empty()) {
    uint32_t numIdle = 0;
    for (auto const& t : _senderThreads) {
      if (t->isDone()) {
        if (t->hasError()) {
          _hasError = true;
          _errorMessages.push_back(t->errorMessage());
        }
        numIdle++;
      }
    }
    if (numIdle == _senderThreads.size()) {
      return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

}  // namespace 
