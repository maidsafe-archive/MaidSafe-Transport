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

#include "maidsafe/transport/tcp_transport.h"
#include "maidsafe/transport/rudp_transport.h"
#include "maidsafe/transport/udp_transport.h"

#include "boost/thread/condition_variable.hpp"

#include "maidsafe/common/asio_service.h"
#include "maidsafe/common/test.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/transport/log.h"

namespace maidsafe {

namespace transport {

namespace test {

namespace {

static const IP kIP(boost::asio::ip::address_v4::loopback());
static const uint16_t kThreadCount = 8;
static const uint16_t kParallelRPC = 8;

class TestMessageHandler {
 public:
  explicit TestMessageHandler(const std::string &id)
      : this_id_(id),
        request_recvd_count_(0),
        response_recvd_count_(0),
        error_count_(0),
        mutex_(),
        done_(),
        total_count_expectation_(0),
        result_mutex_(),
        cond_var_() {}

void DoOnRequestReceived(const std::string &request,
                         const Info &/*info*/,
                         std::string *response,
                         Timeout *timeout) {
  Sleep(boost::posix_time::milliseconds(10));
  {
    boost::mutex::scoped_lock lock(mutex_);
    *response = "Replied to " + request.substr(0, 10);
    *timeout = kImmediateTimeout;
    DLOG(INFO) << this_id_ << " - Received request: \""
               << request.substr(0, 10)
               << "\".  Responding with \"" << *response << "\"";
    ++request_recvd_count_;
  }
}

void DoOnResponseReceived(const std::string &request, const Info &/*info*/,
                          std::string *response, Timeout *timeout) {
  response->clear();
  *timeout = kImmediateTimeout;
  boost::mutex::scoped_lock lock(mutex_);
  DLOG(INFO) << this_id_ << " - Received response: \""
             << request.substr(0, 10) << "\"";
  ++response_recvd_count_;
  NotifyIfDone();
}

void DoOnError(const TransportCondition &tc) {
  boost::mutex::scoped_lock lock(mutex_);
  ++error_count_;
  DLOG(INFO) << this_id_ << " - Error: " << tc << " count - " << error_count_;
  NotifyIfDone();
}

uint32_t request_recvd_count() {
  boost::mutex::scoped_lock lock(mutex_);
  return request_recvd_count_;
}

uint32_t response_recvd_count() {
  boost::mutex::scoped_lock lock(mutex_);
  return response_recvd_count_;
}

uint32_t error_count() {
  boost::mutex::scoped_lock lock(mutex_);
  return error_count_;
}

void SetUpCompletionTrigger(uint32_t count, bool *done, boost::mutex *mutex,
                            boost::condition_variable *cond) {
  total_count_expectation_ = count;
  done_ = done;
  result_mutex_ = mutex;
  cond_var_ = cond;
}

void reset() {
  request_recvd_count_ = 0;
  response_recvd_count_ = 0;
  error_count_ = 0;
  result_mutex_ = nullptr;
  total_count_expectation_ = 0;
  cond_var_ = nullptr;
}

 protected:
  void NotifyIfDone() {
    if (result_mutex_ && cond_var_) {
      boost::mutex::scoped_lock lock(*result_mutex_);
      if ((error_count_ + response_recvd_count_) >= total_count_expectation_) {
        cond_var_->notify_one();
        *done_ = true;
      }
    }
  }

 private:
  TestMessageHandler(const TestMessageHandler&);
  TestMessageHandler &operator=(const TestMessageHandler&);

  std::string this_id_;
  uint32_t request_recvd_count_;
  uint32_t response_recvd_count_;
  uint32_t error_count_;
  boost::mutex mutex_;
  bool * done_;
  uint32_t total_count_expectation_;
  boost::mutex *result_mutex_;
  boost::condition_variable * cond_var_;
};

}  // anonymous namespace

template <typename T>
class TransportStressTest : public testing::Test {
 public:
  TransportStressTest()
      : asio_service_listener_(),
        asio_service_sender_(),
        asio_service_test_(),
        mutex_(),
        rpc_response_count_(0),
        rpc_error_count_(0),
        parallel_rpcs_(0) {
    asio_service_listener_.Start(8);
    asio_service_sender_.Start(8);
    asio_service_test_.Start(8);
  }

  virtual ~TransportStressTest() {
    DLOG(INFO) << "~TransportStressTest!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*****************************";
  }

  void RPC(Endpoint listener_endpoint, const std::string &messages) {
    {
      boost::mutex::scoped_lock lock(mutex_);
      ++parallel_rpcs_;
    }
    boost::mutex rpc_mutex;
    boost::condition_variable rpc_cond;
    bool done(false);
    TestMessageHandler sender_msg_handler("sender");
    TransportPtr sender(new T(asio_service_sender_.service()));
    sender->on_message_received()->connect(boost::bind(
        &TestMessageHandler::DoOnResponseReceived, &sender_msg_handler, _1, _2,
        _3, _4));
    sender->on_error()->connect(
        boost::bind(&TestMessageHandler::DoOnError, &sender_msg_handler, _1));
    // Seting up completion trigger on condition variable
    sender_msg_handler.SetUpCompletionTrigger(1, &done, &rpc_mutex, &rpc_cond);
    // Send
    sender->Send(messages, listener_endpoint, kDefaultInitialTimeout);
    // Waiting for kDefaultInitialTimeout
    {
      boost::mutex::scoped_lock lock(rpc_mutex);
      EXPECT_TRUE(rpc_cond.timed_wait(lock, kDefaultInitialTimeout,
                                      [&]() {
                                              return done;
                                            }));  // NOLINT
    }
    if (1 == sender_msg_handler.response_recvd_count()) {
      EXPECT_EQ(0, sender_msg_handler.error_count());
      boost::mutex::scoped_lock lock(mutex_);
      ++rpc_response_count_;
      --parallel_rpcs_;
    } else if (1 == sender_msg_handler.error_count()) {
      boost::mutex::scoped_lock lock(mutex_);
      ++ rpc_error_count_;
      --parallel_rpcs_;
    }
  }

  void RPCFlooding(uint32_t rpc_count, size_t message_size) {
    std::string messages(message_size, 'A');
    //  Creating transport
    TransportPtr listener(new T(asio_service_listener_.service()));
    Endpoint endpoint(kIP, 0);
    TransportCondition result(kError);
    for (uint16_t port(8000); port != 9000; ++port) {
      endpoint.port = port;
      result = listener->StartListening(endpoint);
      if (transport::kSuccess == result) {
        break;
      } else {
        listener->StopListening();
      }
    }
    //  Connecting signals
    TestMessageHandler listener_msg_handler("listener");
    listener->on_message_received()->connect(boost::bind(
        &TestMessageHandler::DoOnRequestReceived, &listener_msg_handler, _1,
        _2, _3, _4));
    listener->on_error()->connect(
        boost::bind(&TestMessageHandler::DoOnError, &listener_msg_handler, _1));

    Endpoint listener_endpoint(listener->transport_details().endpoint);
    std::string message(message_size, 'C');
    // Sending RPCs
    for (uint32_t i(0); i != rpc_count; ++i) {
      auto rpc = std::bind(&TransportStressTest::RPC, this, listener_endpoint,
                           message);
      asio_service_test_.service().dispatch(rpc);
      while (parallel_rpcs_ > kParallelRPC) {
        boost::this_thread::sleep(boost::posix_time::milliseconds(100));
      }
    }

    //// Waiting for results
    while (parallel_rpcs_ != 0) {
        boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    }

    DLOG(INFO) << "Sender - count of total rpcs : " << rpc_count;
    DLOG(INFO) << "Sender - count of successfull response : "
               << rpc_response_count_;
    DLOG(INFO) << "Sender - count of failed rpcs : "
               << rpc_error_count_;

    // Failure Expectation
    EXPECT_EQ(rpc_count, rpc_response_count_ + rpc_error_count_);
    listener->StopListening();
    DLOG(INFO) << "~TransportStressTest!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!********BEFORE LISTENER STOP*********";
    asio_service_listener_.Stop();
    DLOG(INFO) << "~TransportStressTest!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*******AFTER LISTENER STOP*********";
    asio_service_sender_.Stop();
    DLOG(INFO) << "~TransportStressTest!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!********AFTER SENDER STOP*********";
    asio_service_test_.Stop();
    DLOG(INFO) << "~TransportStressTest!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*******AFTER STOP*********";
  }

 protected:
  void NotifyIfDone() {
    boost::mutex::scoped_lock lock(*result_mutex_);
    if ((rpc_response_count_ + rpc_error_count_) >= total_count_expectation_) {
      done_ = true;
      cond_var_.notify_one();
    }
  }

 private:
  AsioService asio_service_listener_;
  AsioService asio_service_sender_;
  AsioService asio_service_test_;
  boost::mutex mutex_;
  uint16_t rpc_response_count_;
  uint16_t rpc_error_count_;
  uint16_t parallel_rpcs_;
};

TYPED_TEST_CASE_P(TransportStressTest);

TYPED_TEST_P(TransportStressTest, FUNC_RPCFlooding1MBMessage) {
  this->RPCFlooding(1000, 1048576);
}

TYPED_TEST_P(TransportStressTest, FUNC_RPCFloodingSmallMessage) {
  this->RPCFlooding(1000, 10);
}

TYPED_TEST_P(TransportStressTest, FUNC_RPCFlooding256KBMessage) {
  this->RPCFlooding(1000, 262144);
}
REGISTER_TYPED_TEST_CASE_P(TransportStressTest,
                           FUNC_RPCFlooding1MBMessage,
                           FUNC_RPCFloodingSmallMessage,
                           FUNC_RPCFlooding256KBMessage);


INSTANTIATE_TYPED_TEST_CASE_P(TCP, TransportStressTest, TcpTransport);
INSTANTIATE_TYPED_TEST_CASE_P(RUDP, TransportStressTest, RudpTransport);
INSTANTIATE_TYPED_TEST_CASE_P(UDP, TransportStressTest, UdpTransport);

}  // namespace test

}  // namespace transport

}  // namespace maidsafe
