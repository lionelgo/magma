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

#include "s11.hpp"
extern "C" {
#include "common_defs.h"
#include "intertask_interface.h"
#include "log.h"
#include "mme_config.h"
}
#include <stdexcept>

using namespace std;

task_zmq_ctx_t s11_task_zmq_ctx;

S11* s11_inst;

void s11_task(void*);

//------------------------------------------------------------------------------
int s11_mme_init(mme_config_t* mme_config_p) {
  int ret = 0;

  OAILOG_DEBUG(LOG_S11, "Initializing S11 interface\n");

  try {
    s11_inst = new S11();

    if (itti_create_task(TASK_S11, &s11_mme_thread, mme_config_p) < 0) {
      OAILOG_ERROR(LOG_S11, "gtpv1u phtread_create: %s\n", strerror(errno));
      goto fail;
    }

    OAILOG_DEBUG(LOG_S11, "Initializing S11 interface: DONE\n");
    return ret;
  } catch (Exception e) {
    OAILOG_DEBUG(LOG_S11, "Initializing S11 interface: FAILURE\n");
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
      s11_mme_create_bearer_response(
          &s11_mme_stack_handle,
          &received_message_p->ittiMsg.s11_create_bearer_response);
    } break;

    case S11_CREATE_SESSION_REQUEST: {
      s11_inst->send_msg(ref(*m));
      ;
    } break;

    case S11_DELETE_SESSION_REQUEST: {
      s11_mme_delete_session_request(
          &s11_mme_stack_handle,
          &received_message_p->ittiMsg.s11_delete_session_request);
    } break;

    case S11_DELETE_BEARER_COMMAND: {
      s11_mme_delete_bearer_command(
          &s11_mme_stack_handle,
          &received_message_p->ittiMsg.s11_delete_bearer_command);
    } break;

    case S11_MODIFY_BEARER_REQUEST: {
      s11_mme_modify_bearer_request(
          &s11_mme_stack_handle,
          &received_message_p->ittiMsg.s11_modify_bearer_request);
    } break;

    case S11_RELEASE_ACCESS_BEARERS_REQUEST: {
      s11_mme_release_access_bearers_request(
          &s11_mme_stack_handle,
          &received_message_p->ittiMsg.s11_release_access_bearers_request);
    } break;

    case S11_DOWNLINK_DATA_NOTIFICATION_ACKNOWLEDGE: {
      s11_mme_downlink_data_notification_acknowledge(
          &s11_mme_stack_handle,
          &received_message_p->ittiMsg
               .s11_downlink_data_notification_acknowledge);
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

    case UDP_DATA_IND: {
      /*
       * We received new data to handle from the UDP layer
       */
      nw_rc_t rc;
      udp_data_ind_t* udp_data_ind;

      udp_data_ind = &received_message_p->ittiMsg.udp_data_ind;
      rc           = nwGtpv2cProcessUdpReq(
          s11_mme_stack_handle, udp_data_ind->msgBuf,
          udp_data_ind->buffer_length, udp_data_ind->local_port,
          udp_data_ind->peer_port, (struct sockaddr*) &udp_data_ind->sock_addr);
      DevAssert(rc == NW_OK);
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

void s11_task(void* args_p) {
  const task_id_t task_id = TASK_S11;
  itti_inst->notify_task_ready(task_id);

  do {
    std::shared_ptr<itti_msg> shared_msg = itti_inst->receive_msg(task_id);
    auto* msg                            = shared_msg.get();
    switch (msg->msg_type) {
      case S5S8_CREATE_SESSION_REQUEST:
        if (itti_s5s8_create_session_request* m =
                dynamic_cast<itti_s5s8_create_session_request*>(msg)) {
          s11_inst->send_msg(ref(*m));
        }
        break;

      case S5S8_MODIFY_BEARER_REQUEST:
        if (itti_s5s8_modify_bearer_request* m =
                dynamic_cast<itti_s5s8_modify_bearer_request*>(msg)) {
          s11_inst->send_msg(ref(*m));
        }
        break;

      case S5S8_RELEASE_ACCESS_BEARERS_REQUEST:
        if (itti_s5s8_release_access_bearers_request* m =
                dynamic_cast<itti_s5s8_release_access_bearers_request*>(msg)) {
          s11_inst->send_msg(ref(*m));
        }
        break;

      case S5S8_DELETE_SESSION_REQUEST:
        if (itti_s5s8_delete_session_request* m =
                dynamic_cast<itti_s5s8_delete_session_request*>(msg)) {
          s11_inst->send_msg(ref(*m));
        }
        break;

      case S5S8_DOWNLINK_DATA_NOTIFICATION_ACKNOWLEDGE:
        if (itti_s5s8_downlink_data_notification_acknowledge* m =
                dynamic_cast<itti_s5s8_downlink_data_notification_acknowledge*>(
                    msg)) {
          s11_inst->send_msg(ref(*m));
        }
        break;

      case TIME_OUT:
        if (itti_msg_timeout* to = dynamic_cast<itti_msg_timeout*>(msg)) {
          OAILOG_DEBUG(LOG_S11, "TIME-OUT event timer id %d", to->timer_id);
          s11_inst->time_out_itti_event(to->timer_id);
        }
        break;

      case TERMINATE:
        if (itti_msg_terminate* terminate =
                dynamic_cast<itti_msg_terminate*>(msg)) {
          OAILOG_INFO(LOG_S11, "Received terminate message");
          return;
        }
        break;

      case HEALTH_PING:
        break;

      default:
        OAILOG_INFO(LOG_S11, "no handler for msg type %d", msg->msg_type);
    }
  } while (true);
}

//------------------------------------------------------------------------------
s11::s11()
    : gtpv2c_stack(
          mme_config.gtpv2c_config.t3_ms, mme_config.gtpv2c_config.n3,
          string(inet_ntoa(pgwc::pgw_config::s11_.iface.addr4)),
          mme_config.gtpv2c_config.port) {
  Logger::sgwc_s5s8().startup("Starting...");
  if (itti_inst->create_task(TASK_S11, s11_task, nullptr)) {
    OAILOG_ERROR(LOG_S11, "Cannot create task TASK_S11");
    throw std::runtime_error("Cannot create task TASK_S11");
  }
  Logger::sgwc_s5s8().startup("Started");
}
//------------------------------------------------------------------------------
void s11::send_msg(itti_s5s8_create_session_request& i) {
  send_initial_message(
      i.r_endpoint, i.teid, i.l_teid, i.gtp_ies, TASK_S11, i.gtpc_tx_id);
}
//------------------------------------------------------------------------------
void s11::send_msg(itti_s5s8_delete_session_request& i) {
  send_initial_message(
      i.r_endpoint, i.teid, i.l_teid, i.gtp_ies, TASK_S11, i.gtpc_tx_id);
}
//------------------------------------------------------------------------------
void s11::send_msg(itti_s5s8_modify_bearer_request& i) {
  send_initial_message(
      i.r_endpoint, i.teid, i.l_teid, i.gtp_ies, TASK_S11, i.gtpc_tx_id);
}
//------------------------------------------------------------------------------
void s11::send_msg(itti_s5s8_release_access_bearers_request& i) {
  send_initial_message(
      i.r_endpoint, i.teid, i.l_teid, i.gtp_ies, TASK_S11, i.gtpc_tx_id);
}
//------------------------------------------------------------------------------
void s11::send_msg(itti_s5s8_downlink_data_notification_acknowledge& i) {
  send_triggered_message(
      i.r_endpoint, i.teid, i.gtp_ies, i.gtpc_tx_id, CONTINUE_TX);
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
