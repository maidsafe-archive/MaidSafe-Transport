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

#include <string>

#include "boost/multi_index_container.hpp"
#include "boost/multi_index/composite_key.hpp"
#include "boost/multi_index/ordered_index.hpp"
#include "boost/multi_index/identity.hpp"
#include "boost/multi_index/member.hpp"
#include "boost/multi_index/mem_fun.hpp"
#include "boost/thread/shared_mutex.hpp"
#include "boost/thread/locks.hpp"

#include "maidsafe-dht/kademlia/config.h"
#include "maidsafe-dht/transport/transport.h"

namespace maidsafe  {

namespace transport {

typedef std::shared_ptr<Transport> TransportPtr;

struct ManagedConnection {
  ManagedConnection(const TransportPtr transport,
                    const std::string &peer_id,
                    const boost::uint32_t &connection_id)
      : transport_ptr(transport), peerid(peer_id),
        connectionid(connection_id) {}
  bool IsConnected() const {
//     return transport_ptr->IsConnected();
    // TODO (qi.ma@maisdsafe.net) : make transport API support isconnected()
    return true;
  }
  TransportPtr transport_ptr;
  std::string peerid;
  boost::uint32_t connectionid;
};

struct TagConnectionId {};
struct TagConnectionPeerId {};
struct TagConnectionStatus {};

typedef boost::multi_index::multi_index_container<
  ManagedConnection,
  boost::multi_index::indexed_by<
    boost::multi_index::ordered_unique<
      boost::multi_index::tag<TagConnectionId>,
      BOOST_MULTI_INDEX_MEMBER(ManagedConnection, boost::uint32_t, connectionid)
    >,
    boost::multi_index::ordered_non_unique<
      boost::multi_index::tag<TagConnectionPeerId>,
      BOOST_MULTI_INDEX_MEMBER(ManagedConnection, std::string, peerid)
    >,
    boost::multi_index::ordered_non_unique<
      boost::multi_index::tag<TagConnectionStatus>,
      boost::multi_index::const_mem_fun<ManagedConnection, bool,
                                        &ManagedConnection::IsConnected>
    >
  >
> ManagedConnectionContainer;

enum MonitoringMode { kPassive, kActive };

typedef std::shared_ptr<boost::signals2::signal<void(const std::string&)>>
    NotifyDownConnectionPtr;

// This class holds the managed connections, monitoring the connection status
// Two monitoring methods provided:
//      Passive mode: waiting for the signal notification from the transport
//   or
//      Active mode: round-robin all connections' status every defined interval
//
// For each communication pair, dedicated port will be assigned.
// It will be the higher layer code's responsibility to negotiate between pair
// about which port to be used on two sides.
// One port per peer rule shall be followed, so here port number will be used
// as ConnectionId directly.
class ManagedConnectionMap  {
 public:
  ManagedConnectionMap();

  ~ManagedConnectionMap();
  // Creates a managed connection into the multi index container
  // The connection_id will be returned
  template <typename TransportType>
  boost::uint32_t CreateConnection(boost::asio::io_service &asio_service);
  template <typename TransportType>
  boost::uint32_t CreateConnection(boost::asio::io_service &asio_service,
                                   const std::string &peer_id);

  // Set system reserved port
  // System reserved port shall not be used as communication connection
  // port 0 - 1500 will be reserved by default
  void SetReservedPort(const boost::uint32_t &port);

  // Check if the specified port has already be occupied
  bool HasPort(const boost::uint32_t &index);

  // Adds a managed connection into the multi index container
  // The connection_id will be returned
  boost::uint32_t InsertConnection(const TransportPtr transport);
  boost::uint32_t InsertConnection(const TransportPtr transport,
                                   const std::string &peer_id);

  // Remove a managed connection based on the node_id
  // Returns true if successfully removed or false otherwise.
  bool RemoveConnection(const std::string &peer_id);
  bool RemoveConnection(const boost::uint32_t &connection_id);

  // Return the TransportPtr of the node
  TransportPtr GetConnection(const std::string &peer_id);
  TransportPtr GetConnection(const boost::uint32_t &connection_id);

  // Update the TransportPtr of the node
  TransportPtr UpdateConnection(const std::string &peer_id);
  TransportPtr UpdateConnection(const boost::uint32_t &connection_id);

  // Start Monitoring, default will use Passive mode to monitor
  //    In Active mode, this function will start up a monitoring thread
  //    In Passive mode, this needs to do nothing
  void StartMonitoring(const MonitoringMode &monitoring_mode);

  /** Getter.
   *  @return The notify_down_connection_ signal. */
  NotifyDownConnectionPtr notify_down_connection();

 private:
  /** Thread function keeps monitoring the connections' status */
  void MonitoringConnectionsStatus();

  /** Generate next unique empty connection ID */
  boost::uint32_t GenerateConnectionID();

  /** Check if the input peer_id already contained in the multi-index */
  bool HasPeerId(const std::string &peer_id);

  /**  Multi_index container of managed connections */
  std::shared_ptr<ManagedConnectionContainer> connections_container_;
  /** Signal to be fired when there is one dropped connection detected */
  NotifyDownConnectionPtr notify_down_connection_;
  /** Thread safe shared mutex */
  boost::shared_mutex shared_mutex_;
  /** Flag of monitoring mode */
  MonitoringMode monitoring_mode_;
  /** Global Counter used as an connection ID for each added transport */
  boost::uint32_t index_;

//   boost::thread_group thread_group_;

  typedef boost::shared_lock<boost::shared_mutex> SharedLock;
  typedef boost::upgrade_lock<boost::shared_mutex> UpgradeLock;
  typedef boost::unique_lock<boost::shared_mutex> UniqueLock;
  typedef boost::upgrade_to_unique_lock<boost::shared_mutex>
      UpgradeToUniqueLock;
};

}  // namespace transport

}  // namespace maidsafe