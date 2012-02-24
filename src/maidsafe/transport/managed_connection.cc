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

#include "maidsafe/transport/managed_connection.h"

#include "maidsafe/transport/rudp_connection.h"
#include "maidsafe/transport/rudp_transport.h"
#include "maidsafe/transport/transport_pool.h"

namespace arg = std::placeholders;

namespace maidsafe {

namespace transport {


ManagedConnectionMap::ManagedConnectionMap(boost::asio::io_service &io_service, //NOLINT
                                           const std::string &ref_id,
                                           TransportPoolPtr transport_pool)
    : asio_service_(io_service),
      ref_id_(ref_id),
      connections_container_(new ManagedConnectionContainer),
      transport_pool_(transport_pool),
      notify_down_by_peer_id_(),
      notify_down_by_ref_id_(),
      notify_down_by_connection_id_(),
      shared_mutex_(),
      mutex_(),
      monitoring_mode_(kPassive),
      index_(10000),
      condition_enquiry_(),
      thread_group_(),
      enquiry_index(0),
      rpcs_(new ManagedConnectionRpcs()) {}

ManagedConnectionMap::~ManagedConnectionMap() {
  monitoring_mode_ = kPassive;
  connections_container_->clear();
  if (thread_group_)  {
    thread_group_->interrupt_all();
    thread_group_->join_all();
    thread_group_.reset();
  }
}

void ManagedConnectionMap::CreateConnection(const Endpoint &peer,
    CreateConnectionFunctor callback) {
  std::string peer_id(ToString(peer));
  if (IsConnectedByPeerId(peer_id)) {
    callback(-5);  // TODO(Prakash): Define error codes for Already connected
    return;
  }
  RudpTransportPtr transport(transport_pool_->GetAvailableTransport());
  rpcs_->CreateConnection(transport, peer, ref_id_,
      std::bind(&ManagedConnectionMap::ManagedConnectionReqCallback, this,
                arg::_1, arg::_2, arg::_3, transport, callback));
}

void ManagedConnectionMap::ManagedConnectionReqCallback(
      const uint32_t & /*transport_condition*/, const Endpoint &endpoint,
      const std::string &reference_id, RudpTransportPtr transport,
      CreateConnectionFunctor callback) {
  if (endpoint.ip == IP())
    callback(-1);  // TODO(Prakash): Define error codes
  RudpConnectionPtr connection(transport->GetConnection(endpoint));
  if (connection != RudpConnectionPtr()) {
    connection->SetManaged(true);
    InsertConnection(connection, endpoint, reference_id,
                     GenerateConnectionID());
    transport->RemoveConnection(connection);
    callback(kSuccess);
  } else {
    callback(-2);
  }
}

boost::int32_t ManagedConnectionMap::InsertIncomingConnection(
    const Endpoint &peer,
    const std::string &reference_id) {
  RudpConnectionPtr connection(transport_pool_->GetConnection(peer));
  if (connection && peer != Endpoint() && reference_id.empty())
    return InsertConnection(connection, peer, reference_id,
                            GenerateConnectionID());
  else
    return -1;
}

// boost::int32_t ManagedConnectionMap::InsertConnection(
//    const RudpConnectionPtr connection,
//    const std::string &reference_id) {
//  //if (transport->transport_type() != kRUDP)
//  //  return kError;
//  IP localIP(boost::asio::ip::address_v4::loopback());
//  boost::uint16_t peer_port = static_cast<uint16_t>(GenerateConnectionID());
//  Endpoint peer(localIP, peer_port);
//  std::stringstream out;
//  out << peer_port;
//  std::string peer_id = peer.ip.to_string() + ":" + out.str();
//
//  UniqueLock unique_lock(shared_mutex_);
//  ManagedConnectionContainer::index<TagConnectionId>::type&
//      index_by_connection_id = connections_container_->get<TagConnectionId>();
//
//  // For managed connections, the error handling shall be always in the charge
//  // of ManagedConnectionMap
//  // TODO(Prakash) : connecting signal should go where we wre initialising
//  //  transport in the pool
//  //transport->on_error()->connect(
//        transport::OnError::element_type::slot_type(
//  //    &ManagedConnectionMap::DoOnConnectionError, this, _1, _2));
//  ManagedConnection mc(connection, peer, peer_id, reference_id, peer_port);
//  index_by_connection_id.insert(mc);
//  return mc.connectionid;
// }

boost::int32_t ManagedConnectionMap::InsertConnection(
    const RudpConnectionPtr connection,
    const Endpoint &peer,
    const std::string &reference_id,
    const boost::uint32_t &connection_id) {
//  if (transport->transport_type() != kRUDP)
//  return -1;
  std::string peer_id(ToString(peer));
  if (peer_id.empty())
    return -1;
  if ((HasPeerId(peer_id)) || (HasConnectionId(connection_id)))
    return -3;  // Error code indicating peerid or connection_id exists

  UniqueLock unique_lock(shared_mutex_);
  ManagedConnectionContainer::index<TagConnectionId>::type&
      index_by_connection_id = connections_container_->get<TagConnectionId>();

  // For managed connections, the error handling shall be always handled by the
  // ManagedConnectionMap
  ManagedConnection mc(connection, peer, peer_id, reference_id, connection_id);
  index_by_connection_id.insert(mc);
  return mc.connectionid;
}

OnMessageReceived ManagedConnectionMap::on_message_received(
    const std::string &reference_id) {
  if (reference_id.empty())
    return OnMessageReceived();
  {
    SharedLock shared_lock(shared_mutex_);
    ManagedConnectionContainer::index<TagReferenceId>::type&
        index_by_reference_id = connections_container_->get<TagReferenceId>();
    auto it = index_by_reference_id.find(reference_id);
    if (it != index_by_reference_id.end()) {
      if (RudpTransportPtr transport =
            ((*it).connection_ptr->transport_.lock()))
        return transport->on_message_received();
    }
    return OnMessageReceived();
  }
}

OnError ManagedConnectionMap::on_error(const std::string &reference_id) {
  if (reference_id.empty())
    return OnError();
  {
    SharedLock shared_lock(shared_mutex_);
    ManagedConnectionContainer::index<TagReferenceId>::type&
        index_by_reference_id = connections_container_->get<TagReferenceId>();
    auto it = index_by_reference_id.find(reference_id);
    if (it != index_by_reference_id.end()) {
      if (RudpTransportPtr transport =
          ((*it).connection_ptr->transport_.lock()))
        return transport->on_error();
    }
    return OnError();
  }
}

void ManagedConnectionMap::DoOnConnectionError(
    const TransportCondition &error, const Endpoint peer) {
  UpgradeLock upgrade_lock(shared_mutex_);
  std::stringstream out;
  out << peer.port;
  std::string peer_id = peer.ip.to_string() + ":" + out.str();
  ManagedConnectionContainer::index<TagConnectionPeerId>::type&
      index_by_peer_id = connections_container_->get<TagConnectionPeerId>();
  auto it = index_by_peer_id.find(peer_id);
  if ((it != index_by_peer_id.end()) && ((*it).is_connected)) {
    UpgradeToUniqueLock upgrade_unique_lock(upgrade_lock);
    index_by_peer_id.modify(it, ChangeConnectionStatus(false));
  }
  (*notify_down_by_ref_id_)(error, (*it).reference_id);
  (*notify_down_by_peer_id_)(error, peer_id);
}

boost::uint16_t ManagedConnectionMap::GenerateConnectionID() {
  ManagedConnectionContainer::index<TagConnectionId>::type&
      index_by_connection_id = connections_container_->get<TagConnectionId>();
  ++index_;
  auto it = index_by_connection_id.find(index_);
  while (it != index_by_connection_id.end()) {
    ++index_;
    if (index_ > 0x7fffffff)
      index_ = 1501;
    it = index_by_connection_id.find(index_);
  }
  return index_;
}

bool ManagedConnectionMap::HasPeerId(const std::string& peer_id) {
  if (peer_id.empty())
    return false;
  SharedLock shared_lock(shared_mutex_);
  ManagedConnectionContainer::index<TagConnectionPeerId>::type&
      index_by_peer_id = connections_container_->get<TagConnectionPeerId>();
  auto it = index_by_peer_id.find(peer_id);
  if (it == index_by_peer_id.end())
    return false;
  return true;
}

bool ManagedConnectionMap::HasReferenceId(const std::string& reference_id) {
  if (reference_id.empty())
    return false;
  SharedLock shared_lock(shared_mutex_);
  ManagedConnectionContainer::index<TagReferenceId>::type&
      index_by_reference_id = connections_container_->get<TagReferenceId>();
  auto it = index_by_reference_id.find(reference_id);
  if (it == index_by_reference_id.end())
    return false;
  return true;
}

bool ManagedConnectionMap::HasConnectionId(const boost::uint32_t &index) {
  if (index < 1501)
    return true;
  SharedLock shared_lock(shared_mutex_);
  ManagedConnectionContainer::index<TagConnectionId>::type&
      index_by_connection_id = connections_container_->get<TagConnectionId>();
  auto it = index_by_connection_id.find(index);
  if (it == index_by_connection_id.end()) {
    return false;
  } else {
    return true;
  }
}

bool ManagedConnectionMap::RemoveConnection(const boost::uint32_t &index) {
  UpgradeLock upgrade_lock(shared_mutex_);
  ManagedConnectionContainer::index<TagConnectionId>::type&
      index_by_connection_id = connections_container_->get<TagConnectionId>();
  auto it = index_by_connection_id.find(index);
  if (it == index_by_connection_id.end())
    return false;
  UpgradeToUniqueLock unique_lock(upgrade_lock);
  // Remove the entry from multi index
  index_by_connection_id.erase(it);
  return true;
}

bool ManagedConnectionMap::RemoveConnectionByPeerId(
    const std::string &peer_id) {
  if (peer_id.empty())
    return false;
  UpgradeLock upgrade_lock(shared_mutex_);
  ManagedConnectionContainer::index<TagConnectionPeerId>::type&
      index_by_peer_id = connections_container_->get<TagConnectionPeerId>();
  auto it = index_by_peer_id.find(peer_id);
  if (it == index_by_peer_id.end())
    return false;
  UpgradeToUniqueLock unique_lock(upgrade_lock);
  // Remove the entry from multi index
  index_by_peer_id.erase(it);
  return true;
}

bool ManagedConnectionMap::RemoveConnection(const std::string &reference_id) {
  if (reference_id.empty())
    return false;
  UpgradeLock upgrade_lock(shared_mutex_);
  ManagedConnectionContainer::index<TagReferenceId>::type&
      index_by_reference_id = connections_container_->get<TagReferenceId>();
  auto it = index_by_reference_id.find(reference_id);
  if (it == index_by_reference_id.end())
    return false;
  UpgradeToUniqueLock unique_lock(upgrade_lock);
  // Remove the entry from multi index
  index_by_reference_id.erase(it);
  return true;
}

RudpConnectionPtr ManagedConnectionMap::GetConnectionByPeerId(
    const std::string &peer_id) {
  if (peer_id.empty())
    return RudpConnectionPtr();
  SharedLock shared_lock(shared_mutex_);
  ManagedConnectionContainer::index<TagConnectionPeerId>::type&
      index_by_peer_id = connections_container_->get<TagConnectionPeerId>();
  auto it = index_by_peer_id.find(peer_id);
  if (it == index_by_peer_id.end())
    return RudpConnectionPtr();
  return (*it).connection_ptr;
}

RudpConnectionPtr ManagedConnectionMap::GetConnection(
    const std::string &reference_id) {
  if (reference_id.empty())
    return RudpConnectionPtr();
  SharedLock shared_lock(shared_mutex_);
  ManagedConnectionContainer::index<TagReferenceId>::type&
      index_by_reference_id = connections_container_->get<TagReferenceId>();
  auto it = index_by_reference_id.find(reference_id);
  if (it == index_by_reference_id.end())
    return RudpConnectionPtr();
  return (*it).connection_ptr;
}

RudpConnectionPtr ManagedConnectionMap::GetConnection(
    const boost::uint32_t &connection_id) {
  SharedLock shared_lock(shared_mutex_);
  ManagedConnectionContainer::index<TagConnectionId>::type&
      index_by_connection_id = connections_container_->get<TagConnectionId>();
  auto it = index_by_connection_id.find(connection_id);
  if (it == index_by_connection_id.end())
    return RudpConnectionPtr();
  return (*it).connection_ptr;
}

std::string ManagedConnectionMap::ToString(const Endpoint &ep) {
  std::stringstream out;
  out << ep.port;
  std::string peer_id = ep.ip.to_string() + ":" + out.str();
  return peer_id;
}

bool ManagedConnectionMap::IsConnectedByPeerId(const std::string &peer_id) {
  if (peer_id.empty())
    return false;
  SharedLock shared_lock(shared_mutex_);
  ManagedConnectionContainer::index<TagConnectionPeerId>::type&
      index_by_peer_id = connections_container_->get<TagConnectionPeerId>();
  auto it = index_by_peer_id.find(peer_id);
  if (it == index_by_peer_id.end())
    return false;
  return (*it).is_connected;
}

bool ManagedConnectionMap::IsConnected(const std::string &reference_id) {
  if (reference_id.empty())
    return false;
  SharedLock shared_lock(shared_mutex_);
  ManagedConnectionContainer::index<TagReferenceId>::type&
      index_by_reference_id = connections_container_->get<TagReferenceId>();
  auto it = index_by_reference_id.find(reference_id);
  if (it == index_by_reference_id.end())
    return false;
  return (*it).is_connected;
}

bool ManagedConnectionMap::IsConnected(const boost::uint32_t &connection_id) {
  SharedLock shared_lock(shared_mutex_);
  ManagedConnectionContainer::index<TagConnectionId>::type&
      index_by_connection_id = connections_container_->get<TagConnectionId>();
  auto it = index_by_connection_id.find(connection_id);
  if (it == index_by_connection_id.end())
    return false;
  return (*it).is_connected;
}

bool ManagedConnectionMap::Send(const std::string &data,
                                const Endpoint &endpoint,
                                const Timeout &timeout) {
  std::string peer_id(ToString(endpoint));
  if (peer_id.empty())
    return false;
  {
    SharedLock shared_lock(shared_mutex_);
    ManagedConnectionContainer::index<TagConnectionPeerId>::type&
        index_by_peer_id = connections_container_->get<TagConnectionPeerId>();
    auto it = index_by_peer_id.find(peer_id);
    if (it == index_by_peer_id.end())
      return false;
    if (std::shared_ptr<RudpTransport> transport =
        (*it).connection_ptr->transport_.lock()) {
      transport->Send(data, (*it).connection_ptr, timeout);
      return true;
    } else {
      return false;
    }
  }
}

bool ManagedConnectionMap::Send(const std::string &data,
                                const boost::uint32_t &connection_id,
                                const Timeout &timeout) {
  if (connection_id == 0)
    return false;
  {
    SharedLock shared_lock(shared_mutex_);
    ManagedConnectionContainer::index<TagConnectionId>::type&
        index_by_connection_id =
            connections_container_->get<TagConnectionId>();
    auto it = index_by_connection_id.find(connection_id);
    if (it == index_by_connection_id.end())
      return false;
    if (std::shared_ptr<RudpTransport> transport =
        (*it).connection_ptr->transport_.lock()) {
      transport->Send(data, (*it).connection_ptr, timeout);
      return true;
    } else {
      return false;
    }
  }
}

void ManagedConnectionMap::AliveEnquiryThread() {
  boost::mutex::scoped_lock loch_surlaplage(mutex_);
  while (monitoring_mode_ == kActive) {
    condition_enquiry_.wait(loch_surlaplage);

    auto it = connections_container_->get<TagConnectionEnquiryGroup>().
                  equal_range(enquiry_index);
    while (it.first != it.second) {
      // TODO(qi.ma@maidsafe.net) : make the client support listen on socket
      // as well
      // if (((*it.first).is_connected) && ((*it.first).is_client))
      //  (*it.first).transport_ptr->Send("Alive", (*it.first).peer,
      //                                  kImmediateTimeout);
      it.first++;
    }
    ++enquiry_index;
    if (enquiry_index == kNumOfEnquiryGroup)
      enquiry_index = 0;
  }
}

void ManagedConnectionMap::EnquiryThread() {
  while (monitoring_mode_ == kActive) {
    // sleep a while to prevent flooding
    boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
    condition_enquiry_.notify_one();
  }
}

void ManagedConnectionMap::StartMonitoring(
    const MonitoringMode &monitoring_mode) {
  monitoring_mode_ = monitoring_mode;
  if (monitoring_mode == kActive) {
    thread_group_.reset(new boost::thread_group());
    thread_group_->create_thread(
        std::bind(&ManagedConnectionMap::AliveEnquiryThread, this));
    thread_group_->create_thread(
        std::bind(&ManagedConnectionMap::EnquiryThread, this));
  }
}

}  // namespace transport

}  // namespace maidsafe
