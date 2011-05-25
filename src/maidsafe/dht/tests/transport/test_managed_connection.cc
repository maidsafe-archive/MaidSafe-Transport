/* Copyright (c) 2010 maidsafe.net limited
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

#include <memory>
#include <string>
#include <vector>
#include <utility>
#include "gtest/gtest.h"
#include "gtest/gtest-param-test.h"
#include "boost/thread/mutex.hpp"
#include "boost/thread/thread.hpp"
#include "maidsafe/dht/transport/transport.h"
#include "maidsafe/dht/transport/tcp_transport.h"
#include "maidsafe/dht/transport/rudp/rudp_transport.h"
#include "maidsafe/dht/transport/managed_connection.h"
#include "maidsafe/dht/tests/transport/test_transport_api.h"

namespace maidsafe {

namespace dht {

namespace transport {

namespace test {

class MCTestMessageHandler : public TestMessageHandler {
 public:
  explicit MCTestMessageHandler(const std::string &id)
    : TestMessageHandler(id),
      shut_down_records_() {}

  void DoOnShutDownByPeer(const TransportCondition &tc) {
    shut_down_records_.push_back(tc);
  }

  Results shut_down_records() {
    return shut_down_records_;
  }

 private:
  MCTestMessageHandler(const MCTestMessageHandler&);
  MCTestMessageHandler& operator=(const MCTestMessageHandler&);

  Results shut_down_records_;
};

typedef boost::shared_ptr<MCTestMessageHandler> MCTestMessageHandlerPtr;

class RUDPManagedConnectionTest : public TransportAPITest<RudpTransport> {
 public:
  RUDPManagedConnectionTest()
    : managed_connections_(),
      senders_(),
      listening_ports_(),
      msgh_sender_(new MCTestMessageHandler("Sender")),
      msgh_listener_(new MCTestMessageHandler("listener")) {}
  ManagedConnectionMap managed_connections_;
  std::vector<boost::uint32_t> senders_;
  std::vector<boost::uint32_t> listening_ports_;
  MCTestMessageHandlerPtr msgh_sender_;
  MCTestMessageHandlerPtr msgh_listener_;

template <typename T>
void PrepareTransport(bool listen, size_t num_of_connections) {
  for (size_t i = 0; i < num_of_connections; ++i) {
    if (listen) {
      TransportPtr transport1;
        transport1 = TransportPtr(new T(*asio_service_));
      transport1->on_message_received()->connect(
          boost::bind(&MCTestMessageHandler::DoOnRequestReceived,
                      msgh_listener_, _1, _2, _3, _4));
      transport1->on_error()->connect(
          boost::bind(&MCTestMessageHandler::DoOnError, msgh_listener_, _1));
      boost::uint32_t port = managed_connections_.InsertConnection(transport1);
      EXPECT_EQ(kSuccess,
                transport1->StartListening(Endpoint(kIP, port)));
      listening_ports_.push_back(port);
    } else {
      TransportPtr transport1;
        transport1 = TransportPtr(new T(*asio_service_1_));
      transport1->on_message_received()->connect(
          boost::bind(&MCTestMessageHandler::DoOnResponseReceived,
                      msgh_sender_, _1, _2, _3, _4));
      transport1->on_error()->connect(
          boost::bind(&MCTestMessageHandler::DoOnError, msgh_sender_, _1));
      senders_.push_back(managed_connections_.InsertConnection(transport1));
    }
  }
}

template <typename T>
void PrepareConnection(size_t num_of_connections) {
  for (size_t i = 0; i < num_of_connections; ++i) {
    boost::uint32_t port1 = managed_connections_.NextEmptyPort();
    boost::uint32_t port2 = managed_connections_.NextEmptyPort();

    TransportPtr transport_listen;
      transport_listen = TransportPtr(new T(*asio_service_));
    transport_listen->on_message_received()->connect(
        boost::bind(&MCTestMessageHandler::DoOnRequestReceived,
                    msgh_listener_, _1, _2, _3, _4));
    managed_connections_.InsertConnection(transport_listen,
                                          Endpoint(kIP, port2),
                                          port1, true);
    EXPECT_EQ(kSuccess,
              transport_listen->StartListening(Endpoint(kIP, port1)));
    listening_ports_.push_back(port1);

    TransportPtr transport_send;
      transport_send = TransportPtr(new T(*asio_service_1_));
    transport_send->on_message_received()->connect(
        boost::bind(&MCTestMessageHandler::DoOnResponseReceived,
                    msgh_sender_, _1, _2, _3, _4));
    senders_.push_back(managed_connections_.InsertConnection(transport_send,
                                                        Endpoint(kIP, port1),
                                                        port2));
  }
}

};

TEST_F(RUDPManagedConnectionTest, BEH_TRANS_ErrorTransportType) {
  PrepareTransport<UdpTransport>(false, 1);
  EXPECT_EQ(-1, senders_[0]);
  PrepareTransport<TcpTransport>(false, 1);
  EXPECT_EQ(-1, senders_[1]);
  PrepareTransport<RudpTransport>(false, 1);
  EXPECT_NE(-1, senders_[2]);
}

TEST_F(RUDPManagedConnectionTest, BEH_TRANS_OneToManySingleMessage) {
  std::string request(RandomString(1));
  for (int i = 0; i < 4; ++i)
    request = request + request;

  // Prepare 10 listeners, one sender
  PrepareTransport<RudpTransport>(true, 10);
  PrepareTransport<RudpTransport>(false, 1);

  auto it = listening_ports_.begin();
  while (it != listening_ports_.end()) {
    managed_connections_.GetConnection(senders_[0])->Send(
        request, Endpoint(kIP, *it), bptime::seconds(4));
    ++it;
  }

  int waited_seconds(0);
  while (((msgh_listener_->requests_received().size() != 10) ||
          (msgh_sender_->responses_received().size() != 10)) &&
          (waited_seconds < 5)) {
    boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
    ++ waited_seconds;
  }
  EXPECT_EQ(size_t(10), msgh_listener_->requests_received().size());
  EXPECT_EQ(size_t(10), msgh_sender_->responses_received().size());
}

TEST_F(RUDPManagedConnectionTest, BEH_TRANS_PassiveDetectDroppedReceiver) {
  std::string request(RandomString(1));
  for (int i = 0; i < 26; ++i)
    request = request + request;

  // Prepare one pair of listeners and sender, i.e. one managed connection
  PrepareConnection<RudpTransport>(1);

  managed_connections_.GetConnection(senders_[0])->Send(
      request, Endpoint(kIP, listening_ports_[0]), bptime::seconds(26));

  int waited_seconds(0);
  while ((managed_connections_.IsConnected(senders_[0])) &&
         (waited_seconds < 10)) {
    boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
    ++waited_seconds;
    if (waited_seconds == 1)
        managed_connections_.GetConnection(
            listening_ports_[0])->StopListening();
  }
  managed_connections_.RemoveConnection(senders_[0]);
  boost::this_thread::sleep(boost::posix_time::milliseconds(10));
  EXPECT_GT(10, waited_seconds);
}

TEST_F(RUDPManagedConnectionTest, BEH_TRANS_OneToOneAliveMessage) {
  std::string request("Alive");

  // Prepare one listeners, one sender
  PrepareTransport<RudpTransport>(true, 1);
  PrepareTransport<RudpTransport>(false, 1);

  auto it = listening_ports_.begin();
  while (it != listening_ports_.end()) {
    managed_connections_.GetConnection(senders_[0])->Send(
        request, Endpoint(kIP, *it), kImmediateTimeout);
    ++it;
  }
  boost::this_thread::sleep(boost::posix_time::milliseconds(100));
  EXPECT_EQ(size_t(0), msgh_listener_->requests_received().size());
  EXPECT_EQ(size_t(0), msgh_sender_->responses_received().size());
}

TEST_F(RUDPManagedConnectionTest, BEH_TRANS_OneToOneAliveDetectReceiverDrop) {
  // Prepare one pair of listeners and sender, i.e. one managed connection
  PrepareConnection<RudpTransport>(1);
  // Start keep enquiring
  managed_connections_.StartMonitoring(MonitoringMode::kActive);

  int waited_seconds(0);
  while ((managed_connections_.IsConnected(senders_[0])) &&
         (waited_seconds < 10)) {
    boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
    ++waited_seconds;
    if (waited_seconds == 1)
        managed_connections_.GetConnection(
            listening_ports_[0])->StopListening();
  }
  EXPECT_GT(10, waited_seconds);
}

TEST_F(RUDPManagedConnectionTest, BEH_TRANS_MultipleAliveDetection) {
  // Prepare multiple pairs of listeners and sender,
  // i.e. multiple managed connections
//   int num_connection = 1015;  // max num that can be opened on udp
  int num_connection = 500; // max num that can be handled (no send timeout)
//   int num_connection = 510;
RudpParameters::kDefaultSendTimeOut = bptime::milliseconds(1000);
RudpParameters::kClientConnectTimeOut = bptime::milliseconds(1000);
  PrepareConnection<RudpTransport>(num_connection);
  // Start keep enquiring
  managed_connections_.StartMonitoring(MonitoringMode::kActive);

  int waited_seconds(0);
  std::vector<int> dropped_connections_index;
  int stopping;
  // Randomly drop five connections each second
  while (waited_seconds < 5) {
    boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
    ++waited_seconds;
    for (int i = 0; i < 5; ++i) {
      stopping = RandomUint32() % num_connection;
      auto it = std::find(dropped_connections_index.begin(),
                          dropped_connections_index.end(),
                          stopping);
      if (it != dropped_connections_index.end()) {
        managed_connections_.GetConnection(
            listening_ports_[stopping])->StopListening();
        dropped_connections_index.push_back(stopping);
      }
    }
  }
  // ensure the dropped connection will be detected, as the round-robbin only
  // works every one second per enquiry group and now having 5 groups,
  // means at least 5 seconds need to be slept to ensure each connection has
  // been enquired
  boost::this_thread::sleep(boost::posix_time::milliseconds(6500));
  for (int i = 0; i < num_connection; ++i) {
    auto it = std::find(dropped_connections_index.begin(),
                        dropped_connections_index.end(),
                        i);
    if (it == dropped_connections_index.end()) {
      EXPECT_TRUE(managed_connections_.IsConnected(senders_[i]));
    } else {
      EXPECT_FALSE(managed_connections_.IsConnected(senders_[i]));
    }
  }
}

}  // namespace test

}  // namespace transport

}  // namespace dht

}  // namespace maidsafe