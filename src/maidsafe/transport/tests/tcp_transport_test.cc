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

class TcpTestMessageHandler {
 public:
  explicit TcpTestMessageHandler(const std::string &id)
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
  TcpTestMessageHandler(const TcpTestMessageHandler&);
  TcpTestMessageHandler &operator=(const TcpTestMessageHandler&);

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

void SendFlooding(size_t message_count, size_t message_size) {
  std::string messages(message_size, 'A');
  boost::mutex mutex;
  boost::condition_variable cond;
  bool done(false);

  AsioService asio_service_sender;
  asio_service_sender.Start(20);
  AsioService asio_service_listener;
  asio_service_listener.Start(20);

  //  Creating transports
  TransportPtr sender(new TcpTransport(asio_service_sender.service()));
  TransportPtr listener(new TcpTransport(asio_service_listener.service()));
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
  TcpTestMessageHandler listener_msg_handler("listener");
  TcpTestMessageHandler sender_msg_handler("sender");

  sender->on_message_received()->connect(boost::bind(
      &TcpTestMessageHandler::DoOnResponseReceived, &sender_msg_handler, _1, _2,
      _3, _4));
  sender->on_error()->connect(
      boost::bind(&TcpTestMessageHandler::DoOnError, &sender_msg_handler, _1));

  listener->on_message_received()->connect(boost::bind(
      &TcpTestMessageHandler::DoOnRequestReceived, &listener_msg_handler, _1,
      _2, _3, _4));
  listener->on_error()->connect(
      boost::bind(&TcpTestMessageHandler::DoOnError, &listener_msg_handler,
      _1));

  //  Seting up completion trigger on condition variable
  sender_msg_handler.SetUpCompletionTrigger(message_count, &done, &mutex,
                                            &cond);

  Endpoint listener_endpoint(listener->transport_details().endpoint);

  // Sending messages
  for (uint32_t i(0); i != message_count; ++i) {
    sender->Send(messages, listener_endpoint, bptime::seconds(10));
  }
  //  Waiting for 1 minute
  {
    boost::mutex::scoped_lock lock(mutex);
    EXPECT_TRUE(cond.timed_wait(lock, bptime::minutes(1), [&]() {
                                return done;
                                }));  // NOLINT
  }

  DLOG(INFO) << "Sender - count of total send : " << message_count;
  DLOG(INFO) << "Sender - count of successfull send : "
             << sender_msg_handler.response_recvd_count();
  DLOG(INFO) << "Sender - count of failed send : "
             << sender_msg_handler.error_count();

  // Failure Expectation
  EXPECT_EQ(message_count, sender_msg_handler.response_recvd_count() +
            sender_msg_handler.error_count());

  asio_service_sender.Stop();
  asio_service_listener.Stop();
}

}  // anonymous namespace

TEST(TcpTransportTest, FUNC_SendFlooding1MBMessage) {
  SendFlooding(1000, 1048576);
}

TEST(TcpTransportTest, FUNC_SendFloodingSmallMessage) {
  SendFlooding(1000, 10);
}

TEST(TcpTransportTest, FUNC_SendFlooding256KBMessage) {
  SendFlooding(1000, 262144);
}

}  // namespace test

}  // namespace transport

}  // namespace maidsafe
