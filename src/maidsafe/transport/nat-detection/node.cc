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

#include "maidsafe/transport/nat-detection/node.h"

#include "maidsafe/transport/nat_detection_service.h"
#include "maidsafe/transport/rudp_transport.h"
#include "maidsafe/transport/rudp_message_handler.h"
#include "maidsafe/transport/utils.h"
#include "maidsafe/transport/upnp/upnp_client.h"
#include "maidsafe/transport/nat_detection.h"
#include <glog/log_severity.h>


namespace maidsafe {

namespace transport {

namespace detection {

Node::Node()
    : asio_service_(),
      endpoint_(GetLocalAddresses().at(0), 0),
      live_contacts_(),
      transport_(new transport::RudpTransport(asio_service_.service())),
      message_handler_(new RudpMessageHandler(nullptr)),
      service_(new NatDetectionService(asio_service_.service(),
            message_handler_, transport_,
            std::bind(&Node::live_contact, this))) {
  asio_service_.Start(5);
  service_->ConnectToSignals();
}

Node::~Node() {
  transport_->StopListening();
  transport_.reset();
  asio_service_.Stop();
}

bool Node::StartListening() {
  TransportCondition condition(kError);
  size_t max(5), attempt(0);
  while (attempt++ < max && (condition != kSuccess)) {
    endpoint_.port = RandomUint32() % (65535 - 1025) + 1025;
    condition = transport_->StartListening(endpoint_);
  }
  if (condition == kSuccess) {
    // Create contact_ information for node and set contact for Rpcs
    transport::Endpoint endpoint;
    endpoint.ip = transport_->transport_details().endpoint.ip;
    endpoint.port = transport_->transport_details().endpoint.port;
    transport_->transport_details_.local_endpoints.push_back(endpoint);
  }
  ConnectToSignals(transport_, message_handler_);
  return (condition == kSuccess);
}

Endpoint Node::endpoint() {
  return endpoint_;
}

Contact Node::live_contact() {
  return live_contacts_.at(RandomUint32() % live_contacts_.size());
}

RudpTransportPtr Node::transport() const {
  return transport_;
}

RudpMessageHandlerPtr Node::message_handler() const {
  return message_handler_;
}

boost::asio::io_service& Node::io_service() {
  return asio_service_.service();
}

// bool Node::SetLiveContacts(const fs::path& bootstrap) {
//   return ReadContactsFromFile(bootstrap, &live_contacts_);
// }

// bool Node::IsDirectlyConnected() {
//   IP external_ip;
//   std::vector<IP>  local_addresses(GetLocalAddresses());
//   if (!ExternalIpAddress(&external_ip))
//     return false;
//   for (auto it(local_addresses.begin()); it != local_addresses.end(); ++it) {
//     std::cout << (*it).to_string() << std::endl;
//     if (external_ip == (*it)) {
//       return true;
//     }
//   }
//   return false;
// }

// bool Node::ExternalIpAddress(IP *external_ip) {
//   upnp::UpnpIgdClient upnp_client;
//   bool result(upnp_client.InitControlPoint());
//   if (!result)
//     return false;
//   std::string ip(upnp_client.GetExternalIpAddress());
//   if (ip.empty())
//     return false;
//   std::cout << ip << std::endl;
//   *external_ip = IP::from_string(ip);
//   return true;
// }

int16_t Node::DetectNatType() {
  NatDetection nat_detection;
  NatType nat_type;
  Endpoint rendezvous_endpoint;
  nat_detection.Detect(live_contacts_, true, transport_, message_handler_,
                       &nat_type, &rendezvous_endpoint);
  return nat_type;
}

bool Node::ReadBootstrapFile(const fs::path& bootstrap) {
  return ReadContactsFromFile(bootstrap, &live_contacts_);
}

bool Node::WriteBootstrapFile(const fs::path& bootstrap) {
  std::vector<Contact> contacts;
  Contact contact(transport_->transport_details().endpoint,
      transport_->transport_details().local_endpoints,
      transport_->transport_details().rendezvous_endpoint, false, false);
  contacts.push_back(contact);
  return WriteContactsToFile(bootstrap, contacts);
}

void Node::ConnectToSignals(RudpTransportPtr transport,
    RudpMessageHandlerPtr message_handler) {
  transport->on_message_received()->connect(
  transport::OnMessageReceived::element_type::slot_type(
      &RudpMessageHandler::OnMessageReceived, message_handler.get(),
      _1, _2, _3, _4).track_foreign(message_handler));
  transport->on_error()->connect(
      transport::OnError::element_type::slot_type(
          &RudpMessageHandler::OnError,
          message_handler.get(), _1, _2).track_foreign(message_handler));
}

} // detection

} // transport

} // maidsafe
