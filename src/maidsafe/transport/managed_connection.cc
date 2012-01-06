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
#include "maidsafe/transport/rudp/rudp_connection.h"

namespace maidsafe {

namespace transport {

ManagedConnectionMap::ManagedConnectionMap(std::string &ref_id)
    : ref_id_(ref_id),
      connections_container_(new ManagedConnectionContainer),
      notify_down_by_peer_id_(),
      notify_down_by_ref_id_(),
      notify_down_by_connection_id_(),
      shared_mutex_(),
      mutex_(),
      monitoring_mode_(kPassive),
      index_(10000),
      condition_enquiry_(),
      thread_group_(),
      enquiry_index(0) {}

ManagedConnectionMap::~ManagedConnectionMap() {
  monitoring_mode_ = kPassive;
  connections_container_->clear();
  if (thread_group_)  {
    thread_group_->interrupt_all();
    thread_group_->join_all();
    thread_group_.reset();
  }
}

//void ManagedConnectionMap::SetReservedPort(const uint32_t& port) {
//  if (!HasPort(port)) {
//    // insert an empty transport to make the port reserved
//    UniqueLock unique_lock(shared_mutex_);
//    ManagedConnectionContainer::index<TagConnectionId>::type&
//        index_by_connection_id = connections_container_->get<TagConnectionId>();
//    ManagedConnection mc(ConnectionPtr(), Endpoint(), "", "", port);
//    index_by_connection_id.insert(mc);
//  }
//}

boost::int32_t ManagedConnectionMap::CreateConnection(
    const Endpoint &peer,
    const std::string &reference_id) {

  //Todo(Prakash): Identify which transport from the pool to pick.
  TransportPtr transport;//(GetTransportFromPool());
  Endpoint endpoint;//(SendManagedConnectionReq(trnasport, peer));

  if (endpoint.ip == IP())
    return -1; // Error code reqd
  ConnectionPtr connection;//(SendManagedEndpointReq(endpoint));
  if (connection != ConnectionPtr())
    return InsertConnection(connection, peer, reference_id,
                            GenerateConnectionID());
  else {
    return -2; // Error code reqd
  }
}

boost::int32_t ManagedConnectionMap::InsertConnection(
    const ConnectionPtr connection,
    const std::string &reference_id) {
  //if (transport->transport_type() != kRUDP)
  //  return kError;
  IP localIP(boost::asio::ip::address_v4::loopback());
  boost::uint16_t peer_port = static_cast<uint16_t>(GenerateConnectionID());
  Endpoint peer(localIP, peer_port);
  std::stringstream out;
  out << peer_port;
  std::string peer_id = peer.ip.to_string() + ":" + out.str();

  UniqueLock unique_lock(shared_mutex_);
  ManagedConnectionContainer::index<TagConnectionId>::type&
      index_by_connection_id = connections_container_->get<TagConnectionId>();

  // For managed connections, the error handling shall be always in the charge
  // of ManagedConnectionMap
  //Todo(Prakash) : connecting signal should go where we wre initialising transport in the pool
  //transport->on_error()->connect(transport::OnError::element_type::slot_type(
  //    &ManagedConnectionMap::DoOnConnectionError, this, _1, _2));
  ManagedConnection mc(connection, peer, peer_id, reference_id, peer_port);
  index_by_connection_id.insert(mc);
  return mc.connectionid;
}

//boost::int32_t ManagedConnectionMap::InsertConnection(
//    const ConnectionPtr connection,
//    const Endpoint &peer,
//    const std::string &reference_id,
//    const boost::uint16_t port) {
//  return InsertConnection(connection, peer, reference_id, port);
//}

boost::int32_t ManagedConnectionMap::InsertConnection(
    const ConnectionPtr connection,
    const Endpoint &peer,
    const std::string &reference_id,
    const boost::uint16_t &port) {
  //if (transport->transport_type() != kRUDP)
  //  return -1;

  boost::uint32_t peer_port = peer.port;
  std::stringstream out;
  out << peer_port;
  std::string peer_id = peer.ip.to_string() + ":" + out.str();
  if (peer_id != "")
    return -1;
  if ((HasPeerId(peer_id)) || (HasPort(port)))
    return -3; // Error code indicating peerid or port exists

  UniqueLock unique_lock(shared_mutex_);
  ManagedConnectionContainer::index<TagConnectionId>::type&
      index_by_connection_id = connections_container_->get<TagConnectionId>();

  // For managed connections, the error handling shall be always handled by the
  // ManagedConnectionMap
  //Todo(Prakash) : connecting signal should go where we wre initialising transport in the pool
  //transport->on_error()->connect(transport::OnError::element_type::slot_type(
  //    &ManagedConnectionMap::DoOnConnectionError, this, _1, _2));
  ManagedConnection mc(connection, peer, peer_id, reference_id, port);
  index_by_connection_id.insert(mc);
  return mc.connectionid;
}

void ManagedConnectionMap::DoOnConnectionError(
    const TransportCondition &/*error*/, const Endpoint peer) {
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

bool ManagedConnectionMap::HasPort(const boost::uint32_t &index) {
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

bool ManagedConnectionMap::RemoveConnectionByPeerId(const std::string &peer_id) {
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

ConnectionPtr ManagedConnectionMap::GetConnectionByPeerId(
    const std::string &peer_id) {
  if (peer_id.empty())
    return ConnectionPtr();
  SharedLock shared_lock(shared_mutex_);
  ManagedConnectionContainer::index<TagConnectionPeerId>::type&
      index_by_peer_id = connections_container_->get<TagConnectionPeerId>();
  auto it = index_by_peer_id.find(peer_id);
  if (it == index_by_peer_id.end())
    return ConnectionPtr();
  return (*it).connection_ptr;
}

ConnectionPtr ManagedConnectionMap::GetConnection(
    const std::string &reference_id) {
  if (reference_id.empty())
    return ConnectionPtr();
  SharedLock shared_lock(shared_mutex_);
  ManagedConnectionContainer::index<TagReferenceId>::type&
      index_by_reference_id = connections_container_->get<TagReferenceId>();
  auto it = index_by_reference_id.find(reference_id);
  if (it == index_by_reference_id.end())
    return ConnectionPtr();
  return (*it).connection_ptr;
}

ConnectionPtr ManagedConnectionMap::GetConnection(
    const boost::uint32_t &connection_id) {
  SharedLock shared_lock(shared_mutex_);
  ManagedConnectionContainer::index<TagConnectionId>::type&
      index_by_connection_id = connections_container_->get<TagConnectionId>();
  auto it = index_by_connection_id.find(connection_id);
  if (it == index_by_connection_id.end())
    return ConnectionPtr();
  return (*it).connection_ptr;
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

void ManagedConnectionMap::AliveEnquiryThread() {
  boost::mutex::scoped_lock loch_surlaplage(mutex_);
  while (monitoring_mode_ == kActive) {
    condition_enquiry_.wait(loch_surlaplage);

    auto it = connections_container_->get<TagConnectionEnquiryGroup>().
                  equal_range(enquiry_index);
    while (it.first != it.second) {
      // TODO (qi.ma@maidsafe.net) : make the client support listen on socket
      // as well
      //if (((*it.first).is_connected) && ((*it.first).is_client))
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
    const MonitoringMode &monitoring_mode){
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
