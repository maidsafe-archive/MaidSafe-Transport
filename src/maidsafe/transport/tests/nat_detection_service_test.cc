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

#include "boost/thread.hpp"
#include "boost/thread/detail/thread_group.hpp"

#include "maidsafe/common/test.h"
#include "maidsafe/common/asio_service.h"

#include "maidsafe/transport/log.h"
#include "maidsafe/transport/nat_detection_service.h"
#include "maidsafe/transport/transport.h"
#include "maidsafe/transport/rudp_transport.h"
#include "maidsafe/transport/rudp_message_handler.h"
#include "maidsafe/transport/transport_pb.h"
#include "maidsafe/transport/nat_detection.h"
#include "maidsafe/transport/utils.h"
#include "maidsafe/transport/nat-detection/nat_detection_node.h"
#include "maidsafe/transport/nat-detection/node.h"

namespace args = std::placeholders;

namespace maidsafe {

namespace transport {

namespace test {


class MockNatDetectionService : public NatDetectionService {
 public:
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#endif
  MockNatDetectionService(boost::asio::io_service &io_service,  // NOLINT
                          RudpMessageHandlerPtr message_handler,
                          RudpTransportPtr listening_transport,
                          GetEndpointFunctor get_endpoint_functor)
      : NatDetectionService(io_service, message_handler,
                            listening_transport, get_endpoint_functor) {}
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
  MOCK_METHOD2(DirectlyConnected,
               bool(const protobuf::NatDetectionRequest &request,  // NOLINT (Fraser)
                    const Endpoint &endpoint));
  MOCK_METHOD4(ConnectResult, void(const int &in_result,
                                   int *out_result,
                                   boost::mutex *mutex,
                                   boost::condition_variable* condition));
  bool NotDirectlyConnected() {
    return false;
  }

  void ConnectResultFail(int *out_result,
                         boost::mutex *mutex,
                         boost::condition_variable* condition) {
    boost::mutex::scoped_lock lock(*mutex);
    *out_result = kError;
    condition->notify_one();
  }
};

class MockNatDetectionServiceTest : public testing::Test {
 public:
  MockNatDetectionServiceTest()
      :  origin_(new detection::Node),
         proxy_(new detection::Node),
         rendezvous_(new detection::Node) {}
  void ConnectToSignals(TransportPtr transport,
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
  ~MockNatDetectionServiceTest() {
    proxy_.reset();
    rendezvous_.reset();
    origin_.reset();
  }

 protected:
  detection::NodePtr origin_, proxy_, rendezvous_;
};

TEST_F(MockNatDetectionServiceTest, BEH_FullConeDetection) {
}

TEST_F(MockNatDetectionServiceTest, BEH_PortRestrictedDetection) {
}

}  // namespace test

}  // namespace transport

}  // namespace maidsafe
