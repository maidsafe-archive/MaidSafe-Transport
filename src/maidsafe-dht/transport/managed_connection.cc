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

#include "maidsafe-dht/transport/managed_connection.h"

namespace maidsafe {

namespace transport {

ManagedConnectionMap::ManagedConnectionMap()
    : connections_container_(new ManagedConnectionContainer),
      shared_mutex_(),
      monitoring_mode_(MonitoringMode::kPassive),
      index_(1501) {}

ManagedConnectionMap::~ManagedConnectionMap() {}

template <typename TransportType>
boost::uint32_t ManagedConnectionMap::CreateConnection(
    boost::asio::io_service &asio_service) {
  return CreateConnection<TransportType>(asio_service, "");
}

template <typename TransportType>
boost::uint32_t ManagedConnectionMap::CreateConnection(
    boost::asio::io_service &asio_service,
    const std::string &peer_id) {
  TransportPtr transport(new TransportType(asio_service));
  return InsertConnection(transport, peer_id);
}

void ManagedConnectionMap::SetReservedPort(const uint32_t& port) {
  if (!HasPort(port)) {
    // insert an empty transport to make the port reserved
    UniqueLock unique_lock(shared_mutex_);
    ManagedConnectionContainer::index<TagConnectionId>::type&
        index_by_connection_id = connections_container_->get<TagConnectionId>();
    ManagedConnection mc(TransportPtr(), "", port);
    index_by_connection_id.insert(mc);
  }
}

boost::uint32_t ManagedConnectionMap::InsertConnection(
    const TransportPtr transport) {
  return InsertConnection(transport, "");
}

boost::uint32_t ManagedConnectionMap::InsertConnection(
    const TransportPtr transport,
    const std::string &peer_id) {
  if ((peer_id != "") && (HasPeerId(peer_id)))
    return -1;

  UniqueLock unique_lock(shared_mutex_);
  ManagedConnectionContainer::index<TagConnectionId>::type&
      index_by_connection_id = connections_container_->get<TagConnectionId>();
  ManagedConnection mc(transport, peer_id, GenerateConnectionID());
  index_by_connection_id.insert(mc);
  return mc.connectionid;
}

boost::uint32_t ManagedConnectionMap::GenerateConnectionID() {
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
  if (peer_id == "")
    return false;
  SharedLock shared_lock(shared_mutex_);
  ManagedConnectionContainer::index<TagConnectionPeerId>::type&
      index_by_peer_id = connections_container_->get<TagConnectionPeerId>();
  auto it = index_by_peer_id.find(peer_id);
  if (it == index_by_peer_id.end())
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

bool ManagedConnectionMap::RemoveConnection(const std::string &peer_id) {
  if (peer_id == "")
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

TransportPtr ManagedConnectionMap::GetConnection(const std::string &peer_id) {
  if (peer_id == "")
    return TransportPtr();

  SharedLock shared_lock(shared_mutex_);
  ManagedConnectionContainer::index<TagConnectionPeerId>::type&
      index_by_peer_id = connections_container_->get<TagConnectionPeerId>();
  auto it = index_by_peer_id.find(peer_id);
  if (it == index_by_peer_id.end())
    return TransportPtr();
  return (*it).transport_ptr;
}

TransportPtr ManagedConnectionMap::GetConnection(
    const boost::uint32_t &connection_id) {
  SharedLock shared_lock(shared_mutex_);
  ManagedConnectionContainer::index<TagConnectionId>::type&
      index_by_connection_id = connections_container_->get<TagConnectionId>();
  auto it = index_by_connection_id.find(connection_id);
  if (it == index_by_connection_id.end())
    return TransportPtr();
  return (*it).transport_ptr;
}

}  // namespace transport

}  // namespace maidsafe
