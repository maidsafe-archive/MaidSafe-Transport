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

class RudpConnection;

typedef std::shared_ptr<Transport> TransportPtr;
typedef std::shared_ptr<RudpConnection> ConnectionPtr;

// Maximum number of bytes to read at a time
const int kNumOfEnquiryGroup = 5;

struct ManagedConnection {
  ManagedConnection(const ConnectionPtr connection,
                    const Endpoint &new_peer,
                    const std::string &peer_id,
                    const std::string &referenceid,
                    const boost::uint32_t &connection_id)
      : connection_ptr(connection), peer(new_peer), peerid(peer_id),
        reference_id(referenceid), connectionid(connection_id),
        is_connected(true),
        enquiry_group(RandomUint32() % kNumOfEnquiryGroup) {}

  //ManagedConnection(const ConnectionPtr connection,
  //                  const Endpoint &new_peer,
  //                  const std::string &peer_id,
  //                  const std::string &referenceid,
  //                  const boost::uint32_t &connection_id,
  //                  const bool client_mode)
  //    : connection_ptr(connection), peer(new_peer), peerid(peer_id),
  //      reference_id(referenceid), connectionid(connection_id),
  //      is_connected(true), is_client(client_mode),
  //      enquiry_group(RandomUint32() % kNumOfEnquiryGroup) {}

  ConnectionPtr connection_ptr;
  Endpoint peer;
  std::string peerid;
  std::string reference_id;
  boost::uint32_t connectionid;
  bool is_connected;
  int enquiry_group;
};

struct ChangeConnectionStatus {
  explicit ChangeConnectionStatus(bool new_status) : status(new_status) {}
  void operator()(ManagedConnection &managed_connection) {
    managed_connection.is_connected = status;
  }
  bool status;
};

struct TagConnectionId {};
struct TagConnectionPeerId {};
struct TagReferenceId {};
struct TagConnectionStatus {};
struct TagConnectionEnquiryGroup {};

typedef boost::multi_index::multi_index_container<
  ManagedConnection,
  boost::multi_index::indexed_by<
    boost::multi_index::ordered_unique<
      boost::multi_index::tag<TagConnectionId>,
      BOOST_MULTI_INDEX_MEMBER(ManagedConnection, boost::uint32_t, connectionid)
    >,
    boost::multi_index::ordered_unique<
      boost::multi_index::tag<TagConnectionPeerId>,
      BOOST_MULTI_INDEX_MEMBER(ManagedConnection, std::string, peerid)
    >,
    boost::multi_index::ordered_unique<
      boost::multi_index::tag<TagReferenceId>,
      BOOST_MULTI_INDEX_MEMBER(ManagedConnection, std::string, reference_id)
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
    NotifyDownPtr;
typedef std::shared_ptr<boost::signals2::signal<void(const boost::uint32_t&)>>
    NotifyDownByConnectionIdPtr;

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
  ManagedConnectionMap(std::string &ref_id);

  ~ManagedConnectionMap();

  // Set system reserved port
  // System reserved port shall not be used as communication connection
  // port 0 - 1500 will be reserved by default
//  void SetReservedPort(const boost::uint32_t &port);

  // Check if the specified port has already be occupied
  bool HasPort(const boost::uint32_t &index);

  // Creates a managed connection into the multi index container
  // The connection_id will be returned

  boost::int32_t CreateConnection(const Endpoint &peer,
                                  const std::string &reference_id);
  boost::int32_t InsertIncomingConnection(const Endpoint &peer,
                                          const std::string &reference_id);

  // Re-establishes the connection if it is lost
  bool UpdateConnectionByPeerId(const std::string &peer_id);
  bool UpdateConnection(const boost::uint32_t &connection_id);
  bool UpdateConnection(const std::string &reference_id);

  // Remove a managed connection based on the peer_id/connection_id/reference_id
  // Returns true if successfully removed or false otherwise.
  bool RemoveConnectionByPeerId(const std::string &peer_id);
  bool RemoveConnection(const boost::uint32_t &connection_id);
  bool RemoveConnection(const std::string &reference_id);

  // Return the connection status
  bool IsConnectedByPeerId(const std::string &peer_id);
  bool IsConnected(const boost::uint32_t &connection_id);
  bool IsConnected(const std::string &reference_id);

  // Sends the data through existing connection referred by
  // endpoint/connection_id/reference_id. If the connection reffered doesn't
  // exists, false will be returned.
  // This timeout define the max allowed duration for the receiver to respond
  // a received request. If the receiver is to be expected respond slow
  // (say because of the large request msg to be processed), a long duration
  // shall be given for this timeout.
  // If no response to be expected, kImmediateTimeout shall be given.
  bool Send(const std::string &data,
            const Endpoint &endpoint,
            const Timeout &timeout);
  bool Send(const std::string &data,
            const boost::uint32_t &connection_id,
            const Timeout &timeout);
  bool Send(const std::string &data,
            const std::string &reference_id,
            const Timeout &timeout);

  // Start Monitoring, default will use Passive mode to monitor
  //    In Active mode, this function will start up a monitoring thread
  //    In Passive mode, this needs to do nothing
  void StartMonitoring(const MonitoringMode &monitoring_mode);

  /** Getter.
   *  @return The notify_down_connection_ signal. */
  NotifyDownPtr notify_down_by_peer_id();
  NotifyDownPtr notify_down_by_ref_id();
  NotifyDownByConnectionIdPtr notify_down_by_connection_id();

  /** Return next available port */
//  boost::uint16_t NextEmptyPort() { return GenerateConnectionID(); }

 private:
  // Adds a managed connection into the multi index container
  // The connection_id will be returned
  boost::int32_t InsertConnection(const ConnectionPtr connection,
                                  const std::string &reference_id);
  boost::int32_t InsertConnection(const ConnectionPtr connection,
                                  const Endpoint &peer,
                                  const std::string &reference_id,
                                  const boost::uint16_t &port);
  //boost::int32_t InsertConnection(const ConnectionPtr connection,
  //                                const Endpoint &peer,
  //                                const std::string &reference_id,
  //                                const boost::uint16_t &port);

  // Return the ConnectionPtr of the node
  ConnectionPtr GetConnectionByPeerId(const std::string &peer_id);
  ConnectionPtr GetConnection(const boost::uint32_t &connection_id);
  ConnectionPtr GetConnection(const std::string &reference_id);

   /** Generate next unique empty connection ID */
  boost::uint16_t GenerateConnectionID();

  /** Check if the input peer_id already contained in the multi-index */
  bool HasPeerId(const std::string &peer_id);

  /** Check if the input reference_id already contained in the multi-index */
  bool HasReferenceId(const std::string &reference_id);

  Endpoint SendManagedConnectionReq(TransportPtr transport,
                                    const Endpoint &peer);
  /** Handle connection error, mark corresponding connection to be DOWN */
  void DoOnConnectionError(const TransportCondition &error,
                           const Endpoint peer);

  /** Thread function to keep enquiring all entries about the alive status */
  void AliveEnquiryThread();
  void EnquiryThread();

  std::string ref_id_;
  /** Multi_index container of managed connections */
  std::shared_ptr<ManagedConnectionContainer> connections_container_;
  /** Signal to be fired when there is one dropped connection detected */
  NotifyDownPtr notify_down_by_peer_id_;
  NotifyDownPtr notify_down_by_ref_id_;
  NotifyDownByConnectionIdPtr notify_down_by_connection_id_;

  /** Thread safe shared mutex */
  boost::shared_mutex shared_mutex_;
  /** Mutex lock */
  boost::mutex mutex_;
  /** Flag of monitoring mode */
  MonitoringMode monitoring_mode_;
  /** Global Counter used as an connection ID for each added transport */
  boost::uint16_t index_;
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