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

#ifndef MAIDSAFE_TRANSPORT_MANAGED_CONNECTION_RPC_H_
#define MAIDSAFE_TRANSPORT_MANAGED_CONNECTION_RPC_H_

#include<string>

#include "maidsafe/transport/transport.h"
#include "maidsafe/transport/transport_pb.h"
#include "maidsafe/transport/rpc_objects.h"
#include "maidsafe/transport/rudp_message_handler.h"

namespace maidsafe {

namespace transport {

typedef std::function<void(const int32_t&, const Endpoint&, const std::string&)>
    CreateConnectionRpc;

class ManagedConnectionRpcs {
 public:
  ManagedConnectionRpcs();
  void CreateConnection(TransportPtr transport,
                        const Endpoint &endpoint,
                        const std::string &ref_id,
                        CreateConnectionRpc callback);
  void ManagedConnectionInfoCallback(const TransportCondition &condition,
      const transport::Info &info,
      const Endpoint &remote_endpoint,
      const protobuf::ManagedConnectionInfoResponse &response,
      const std::string &ref_id,
      CreateConnectionRpc callback,
      const size_t &index,  const Endpoint &endpoint);
  void CreateConnectionCallback(const TransportCondition& condition,
      const transport::Info &info,
      const Endpoint &remote_endpoint,
      const protobuf::ManagedConnectionResponse &response,
      const Endpoint &endpoint, CreateConnectionRpc callback,
      const size_t &index);

 private:
  uint32_t Prepare(TransportPtr transport,
                   RudpMessageHandlerPtr message_handler);

  ConnectedObjectsList connected_objects_;
  static Asym::Keys crypto_key_pair_;
  std::shared_ptr<Asym::PrivateKey> private_key_;
};
}  // namesapace transport

}  // namespace maidsafe

#endif  // MAIDSAFE_TRANSPORT_MANAGED_CONNECTION_RPC_H_

