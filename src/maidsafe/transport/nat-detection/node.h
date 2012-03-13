/* Copyright (c) 2011 maidsafe.net limited
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
    * Neither the name of the maidsafe.net limited nor the names of its
    contributors may be used to endorse or promote products derived from this
    software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef MAIDSAFE_TRANSPORT_NAT_DETECTION_NODE_H_
#define MAIDSAFE_TRANSPORT_NAT_DETECTION_NODE_H_

#include "maidsafe/common/utils.h"
#include "maidsafe/common/asio_service.h"
#include "maidsafe/transport/transport.h"


namespace maidsafe {

namespace transport {
  
class RudpTransport;
class RudpMessageHandler;
class NatDetectionService;

typedef std::shared_ptr<RudpTransport> RudpTransportPtr;
typedef std::shared_ptr<RudpMessageHandler> RudpMessageHandlerPtr;
typedef std::shared_ptr<NatDetectionService> NatDetectionServicePtr;

namespace detection {
  
class Node {
 public:
  Node();
  ~Node();
  bool StartListening();
  Endpoint endpoint();
  Contact live_contact();
  RudpTransportPtr transport() const;
  RudpMessageHandlerPtr message_handler() const;
  boost::asio::io_service& io_service();
  int16_t DetectNatType();
  bool ReadBootstrapFile(const fs::path& bootstrap);
  bool WriteBootstrapFile(const fs::path& bootstrap);
//  bool IsDirectlyConnected();

 private:
//  bool ExternalIpAddress(IP *external_ip);
  void ConnectToSignals(RudpTransportPtr transport,
                        RudpMessageHandlerPtr message_handler);

  AsioService asio_service_;
  Endpoint endpoint_;
  std::vector<Contact> live_contacts_;
  RudpTransportPtr transport_;
  RudpMessageHandlerPtr message_handler_;
  NatDetectionServicePtr service_;
};

typedef std::shared_ptr<Node> NodePtr;

} // detection

} // transport

} // maidsafe

#endif // MAIDSAFE_TRANSPORT_NAT_DETECTION_NODE_H_
