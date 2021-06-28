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

#include "s11.h"
extern "C" {
#include "common_defs.h"
#include "intertask_interface.h"
#include "log.h"
#include "mme_config.h"
}
#include <stdexcept>

using namespace magma::lte;
using namespace std;

task_zmq_ctx_t mme_s11_task_zmq_ctx;

S11* s11_inst;

static int handle_message(zloop_t* loop, zsock_t* reader, void* arg);
static void* mme_s11_thread(void* args);
static void mme_s11_exit(void);

//------------------------------------------------------------------------------
static void* mme_s11_thread(void* args) {
  itti_mark_task_ready(TASK_S11);
  task_id_t task_ids[] = {TASK_MME_APP};
  init_task_context(
      TASK_S11, task_ids, 1, handle_message, &mme_s11_task_zmq_ctx);

  zloop_start(mme_s11_task_zmq_ctx.event_loop);
  mme_s11_exit();
  return NULL;
}
//------------------------------------------------------------------------------
int s11_mme_init(mme_config_t* mme_config_p) {
  int ret = 0;

  OAILOG_DEBUG(LOG_S11, "Initializing MME S11 interface\n");

  try {
    s11_inst = new S11();

    if (itti_create_task(TASK_S11, &mme_s11_thread, mme_config_p) < 0) {
      OAILOG_ERROR(LOG_S11, "Failed to create mme_s11 task\n");
      return RETURNerror;
    }
    OAILOG_DEBUG(LOG_S11, "Initializing MME S11 interface: DONE\n");
    return ret;
  } catch (std::exception e) {
    OAILOG_DEBUG(LOG_S11, "Initializing MME S11 interface: FAILURE\n");
    return RETURNerror;
  }
}
//------------------------------------------------------------------------------
static int handle_message(zloop_t* loop, zsock_t* reader, void* arg) {
  MessageDef* received_message_p = receive_msg(reader);

  switch (ITTI_MSG_ID(received_message_p)) {
    case MESSAGE_TEST: {
      OAI_FPRINTF_INFO("TASK_S11 received MESSAGE_TEST\n");
    } break;

    case S11_CREATE_BEARER_RESPONSE: {
      gtpv2c::gtpv2c_create_bearer_response m(
          received_message_p->ittiMsg.s11_create_bearer_response);
      s11_inst->SendMsg(
          received_message_p->ittiMsg.s11_create_bearer_response.local_teid, m,
          received_message_p->ittiMsg.s11_create_bearer_response.trxn);
    } break;

    case S11_CREATE_SESSION_REQUEST: {
      gtpv2c::gtpv2c_create_session_request m(
          received_message_p->ittiMsg.s11_create_session_request);
      s11_inst->SendMsg(
          received_message_p->ittiMsg.s11_create_session_request.r_endpoint,
          received_message_p->ittiMsg.s11_create_session_request.local_teid, m,
          received_message_p->ittiMsg.s11_create_session_request.trxn);
    } break;

    case S11_DELETE_SESSION_REQUEST: {
      gtpv2c::gtpv2c_delete_session_request m(
          received_message_p->ittiMsg.s11_delete_session_request);
      s11_inst->SendMsg(
          received_message_p->ittiMsg.s11_delete_session_request.r_endpoint,
          received_message_p->ittiMsg.s11_delete_session_request.local_teid, m,
          received_message_p->ittiMsg.s11_delete_session_request.trxn);
    } break;

    case S11_DELETE_BEARER_COMMAND: {
      s11_mme_delete_bearer_command(
          &s11_mme_stack_handle,
          &received_message_p->ittiMsg.s11_delete_bearer_command);
    } break;

    case S11_MODIFY_BEARER_REQUEST: {
      gtpv2c::gtpv2c_modify_bearer_request m(
          received_message_p->ittiMsg.s11_modify_bearer_request);
      s11_inst->SendMsg(
          received_message_p->ittiMsg.s11_modify_bearer_request.r_endpoint,
          received_message_p->ittiMsg.s11_modify_bearer_request.local_teid, m,
          received_message_p->ittiMsg.s11_modify_bearer_request.trxn);
    } break;

    case S11_RELEASE_ACCESS_BEARERS_REQUEST: {
      gtpv2c::gtpv2c_release_access_bearers_request m(
          received_message_p->ittiMsg.s11_release_access_bearers_request);
      s11_inst->SendMsg(
          received_message_p->ittiMsg.s11_release_access_bearers_request
              .r_endpoint,
          received_message_p->ittiMsg.s11_release_access_bearers_request
              .local_teid,
          m,
          received_message_p->ittiMsg.s11_release_access_bearers_request.trxn);
    } break;

    case S11_DOWNLINK_DATA_NOTIFICATION_ACKNOWLEDGE: {
      gtpv2c::gtpv2c_downlink_data_notification_acknowledge m(
          received_message_p->ittiMsg
              .s11_downlink_data_notification_acknowledge);
      s11_inst->SendMsg(
          received_message_p->ittiMsg.s11_downlink_data_notification_acknowledge
              .local_teid,
          m,
          received_message_p->ittiMsg.s11_downlink_data_notification_acknowledge
              .trxn);
    } break;

    case TERMINATE_MESSAGE: {
      itti_free_msg_content(received_message_p);
      free(received_message_p);
      s11_mme_exit();
    } break;

    case TIMER_HAS_EXPIRED: {
      OAILOG_DEBUG(
          LOG_S11, "Processing timeout for timer_id 0x%lx and arg %p\n",
          received_message_p->ittiMsg.timer_has_expired.timer_id,
          received_message_p->ittiMsg.timer_has_expired.arg);
      nw_rc_t nw_rc = nwGtpv2cProcessTimeout(
          received_message_p->ittiMsg.timer_has_expired.arg);
      if (nw_rc != NW_OK) {
        OAILOG_DEBUG(
            LOG_S11,
            "Processing timeout for timer_id 0x%lx and arg %p failed\n",
            received_message_p->ittiMsg.timer_has_expired.timer_id,
            received_message_p->ittiMsg.timer_has_expired.arg);
      }
    } break;

    default:
      OAILOG_ERROR(
          LOG_S11, "Unknown message ID %d:%s\n",
          ITTI_MSG_ID(received_message_p), ITTI_MSG_NAME(received_message_p));
      break;
  }

  itti_free_msg_content(received_message_p);
  free(received_message_p);
  return 0;
}

//------------------------------------------------------------------------------
s11::s11()
    : gtpv2c_stack(
          mme_config.gtpv2c_config.t3_ms, mme_config.gtpv2c_config.n3,
          string(inet_ntoa(pgwc::pgw_config::s11_.iface.addr4)),
          mme_config.gtpv2c_config.port) {}
//------------------------------------------------------------------------------
void s11::send_msg(
    teid_t local_teid, gtpv2c::gtpv2c_create_bearer_response& gtp_ies,
    uint64_t gtpc_tx_id) {
  send_triggered_message(local_teid, gtp_ies, gtpc_tx_id, CONTINUE_TX);
}
//------------------------------------------------------------------------------
void s11::send_msg(
    EndPoint& remote_endpoint, teid_t remote_teid, teid_t local_teid,
    gtpv2c::gtpv2c_create_session_request gtp_ies, uint64_t gtpc_tx_id) {
  send_initial_message(
      remote_endpoint, remote_teid, local_teid, gtp_ies, TASK_S11, gtpc_tx_id);
}
//------------------------------------------------------------------------------
void s11::send_msg(
    EndPoint& remote_endpoint, teid_t remote_teid, teid_t local_teid,
    gtpv2c::gtpv2c_delete_session_request gtp_ies, uint64_t gtpc_tx_id) {
  send_initial_message(
      remote_endpoint, remote_teid, local_teid, gtp_ies, TASK_S11, gtpc_tx_id);
}
//------------------------------------------------------------------------------
void s11::send_msg(
    EndPoint& remote_endpoint, teid_t remote_teid, teid_t local_teid,
    gtpv2c::gtpv2c_modify_bearer_request gtp_ies, uint64_t gtpc_tx_id) {
  send_initial_message(
      remote_endpoint, remote_teid, local_teid, gtp_ies, TASK_S11, gtpc_tx_id);
}
//------------------------------------------------------------------------------
void s11::send_msg(
    EndPoint& remote_endpoint, teid_t remote_teid, teid_t local_teid,
    gtpv2c::gtpv2c_release_access_bearers_request gtp_ies,
    uint64_t gtpc_tx_id) {
  send_initial_message(
      remote_endpoint, remote_teid, local_teid, gtp_ies, TASK_S11, gtpc_tx_id);
}
//------------------------------------------------------------------------------
void s11::send_msg(
    EndPoint& remote_endpoint, teid_t local_teid,
    gtpv2c::gtpv2c_downlink_data_notification_acknowledge gtp_ies,
    uint64_t gtpc_tx_id) {
  send_triggered_message(local_teid, gtp_ies, gtpc_tx_id, CONTINUE_TX);
}
//------------------------------------------------------------------------------
void s11::handle_receive_create_session_response(
    gtpv2c_msg& msg, const endpoint& remote_endpoint) {
  bool error                                       = true;
  uint64_t gtpc_tx_id                              = 0;
  gtpv2c_create_session_response msg_ies_container = {};
  msg.to_core_type(msg_ies_container);

  handle_receive_message_cb(msg, remote_endpoint, TASK_S11, error, gtpc_tx_id);
  if (!error) {
    itti_s5s8_create_session_response* itti_msg =
        new itti_s5s8_create_session_response(TASK_S11, TASK_SGWC_APP);
    itti_msg->gtp_ies    = msg_ies_container;
    itti_msg->r_endpoint = remote_endpoint;
    itti_msg->gtpc_tx_id = gtpc_tx_id;
    itti_msg->teid       = msg.get_teid();
    std::shared_ptr<itti_s5s8_create_session_response> i =
        std::shared_ptr<itti_s5s8_create_session_response>(itti_msg);
    int ret = itti_inst->send_msg(i);
    if (RETURNok != ret) {
      OAILOG_ERROR(
          LOG_S11, "Could not send ITTI message %s to task TASK_SGWC_APP",
          i->get_msg_name());
    }
  }
  // else ignore
}
//------------------------------------------------------------------------------
void s11::handle_receive_modify_bearer_response(
    gtpv2c_msg& msg, const endpoint& remote_endpoint) {
  bool error                                      = true;
  uint64_t gtpc_tx_id                             = 0;
  gtpv2c_modify_bearer_response msg_ies_container = {};
  msg.to_core_type(msg_ies_container);

  handle_receive_message_cb(msg, remote_endpoint, TASK_S11, error, gtpc_tx_id);
  if (!error) {
    itti_s5s8_modify_bearer_response* itti_msg =
        new itti_s5s8_modify_bearer_response(TASK_S11, TASK_SGWC_APP);
    itti_msg->gtp_ies    = msg_ies_container;
    itti_msg->r_endpoint = remote_endpoint;
    itti_msg->gtpc_tx_id = gtpc_tx_id;
    itti_msg->teid       = msg.get_teid();
    std::shared_ptr<itti_s5s8_modify_bearer_response> i =
        std::shared_ptr<itti_s5s8_modify_bearer_response>(itti_msg);
    int ret = itti_inst->send_msg(i);
    if (RETURNok != ret) {
      OAILOG_ERROR(
          LOG_S11, "Could not send ITTI message %s to task TASK_SGWC_APP",
          i->get_msg_name());
    }
  }
  // else ignore
}
//------------------------------------------------------------------------------
void s11::handle_receive_release_access_bearers_response(
    gtpv2c_msg& msg, const endpoint& remote_endpoint) {
  bool error                                               = true;
  uint64_t gtpc_tx_id                                      = 0;
  gtpv2c_release_access_bearers_response msg_ies_container = {};
  msg.to_core_type(msg_ies_container);

  handle_receive_message_cb(msg, remote_endpoint, TASK_S11, error, gtpc_tx_id);
  if (!error) {
    itti_s5s8_release_access_bearers_response* itti_msg =
        new itti_s5s8_release_access_bearers_response(TASK_S11, TASK_SGWC_APP);
    itti_msg->gtp_ies    = msg_ies_container;
    itti_msg->r_endpoint = remote_endpoint;
    itti_msg->gtpc_tx_id = gtpc_tx_id;
    itti_msg->teid       = msg.get_teid();
    std::shared_ptr<itti_s5s8_release_access_bearers_response> i =
        std::shared_ptr<itti_s5s8_release_access_bearers_response>(itti_msg);
    int ret = itti_inst->send_msg(i);
    if (RETURNok != ret) {
      OAILOG_ERROR(
          LOG_S11, "Could not send ITTI message %s to task TASK_SGWC_APP",
          i->get_msg_name());
    }
  }
  // else ignore
}
//------------------------------------------------------------------------------
void s11::handle_receive_delete_session_response(
    gtpv2c_msg& msg, const endpoint& remote_endpoint) {
  bool error                                       = true;
  uint64_t gtpc_tx_id                              = 0;
  gtpv2c_delete_session_response msg_ies_container = {};
  msg.to_core_type(msg_ies_container);

  handle_receive_message_cb(msg, remote_endpoint, TASK_S11, error, gtpc_tx_id);
  if (!error) {
    itti_s5s8_delete_session_response* itti_msg =
        new itti_s5s8_delete_session_response(TASK_S11, TASK_SGWC_APP);
    itti_msg->gtp_ies    = msg_ies_container;
    itti_msg->r_endpoint = remote_endpoint;
    itti_msg->gtpc_tx_id = gtpc_tx_id;
    itti_msg->teid       = msg.get_teid();
    std::shared_ptr<itti_s5s8_delete_session_response> i =
        std::shared_ptr<itti_s5s8_delete_session_response>(itti_msg);
    int ret = itti_inst->send_msg(i);
    if (RETURNok != ret) {
      OAILOG_ERROR(
          LOG_S11, "Could not send ITTI message %s to task TASK_SGWC_APP",
          i->get_msg_name());
    }
  }
  // else ignore
}
//------------------------------------------------------------------------------
void s11::handle_receive_downlink_data_notification(
    gtpv2c::gtpv2c_msg& msg, const endpoint& remote_endpoint) {
  bool error                                          = true;
  uint64_t gtpc_tx_id                                 = 0;
  gtpv2c_downlink_data_notification msg_ies_container = {};
  msg.to_core_type(msg_ies_container);

  handle_receive_message_cb(msg, remote_endpoint, TASK_S11, error, gtpc_tx_id);
  if (!error) {
    itti_s5s8_downlink_data_notification* itti_msg =
        new itti_s5s8_downlink_data_notification(TASK_S11, TASK_SGWC_APP);
    itti_msg->gtp_ies    = msg_ies_container;
    itti_msg->r_endpoint = remote_endpoint;
    itti_msg->gtpc_tx_id = gtpc_tx_id;
    itti_msg->teid       = msg.get_teid();
    std::shared_ptr<itti_s5s8_downlink_data_notification> i =
        std::shared_ptr<itti_s5s8_downlink_data_notification>(itti_msg);
    int ret = itti_inst->send_msg(i);
    if (RETURNok != ret) {
      OAILOG_ERROR(
          LOG_S11, "Could not send ITTI message %s to task TASK_SGWC_APP",
          i->get_msg_name());
    }
  }
  // else ignore
}

//------------------------------------------------------------------------------
void s11::handle_receive_gtpv2c_msg(
    gtpv2c_msg& msg, const endpoint& remote_endpoint) {
  // Logger::sgwc_s5s8().trace( "handle_receive_gtpv2c_msg msg type %d length
  // %d", msg.get_message_type(), msg.get_message_length());
  switch (msg.get_message_type()) {
    case GTP_CREATE_SESSION_REQUEST: {
    } break;
    case GTP_ECHO_REQUEST:
    case GTP_ECHO_RESPONSE:
    case GTP_VERSION_NOT_SUPPORTED_INDICATION:
    case GTP_CREATE_SESSION_RESPONSE:
      handle_receive_create_session_response(msg, remote_endpoint);
      break;
    case GTP_MODIFY_BEARER_REQUEST: {
    } break;
    case GTP_MODIFY_BEARER_RESPONSE:
      handle_receive_modify_bearer_response(msg, remote_endpoint);
      break;
    case GTP_DELETE_SESSION_REQUEST: {
      // handle_receive_delete_session_request(msg, remote_endpoint);
    } break;
    case GTP_DELETE_SESSION_RESPONSE: {
      handle_receive_delete_session_response(msg, remote_endpoint);
    } break;
    case GTP_RELEASE_ACCESS_BEARERS_RESPONSE: {
      handle_receive_release_access_bearers_response(msg, remote_endpoint);
    } break;
    case GTP_DOWNLINK_DATA_NOTIFICATION: {
      handle_receive_downlink_data_notification(msg, remote_endpoint);
    } break;

    case GTP_CHANGE_NOTIFICATION_REQUEST:
    case GTP_CHANGE_NOTIFICATION_RESPONSE:
    case GTP_REMOTE_UE_REPORT_NOTIFICATION:
    case GTP_REMOTE_UE_REPORT_ACKNOWLEDGE:
    case GTP_MODIFY_BEARER_COMMAND:
    case GTP_MODIFY_BEARER_FAILURE_INDICATION:
    case GTP_DELETE_BEARER_COMMAND:
    case GTP_DELETE_BEARER_FAILURE_INDICATION:
    case GTP_BEARER_RESOURCE_COMMAND:
    case GTP_BEARER_RESOURCE_FAILURE_INDICATION:
    case GTP_DOWNLINK_DATA_NOTIFICATION_FAILURE_INDICATION:
    case GTP_TRACE_SESSION_ACTIVATION:
    case GTP_TRACE_SESSION_DEACTIVATION:
    case GTP_STOP_PAGING_INDICATION:
    case GTP_CREATE_BEARER_REQUEST:
    case GTP_CREATE_BEARER_RESPONSE:
    case GTP_UPDATE_BEARER_REQUEST:
    case GTP_UPDATE_BEARER_RESPONSE:
    case GTP_DELETE_BEARER_REQUEST:
    case GTP_DELETE_BEARER_RESPONSE:
    case GTP_DELETE_PDN_CONNECTION_SET_REQUEST:
    case GTP_DELETE_PDN_CONNECTION_SET_RESPONSE:
    case GTP_PGW_DOWNLINK_TRIGGERING_NOTIFICATION:
    case GTP_PGW_DOWNLINK_TRIGGERING_ACKNOWLEDGE:
    case GTP_IDENTIFICATION_REQUEST:
    case GTP_IDENTIFICATION_RESPONSE:
    case GTP_CONTEXT_REQUEST:
    case GTP_CONTEXT_RESPONSE:
    case GTP_CONTEXT_ACKNOWLEDGE:
    case GTP_FORWARD_RELOCATION_REQUEST:
    case GTP_FORWARD_RELOCATION_RESPONSE:
    case GTP_FORWARD_RELOCATION_COMPLETE_NOTIFICATION:
    case GTP_FORWARD_RELOCATION_COMPLETE_ACKNOWLEDGE:
    case GTP_FORWARD_ACCESS_CONTEXT_NOTIFICATION:
    case GTP_FORWARD_ACCESS_CONTEXT_ACKNOWLEDGE:
    case GTP_RELOCATION_CANCEL_REQUEST:
    case GTP_RELOCATION_CANCEL_RESPONSE:
    case GTP_CONFIGURATION_TRANSFER_TUNNEL_MESSAGE:
    case GTP_DETACH_NOTIFICATION:
    case GTP_DETACH_ACKNOWLEDGE:
    case GTP_CS_PAGING_INDICATION:
    case GTP_RAN_INFORMATION_RELAY:
    case GTP_ALERT_MME_NOTIFICATION:
    case GTP_ALERT_MME_ACKNOWLEDGE:
    case GTP_UE_ACTIVITY_NOTIFICATION:
    case GTP_UE_ACTIVITY_ACKNOWLEDGE:
    case GTP_ISR_STATUS_INDICATION:
    case GTP_UE_REGISTRATION_QUERY_REQUEST:
    case GTP_UE_REGISTRATION_QUERY_RESPONSE:
    case GTP_CREATE_FORWARDING_TUNNEL_REQUEST:
    case GTP_CREATE_FORWARDING_TUNNEL_RESPONSE:
    case GTP_SUSPEND_NOTIFICATION:
    case GTP_SUSPEND_ACKNOWLEDGE:
    case GTP_RESUME_NOTIFICATION:
    case GTP_RESUME_ACKNOWLEDGE:
    case GTP_CREATE_INDIRECT_DATA_FORWARDING_TUNNEL_REQUEST:
    case GTP_CREATE_INDIRECT_DATA_FORWARDING_TUNNEL_RESPONSE:
    case GTP_DELETE_INDIRECT_DATA_FORWARDING_TUNNEL_REQUEST:
    case GTP_DELETE_INDIRECT_DATA_FORWARDING_TUNNEL_RESPONSE:
    case GTP_RELEASE_ACCESS_BEARERS_REQUEST:
    case GTP_DOWNLINK_DATA_NOTIFICATION_ACKNOWLEDGE:
    case GTP_PGW_RESTART_NOTIFICATION:
    case GTP_PGW_RESTART_NOTIFICATION_ACKNOWLEDGE:
    case GTP_UPDATE_PDN_CONNECTION_SET_REQUEST:
    case GTP_UPDATE_PDN_CONNECTION_SET_RESPONSE:
    case GTP_MODIFY_ACCESS_BEARERS_REQUEST:
    case GTP_MODIFY_ACCESS_BEARERS_RESPONSE:
    case GTP_MBMS_SESSION_START_REQUEST:
    case GTP_MBMS_SESSION_START_RESPONSE:
    case GTP_MBMS_SESSION_UPDATE_REQUEST:
    case GTP_MBMS_SESSION_UPDATE_RESPONSE:
    case GTP_MBMS_SESSION_STOP_RESPONSE:
      break;
    default:
      OAILOG_ERROR(
          LOG_S11, "handle_receive_gtpv2c_msg msg length %d",
          msg.get_message_length());
  }
}
//------------------------------------------------------------------------------
void s11::handle_receive(
    char* recv_buffer, const std::size_t bytes_transferred,
    const endpoint& remote_endpoint) {
  // OAILOG_INFO(LOG_S11, "handle_receive(%d bytes)", bytes_transferred);
  // std::cout << string_to_hex(recv_buffer, bytes_transferred) << std::endl;
  std::istringstream iss(std::istringstream::binary);
  iss.rdbuf()->pubsetbuf(recv_buffer, bytes_transferred);
  gtpv2c_msg msg  = {};
  msg.remote_port = remote_endpoint.port();
  try {
    msg.load_from(iss);
    handle_receive_gtpv2c_msg(msg, remote_endpoint);
  } catch (gtpc_exception& e) {
    OAILOG_INFO(LOG_S11, "handle_receive exception %s", e.what());
  }
}
//------------------------------------------------------------------------------
void s11::notify_ul_error(
    const endpoint& r_endpoint, const teid_t l_teid, const cause_value_e cause,
    const uint64_t gtpc_tx_id) {
  switch (cause) {
    case cause_value_e::REMOTE_PEER_NOT_RESPONDING: {
      itti_s5s8_remote_peer_not_responding* itti_msg =
          new itti_s5s8_remote_peer_not_responding(TASK_S11, TASK_SGWC_APP);
      itti_msg->r_endpoint = r_endpoint;
      itti_msg->gtpc_tx_id = gtpc_tx_id;
      itti_msg->l_teid     = l_teid;
      std::shared_ptr<itti_s5s8_remote_peer_not_responding> i =
          std::shared_ptr<itti_s5s8_remote_peer_not_responding>(itti_msg);
      int ret = itti_inst->send_msg(i);
      if (RETURNok != ret) {
        OAILOG_ERROR(
            LOG_S11, "Could not send ITTI message %s to task TASK_SGWC_APP",
            i->get_msg_name());
      }
    } break;
    default:
      OAILOG_WARNING(LOG_S11, "notify_ul_error cause %d not handled", cause);
  }
}
//------------------------------------------------------------------------------
void s11::time_out_itti_event(const uint32_t timer_id) {
  bool handled = false;
  time_out_event(timer_id, TASK_S11, handled);
  if (!handled) {
    OAILOG_WARNING(LOG_S11, "Timer %d not Found", timer_id);
  }
}
//------------------------------------------------------------------------------
static void mme_s11_exit(void) {
  destroy_task_context(&mme_s11_task_zmq_ctx);
  OAILOG_DEBUG(LOG_S11, "Finished cleaning up MME_S11 task \n");
  OAI_FPRINTF_INFO("TASK_S11 terminated\n");
  pthread_exit(NULL);
}
