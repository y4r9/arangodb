/*
* Currently only for debugging. An ID for a (semantic) network message (e.g.
* in this case stays the same over retries in the agency comm).
*
* TODO: I just noticed the name clashes with GeneralRequest::messageId().
*       One must be renamed or removed (the GeneralRequest messageId looks
*       unused at the first glance).
*/

#ifndef ARANGOD_NETWORK_MESSAGEID_H
#define ARANGOD_NETWORK_MESSAGEID_H

#include "Basics/Identifier.h"
#include "Basics/NumberUtils.h"
#include "VocBase/ticks.h"

#include <map>
#include <string>

namespace arangodb::network {

class MessageId : public basics::Identifier {
 public:
  using Identifier::Identifier;
  using Identifier::BaseType;
};

inline MessageId createNewMessageId() {
  return MessageId{TRI_NewServerSpecificTick()};
}

inline const std::string messageIdHeader = {"x-arango-messageid"};

inline const MessageId getMessageId(std::map<std::string, std::string> const& headers) {
  if (auto const it = headers.find(messageIdHeader); it != headers.end()) {
    auto const begin = it->second.c_str();
    auto const end = begin + it->second.size();
    bool valid;
    auto const idValue = NumberUtils::atoi<MessageId::BaseType>(begin, end, valid);
    if (valid) {
      return MessageId{idValue};
    } else {
      using namespace std::string_literals;
      throw std::logic_error(messageIdHeader + " is invalid: "s + it->second);
    }
  }
  return MessageId{};
}
inline const MessageId getMessageId(std::unordered_map<std::string, std::string> const& headers) {
  if (auto const it = headers.find(messageIdHeader); it != headers.end()) {
    auto const begin = it->second.c_str();
    auto const end = begin + it->second.size();
    bool valid;
    auto const idValue = NumberUtils::atoi<MessageId::BaseType>(begin, end, valid);
    if (valid) {
      return MessageId{idValue};
    } else {
      using namespace std::string_literals;
      throw std::logic_error(messageIdHeader + " is invalid: "s + it->second);
    }
  }
  return MessageId{};
}

}  // namespace arangodb::network

#endif  // ARANGOD_NETWORK_MESSAGEID_H
