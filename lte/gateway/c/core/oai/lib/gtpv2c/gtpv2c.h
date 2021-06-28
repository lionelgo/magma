/*
Copyright 2020 The Magma Authors.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#ifndef FILE_GTPV2C_HPP_SEEN
#define FILE_GTPV2C_HPP_SEEN

#include "3gpp_29.274cc.h"
extern "C" {
#include "intertask_interface.h"
}
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "msg_gtpv2c.h"

#include <folly/AtomicHashMap.h>

#include "EndPoint.h"
#include "uint_generator.h"
#include "udp.h"

namespace magma {
namespace lte {
namespace gtpv2c {

static const uint16_t default_port = 2123;

typedef int timer_id_t;

class Gtpv2cProcedure {
 public:
  std::shared_ptr<gtpv2c_msg> retry_msg;
  EndPoint remote_endpoint;
  teid_t local_teid;  // for peer not responding
  timer_id_t retry_timer_id;
  timer_id_t proc_cleanup_timer_id;
  uint64_t gtpc_tx_id;
  uint8_t initial_msg_type;    // sent or received
  uint8_t triggered_msg_type;  // sent or received
  uint8_t retry_count;
  // Could add customized N3, and customized T3:
  // T3-RESPONSE timer and N3-REQUESTS counter setting is implementation
  // dependent. That is, the timers and counters may be configurable per
  // procedure. Multileg communications (e.g. Create Session Requests and
  // Responses) however require longer timer values and possibly a higher number
  // of retransmission attempts compared to single leg communication.
  Gtpv2cProcedure()
      : retry_msg(),
        remote_endpoint(),
        local_teid(0),
        retry_timer_id(0),
        proc_cleanup_timer_id(0),
        gtpc_tx_id(0),
        initial_msg_type(0),
        triggered_msg_type(0),
        retry_count(0) {}

  Gtpv2cProcedure(const Gtpv2cProcedure& p)
      : retry_msg(p.retry_msg),
        remote_endpoint(p.remote_endpoint),
        local_teid(p.local_teid),
        retry_timer_id(p.retry_timer_id),
        proc_cleanup_timer_id(p.proc_cleanup_timer_id),
        gtpc_tx_id(p.gtpc_tx_id),
        initial_msg_type(p.initial_msg_type),
        triggered_msg_type(p.triggered_msg_type),
        retry_count(p.retry_count) {}
};

enum gtpv2c_transaction_action { DELETE_TX = 0, CONTINUE_TX };

class Gtpv2cStack : public UdpApplication {
#define GTPV2C_PROC_TIME_OUT_MS(T, N) ((T) * (N + 1 + 1))

 protected:
  uint32_t t3_ms;
  uint32_t n3;
  uint32_t id;
  UdpServer udp_s;
  UdpServer udp_s_allocated;

  // seems no need for atomic
  uint32_t seq_num;
  std::mutex m_seq_num;
  uint32_t restart_counter;

  // key is transaction id
  folly::AtomicHashMap<uint64_t, uint32_t> gtpc_tx_id2seq_num;
  folly::AtomicHashMap<timer_id_t, uint32_t> proc_cleanup_timers;
  folly::AtomicHashMap<timer_id_t, uint32_t> msg_out_retry_timers;
  // key is message sequence number
  folly::AtomicHashMap<uint32_t, Gtpv2cProcedure> pending_procedures;

  static const char* msg_type2cstr[256];

  uint32_t GetNextSeqNum();

  static uint64_t GenerateGtpcTxId() {
    return UintUidGenerator<uint64_t>::GetInstance().GetUid();
  }

  static void FreeGtpcTxId(const uint64_t gtpc_tx_id) {
    UintUidGenerator<uint64_t>::GetInstance().FreeUid(gtpc_tx_id);
  }

  static bool CheckInitialMessageType(const uint8_t initial);
  static bool CheckTriggeredMessageType(
      const uint8_t initial, const uint8_t triggered);
  void StartProcCleanupTimer(
      Gtpv2cProcedure& p, uint32_t time_out_milli_seconds,
      const task_id_t& task_id, const uint32_t& seq_num);
  void StartMsgRetryTimer(
      Gtpv2cProcedure& p, uint32_t time_out_milli_seconds,
      const task_id_t& task_id, const uint32_t& seq_num);
  void StartMsgRetryTimer(Gtpv2cProcedure& p);
  void StopProcCleanupTimer(timer_id_t& t);
  void StopProcCleanupTimer(Gtpv2cProcedure& p);
  void NotifyUlError(
      const Gtpv2cProcedure& p, const gtpv2c_cause_value_e cause);

 public:
  static const uint8_t version = 2;
  Gtpv2cStack(
      const uint32_t t1_milli_seconds, const uint32_t n1_retransmit,
      const std::string& ip_address, const unsigned short port_num,
      const ThreadSchedParams& sched_param);
  virtual void HandleReceive(
      char* recv_buffer, const std::size_t bytes_transferred,
      const EndPoint& r_EndPoint);
  virtual void NotifyUlError(
      const EndPoint& r_EndPoint, const teid_t l_teid,
      const gtpv2c_cause_value_e cause, const uint64_t gtpc_tx_id);

  void HandleReceiveMessageCb(
      const gtpv2c_msg& msg, const EndPoint& r_EndPoint,
      const task_id_t& task_id, bool& error, uint64_t& gtpc_tx_id);

  // Path mangement messages
  virtual uint32_t SendInitialMessage(
      const EndPoint& r_EndPoint, const gtpv2c_echo_request& gtp_ies,
      const task_id_t& task_id, const uint64_t gtp_tx_id);
  virtual void SendTriggeredMessage(
      const EndPoint& r_EndPoint, const gtpv2c_echo_response& gtp_ies,
      const uint64_t gtp_tx_id, const gtpv2c_transaction_action& a = DELETE_TX);

  // Tunnel management messages
  virtual uint32_t SendInitialMessage(
      const EndPoint& r_EndPoint, const teid_t r_teid, const teid_t l_teid,
      const gtpv2c_create_session_request& gtp_ies, const task_id_t& task_id,
      const uint64_t gtp_tx_id);
  virtual uint32_t SendInitialMessage(
      const EndPoint& r_EndPoint, const teid_t r_teid, const teid_t l_teid,
      const gtpv2c_delete_session_request& gtp_ies, const task_id_t& task_id,
      const uint64_t gtp_tx_id);
  virtual uint32_t SendInitialMessage(
      const EndPoint& r_EndPoint, const teid_t r_teid, const teid_t l_teid,
      const gtpv2c_modify_bearer_request& gtp_ies, const task_id_t& task_id,
      const uint64_t gtp_tx_id);
  virtual uint32_t SendInitialMessage(
      const EndPoint& r_EndPoint, const teid_t r_teid, const teid_t l_teid,
      const gtpv2c_release_access_bearers_request& gtp_ies,
      const task_id_t& task_id, const uint64_t gtp_tx_id);
  virtual uint32_t SendInitialMessage(
      const EndPoint& r_EndPoint, const teid_t r_teid, const teid_t l_teid,
      const gtpv2c_downlink_data_notification& gtp_ies,
      const task_id_t& task_id, const uint64_t gtp_tx_id);

  //  virtual void SendTriggeredMessage(
  //      const EndPoint& r_EndPoint, const teid_t teid,
  //      const gtpv2c_create_session_response& gtp_ies, const uint64_t
  //      gtp_tx_id, const gtpv2c_transaction_action& a = DELETE_TX);
  //  virtual void SendTriggeredMessage(
  //      const EndPoint& r_EndPoint, const teid_t teid,
  //      const gtpv2c_delete_session_response& gtp_ies, const uint64_t
  //      gtp_tx_id, const gtpv2c_transaction_action& a = DELETE_TX);
  //  virtual void SendTriggeredMessage(
  //      const EndPoint& r_EndPoint, const teid_t teid,
  //      const gtpv2c_modify_bearer_response& gtp_ies, const uint64_t
  //      gtp_tx_id, const gtpv2c_transaction_action& a = DELETE_TX);
  //  virtual void SendTriggeredMessage(
  //      const EndPoint& r_EndPoint, const teid_t teid,
  //      const gtpv2c_release_access_bearers_response& gtp_ies,
  //      const uint64_t gtp_tx_id, const gtpv2c_transaction_action& a =
  //      DELETE_TX);
  //  virtual void SendTriggeredMessage(
  //      const EndPoint& r_EndPoint, const teid_t teid,
  //      const gtpv2c_downlink_data_notification_acknowledge& gtp_ies,
  //      const uint64_t gtp_tx_id, const gtpv2c_transaction_action& a =
  //      DELETE_TX);
  template<typename GTP_IES>
  void SendTriggeredMessage(
      const EndPoint& r_EndPoint, const teid_t teid, const GTP_IES& gtp_ies,
      const uint64_t gtp_tx_id, const gtpv2c_transaction_action& a = DELETE_TX);

  void TimeOutEvent(
      const uint32_t timer_id, const task_id_t& task_id, bool& error);
};
}  // namespace gtpv2c
}  // namespace lte
}  // namespace magma

#endif /* FILE_GTPV2C_HPP_SEEN */
