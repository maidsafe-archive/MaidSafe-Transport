/* Copyright (c) 2009 maidsafe.net limited
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

#include "maidsafe/transport/managed_connection_rpc.h"

namespace arg = std::placeholders;

namespace maidsafe {
  
namespace transport {

ManagedConnectionRpcs::ManagedConnectionRpcs() {
  private_key_.reset(new Asym::PrivateKey(crypto_key_pair_.private_key));
}

void ManagedConnectionRpcs::CreateConnection(TransportPtr transport, 
                                             const Endpoint &endpoint,
                                             const std::string &ref_id,
                                             CreateConnectionRpc callback) {
  MessageHandlerPtr message_handler;
  uint32_t index = Prepare(transport, message_handler);
  protobuf::ManagedConnectionInfoRequest request;
  request.set_identifier(ref_id);
  std::string message(message_handler->WrapMessage(request));
  Info info;
  info.endpoint = endpoint;
  // Connect callback to message handler for incoming parsed response or error
  message_handler->on_managed_connection_info_response()->connect(
      std::bind(&ManagedConnectionRpcs::ManagedConnectionInfoCallback, this,
                kSuccess, arg::_1, endpoint, arg::_2, ref_id, callback, index,
                endpoint));
  message_handler->on_error()->connect(
      std::bind(&ManagedConnectionRpcs::ManagedConnectionInfoCallback, this,
                arg::_1, info, arg::_2,
                protobuf::ManagedConnectionInfoResponse(), ref_id, callback,
                index, endpoint));
  transport->Send(message, endpoint, transport::kDefaultInitialTimeout);
}

void ManagedConnectionRpcs::ManagedConnectionInfoCallback(
    const TransportCondition &condition, const transport::Info &info,
    const Endpoint &remote_endpoint,
    const protobuf::ManagedConnectionInfoResponse& response,
    const std::string &ref_id, CreateConnectionRpc callback,
    const size_t &index, const Endpoint &endpoint) {
  if((remote_endpoint != endpoint) || (info.endpoint != endpoint))
    return;
  //  sending a managedconnection request message to the endpoint in response
  // received.
  if (condition == kSuccess) {
    Endpoint managed_endpoint;
    managed_endpoint.ip.from_string(response.endpoint().ip().c_str());
    managed_endpoint.port = static_cast<Port> (response.endpoint().port());
    MessageHandlerPtr message_handler(
      connected_objects_.GetMessageHandler(index));
    protobuf::ManagedConnectionRequest request;
    request.set_identifier(ref_id);
    std::string message(message_handler->WrapMessage(request));
    Info managed_ep_info;
    managed_ep_info.endpoint = managed_endpoint;
    // Connect callback to message handler for incoming parsed response or error
    message_handler->on_managed_connection_response()->connect(
        std::bind(&ManagedConnectionRpcs::CreateConnectionCallback, this,
                  kSuccess, arg::_1, managed_endpoint, arg::_2,
                  managed_endpoint, callback, index));
    message_handler->on_error()->connect(
        std::bind(&ManagedConnectionRpcs::CreateConnectionCallback, this,
                  arg::_1, managed_ep_info, arg::_2,
                  protobuf::ManagedConnectionResponse(), Endpoint(), callback,
                  index));
    connected_objects_.GetTransport(index)->Send(message, managed_endpoint,
        transport::kDefaultInitialTimeout);
  } else {
    callback(kError, Endpoint(), std::string(""));
  }
}

void ManagedConnectionRpcs::CreateConnectionCallback(
    const TransportCondition& condition, const transport::Info &info,
    const Endpoint &remote_endpoint,
    const protobuf::ManagedConnectionResponse &response,
    const Endpoint &endpoint, CreateConnectionRpc callback,
    const size_t &index) {
  if((remote_endpoint != endpoint) || (info.endpoint != endpoint))
    return;
//TODO (Prakash) check if all protobuf values exist????
  callback(response.result(), endpoint, response.identifier());
  connected_objects_.RemoveObject(index);
}

uint32_t ManagedConnectionRpcs::Prepare(TransportPtr &transport,
                                    MessageHandlerPtr &message_handler) {
  message_handler.reset(new MessageHandler(private_key_));
  // Connect message handler to transport for incoming raw messages
  transport->on_message_received()->connect(
      transport::OnMessageReceived::element_type::slot_type(
          &MessageHandler::OnMessageReceived, message_handler.get(),
          _1, _2, _3, _4).track_foreign(message_handler));
  transport->on_error()->connect(
      transport::OnError::element_type::slot_type(
          &MessageHandler::OnError, message_handler.get(),
          _1, _2).track_foreign(message_handler));
  return connected_objects_.AddObject(transport, message_handler);
}
  
}  // namesapace transport

}  // namespace maidsafe
