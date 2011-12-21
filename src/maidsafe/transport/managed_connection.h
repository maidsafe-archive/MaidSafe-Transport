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
#include <sstream>

#include "boost/multi_index_container.hpp"
#include "boost/multi_index/composite_key.hpp"
#include "boost/multi_index/ordered_index.hpp"
#include "boost/multi_index/identity.hpp"
#include "boost/multi_index/member.hpp"
#include "boost/multi_index/mem_fun.hpp"
#include "boost/thread/shared_mutex.hpp"
#include "boost/thread/locks.hpp"
#include "boost/thread.hpp"

#include "maidsafe/common/utils.h"

//  #include "maidsafe/dht/kademlia/config.h"
#include "maidsafe/transport/transport.h"

namespace maidsafe  {

namespace transport {

typedef std::shared_ptr<Transport> TransportPtr;

// Maximum number of bytes to read at a time
const int kNumOfEnquiryGroup = 5;

struct ManagedConnection {
  ManagedConnection(const TransportPtr transport,
                    const Endpoint new_peer,
                    const std::string &peer_id,
                    const boost::uint32_t &connection_id)
      : transport_ptr(transport), peer(new_peer), peerid(peer_id),
        connectionid(connection_id), is_connected(true), is_client(true),
        enquiry_group(RandomUint32() % kNumOfEnquiryGroup) {}

  ManagedConnection(const TransportPtr transport,
                    const Endpoint new_peer,
                    const std::string &peer_id,
                    const boost::uint32_t &connection_id,
                    const bool client_mode)
      : transport_ptr(transport), peer(new_peer), peerid(peer_id),
        connectionid(connection_id), is_connected(true),
        is_client(client_mode), enquiry_group(RandomUint32() % kNumOfEnquiryGroup) {}

  TransportPtr transport_ptr;
  Endpoint peer;
  std::string peerid;
  boost::uint32_t connectionid;
  bool is_connected;
  bool is_client;
  int enquiry_group;
};

struct ChangeConnectionStatus {
  explicit ChangeConnectionStatus(bool new_status) : status(new_status) {}
  void operator()(ManagedConnection &manged_connection) {
    manged_connection.is_connected = status;
  }
  bool status;
};

struct TagConnectionId {};
struct TagConnectionPeerId {};
struct TagConnectionStatus {};
struct TagConnectionEnquiryGroup {};

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
      BOOST_MULTI_INDEX_MEMBER(ManagedConnection, bool, is_connected)
    >,
    boost::multi_index::ordered_non_unique<
      boost::multi_index::tag<TagConnectionEnquiryGroup>,
      BOOST_MULTI_INDEX_MEMBER(ManagedConnection, int, enquiry_group)
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

  // Set system reserved port
  // System reserved port shall not be used as communication connection
  // port 0 - 1500 will be reserved by default
  void SetReservedPort(const boost::uint32_t &port);

  // Check if the specified port has already be occupied
  bool HasPort(const boost::uint32_t &index);

  // Creates a managed connection into the multi index container
  // The connection_id will be returned
  template <typename TransportType>
  boost::int32_t CreateConnection(boost::asio::io_service &asio_service,
                                   const Endpoint &peer);

  // Adds a managed connection into the multi index container
  // The connection_id will be returned
  boost::int32_t InsertConnection(const TransportPtr transport);
  boost::int32_t InsertConnection(const TransportPtr transport,
                                   const Endpoint &peer,
                                   const boost::uint32_t port);
  boost::int32_t InsertConnection(const TransportPtr transport,
                                   const Endpoint &peer,
                                   const boost::uint32_t port,
                                   const bool server_mode);

  // Remove a managed connection based on the node_id
  // Returns true if successfully removed or false otherwise.
  bool RemoveConnection(const std::string &peer_id);
  bool RemoveConnection(const boost::uint32_t &connection_id);

  // Return the TransportPtr of the node
  TransportPtr GetConnection(const std::string &peer_id);
  TransportPtr GetConnection(const boost::uint32_t &connection_id);

  // Return the Connection Status
  bool IsConnected(const std::string &peer_id);
  bool IsConnected(const boost::uint32_t &connection_id);

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

  /** Return next available port */
  boost::uint32_t NextEmptyPort() { return GenerateConnectionID(); }

 private:
  /** Generate next unique empty connection ID */
  boost::uint32_t GenerateConnectionID();

  /** Check if the input peer_id already contained in the multi-index */
  bool HasPeerId(const std::string &peer_id);

  /** Handle connection error, mark corresponding connection to be DOWN */
  void DoOnConnectionError(const TransportCondition &error,
                           const Endpoint peer);

  /** Thread function to keep enquiring all entries about the alive status */
  void AliveEnquiryThread();
  void EnquiryThread();

  /** Multi_index container of managed connections */
  std::shared_ptr<ManagedConnectionContainer> connections_container_;
  /** Signal to be fired when there is one dropped connection detected */
  NotifyDownConnectionPtr notify_down_connection_;
  /** Thread safe shared mutex */
  boost::shared_mutex shared_mutex_;
  /** Mutex lock */
  boost::mutex mutex_;
  /** Flag of monitoring mode */
  MonitoringMode monitoring_mode_;
  /** Global Counter used as an connection ID for each added transport */
  boost::uint32_t index_;
  /** Conditional Variable to wait/notify the thread monitoring function*/
  boost::condition_variable condition_enquiry_;
  /** The thread group to hold all monitoring treads
   *  Used by: AliveEnquiryThread */
  std::shared_ptr<boost::thread_group> thread_group_;
  /** Bucket num for the current group to enquiry alive status */
  int enquiry_index;

  typedef boost::shared_lock<boost::shared_mutex> SharedLock;
  typedef boost::upgrade_lock<boost::shared_mutex> UpgradeLock;
  typedef boost::unique_lock<boost::shared_mutex> UniqueLock;
  typedef boost::upgrade_to_unique_lock<boost::shared_mutex>
      UpgradeToUniqueLock;
};

}  // namespace transport

}  // namespace maidsafe