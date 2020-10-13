/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the terms found in the LICENSE file in the root of this source tree.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*! \file s1ap_mme_handlers.c
  \brief
  \author Sebastien ROUX, Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/types.h>

#include "bstrlib.h"
#include "hashtable.h"
#include "log.h"
#include "assertions.h"
#include "conversions.h"
#include "intertask_interface.h"
#include "timer.h"
#include "dynamic_memory_check.h"
#include "mme_config.h"
#include "s1ap_common.h"
#include "s1ap_mme_encoder.h"
#include "s1ap_mme_nas_procedures.h"
#include "s1ap_mme_itti_messaging.h"
#include "s1ap_mme.h"
#include "s1ap_mme_ta.h"
#include "s1ap_mme_handlers.h"
#include "mme_app_statistics.h"
#include "mme_events.h"
#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "3gpp_36.401.h"
#include "3gpp_36.413.h"
#include "BIT_STRING.h"
#include "INTEGER.h"
#include "S1ap_S1AP-PDU.h"
#include "S1ap_CNDomain.h"
#include "S1ap_CauseMisc.h"
#include "S1ap_CauseNas.h"
#include "S1ap_CauseProtocol.h"
#include "S1ap_CauseRadioNetwork.h"
#include "S1ap_CauseTransport.h"
#include "S1ap_E-RABItem.h"
#include "S1ap_E-RABSetupItemBearerSURes.h"
#include "S1ap_E-RABSetupItemCtxtSURes.h"
#include "S1ap_ENB-ID.h"
#include "S1ap_ENB-UE-S1AP-ID.h"
#include "S1ap_ENBname.h"
#include "S1ap_GTP-TEID.h"
#include "S1ap_Global-ENB-ID.h"
#include "S1ap_LAI.h"
#include "S1ap_MME-Code.h"
#include "S1ap_MME-Group-ID.h"
#include "S1ap_MME-UE-S1AP-ID.h"
#include "S1ap_PLMNidentity.h"
#include "S1ap_ProcedureCode.h"
#include "S1ap_ResetType.h"
#include "S1ap_S-TMSI.h"
#include "S1ap_ServedGUMMEIsItem.h"
#include "S1ap_ServedGroupIDs.h"
#include "S1ap_ServedMMECs.h"
#include "S1ap_ServedPLMNs.h"
#include "S1ap_TAI.h"
#include "S1ap_TAIItem.h"
#include "S1ap_TimeToWait.h"
#include "S1ap_TransportLayerAddress.h"
#include "S1ap_UE-S1AP-ID-pair.h"
#include "S1ap_UE-S1AP-IDs.h"
#include "S1ap_UE-associatedLogicalS1-ConnectionItem.h"
#include "S1ap_UE-associatedLogicalS1-ConnectionListRes.h"
#include "S1ap_UEAggregateMaximumBitrate.h"
#include "S1ap_UEPagingID.h"
#include "S1ap_UERadioCapability.h"
#include "asn_SEQUENCE_OF.h"
#include "common_defs.h"
#include "intertask_interface_types.h"
#include "itti_types.h"
#include "mme_app_messages_types.h"
#include "service303.h"
#include "s1ap_state.h"

struct S1ap_E_RABItem_s;
struct S1ap_E_RABSetupItemBearerSURes_s;
struct S1ap_E_RABSetupItemCtxtSURes_s;
struct S1ap_IE;

int s1ap_generate_s1_setup_response(
    s1ap_state_t* state, enb_description_t* enb_association);

int s1ap_mme_generate_ue_context_release_command(
    s1ap_state_t* state, ue_description_t* ue_ref_p, enum s1cause,
    imsi64_t imsi64);

bool is_all_erabId_same(S1ap_PathSwitchRequest_t* container);

/* Handlers matrix. Only mme related procedures present here.
 */
s1ap_message_handler_t message_handlers[][3] = {
    {0, 0, 0}, /* HandoverPreparation */
    {0, 0, 0}, /* HandoverResourceAllocation */
    {0, 0, 0}, /* HandoverNotification */
    {s1ap_mme_handle_path_switch_request, 0, 0}, /* PathSwitchRequest */
    {0, 0, 0},                                   /* HandoverCancel */
    {0, s1ap_mme_handle_erab_setup_response,
     s1ap_mme_handle_erab_setup_failure},          /* E_RABSetup */
    {0, 0, 0},                                     /* E_RABModify */
    {0, s1ap_mme_handle_erab_release_response, 0}, /* E_RABRelease */
    {0, 0, 0},                                     /* E_RABReleaseIndication */
    {0, s1ap_mme_handle_initial_context_setup_response,
     s1ap_mme_handle_initial_context_setup_failure}, /* InitialContextSetup */
    {0, 0, 0},                                       /* Paging */
    {0, 0, 0},                                       /* downlinkNASTransport */
    {s1ap_mme_handle_initial_ue_message, 0, 0},      /* initialUEMessage */
    {s1ap_mme_handle_uplink_nas_transport, 0, 0},    /* uplinkNASTransport */
    {s1ap_mme_handle_enb_reset, 0, 0},               /* Reset */
    {s1ap_mme_handle_error_ind_message, 0, 0},       /* ErrorIndication */
    {s1ap_mme_handle_nas_non_delivery, 0, 0}, /* NASNonDeliveryIndication */
    {s1ap_mme_handle_s1_setup_request, 0, 0}, /* S1Setup */
    {s1ap_mme_handle_ue_context_release_request, 0,
     0},       /* UEContextReleaseRequest */
    {0, 0, 0}, /* DownlinkS1cdma2000tunneling */
    {0, 0, 0}, /* UplinkS1cdma2000tunneling */
    {0, s1ap_mme_handle_ue_context_modification_response,
     s1ap_mme_handle_ue_context_modification_failure}, /* UEContextModification
                                                        */
    {s1ap_mme_handle_ue_cap_indication, 0, 0}, /* UECapabilityInfoIndication */
    {s1ap_mme_handle_ue_context_release_request,
     s1ap_mme_handle_ue_context_release_complete, 0}, /* UEContextRelease */
    {0, 0, 0},                                        /* eNBStatusTransfer */
    {0, 0, 0},                                        /* MMEStatusTransfer */
    {0, 0, 0},                                        /* DeactivateTrace */
    {0, 0, 0},                                        /* TraceStart */
    {0, 0, 0}, /* TraceFailureIndication */
    {0, 0, 0}, /* ENBConfigurationUpdate */
    {0, 0, 0}, /* MMEConfigurationUpdate */
    {0, 0, 0}, /* LocationReportingControl */
    {0, 0, 0}, /* LocationReportingFailureIndication */
    {0, 0, 0}, /* LocationReport */
    {0, 0, 0}, /* OverloadStart */
    {0, 0, 0}, /* OverloadStop */
    {0, 0, 0}, /* WriteReplaceWarning */
    {0, 0, 0}, /* eNBDirectInformationTransfer */
    {0, 0, 0}, /* MMEDirectInformationTransfer */
    {0, 0, 0}, /* PrivateMessage */
    {s1ap_mme_handle_enb_configuration_transfer, 0,
     0},       /* eNBConfigurationTransfer */
    {0, 0, 0}, /* MMEConfigurationTransfer */
    {0, 0, 0}, /* CellTrafficTrace */
               // UPDATE RELEASE 9
    {0, 0, 0}, /* Kill */
    {0, 0, 0}, /* DownlinkUEAssociatedLPPaTransport  */
    {0, 0, 0}, /* UplinkUEAssociatedLPPaTransport */
    {0, 0, 0}, /* DownlinkNonUEAssociatedLPPaTransport */
    {0, 0, 0}, /* UplinkNonUEAssociatedLPPaTransport */
};

int s1ap_mme_handle_message(
    s1ap_state_t* state, const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream, S1ap_S1AP_PDU_t* pdu) {
  /*
   * Checking procedure Code and direction of pdu
   */
  if (pdu->choice.initiatingMessage.procedureCode >=
          COUNT_OF(message_handlers) ||
      pdu->present > S1ap_S1AP_PDU_PR_unsuccessfulOutcome) {
    OAILOG_DEBUG(
        LOG_S1AP,
        "[SCTP %d] Either procedureCode %d or direction %d exceed expected\n",
        assoc_id, (int) pdu->choice.initiatingMessage.procedureCode,
        (int) pdu->present);
    return -1;
  }

  s1ap_message_handler_t handler =
      message_handlers[pdu->choice.initiatingMessage.procedureCode]
                      [pdu->present - 1];

  if (handler == NULL) {
    // not implemented or no procedure for eNB (wrong message)
    OAILOG_DEBUG(
        LOG_S1AP, "[SCTP %d] No handler for procedureCode %d in %s\n", assoc_id,
        (int) pdu->choice.initiatingMessage.procedureCode,
        s1ap_direction2str(pdu->present));
    return -2;
  }

  return handler(state, assoc_id, stream, pdu);
}

//------------------------------------------------------------------------------
int s1ap_mme_set_cause(
    S1ap_Cause_t* cause_p, const S1ap_Cause_PR cause_type,
    const long cause_value) {
  DevAssert(cause_p != NULL);
  cause_p->present = cause_type;

  switch (cause_type) {
    case S1ap_Cause_PR_radioNetwork:
      cause_p->choice.misc = cause_value;
      break;

    case S1ap_Cause_PR_transport:
      cause_p->choice.transport = cause_value;
      break;

    case S1ap_Cause_PR_nas:
      cause_p->choice.nas = cause_value;
      break;

    case S1ap_Cause_PR_protocol:
      cause_p->choice.protocol = cause_value;
      break;

    case S1ap_Cause_PR_misc:
      cause_p->choice.misc = cause_value;
      break;

    default:
      return -1;
  }

  return 0;
}

//------------------------------------------------------------------------------
int s1ap_mme_generate_s1_setup_failure(
    const sctp_assoc_id_t assoc_id, const S1ap_Cause_PR cause_type,
    const long cause_value, const long time_to_wait) {
  uint8_t* buffer_p = 0;
  uint32_t length   = 0;
  S1ap_S1AP_PDU_t pdu;
  S1ap_S1SetupFailure_t* out;
  S1ap_S1SetupFailureIEs_t* ie = NULL;
  int rc                       = RETURNok;

  OAILOG_FUNC_IN(LOG_S1AP);

  memset(&pdu, 0, sizeof(pdu));
  pdu.present = S1ap_S1AP_PDU_PR_unsuccessfulOutcome;
  pdu.choice.unsuccessfulOutcome.procedureCode = S1ap_ProcedureCode_id_S1Setup;
  pdu.choice.unsuccessfulOutcome.criticality   = S1ap_Criticality_reject;
  pdu.choice.unsuccessfulOutcome.value.present =
      S1ap_UnsuccessfulOutcome__value_PR_S1SetupFailure;
  out = &pdu.choice.unsuccessfulOutcome.value.choice.S1SetupFailure;

  ie = (S1ap_S1SetupFailureIEs_t*) calloc(1, sizeof(S1ap_S1SetupFailureIEs_t));
  ie->id            = S1ap_ProtocolIE_ID_id_Cause;
  ie->criticality   = S1ap_Criticality_ignore;
  ie->value.present = S1ap_S1SetupFailureIEs__value_PR_Cause;
  s1ap_mme_set_cause(&ie->value.choice.Cause, cause_type, cause_value);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /*
   * Include the optional field time to wait only if the value is > -1
   */
  if (time_to_wait > -1) {
    ie =
        (S1ap_S1SetupFailureIEs_t*) calloc(1, sizeof(S1ap_S1SetupFailureIEs_t));
    ie->id                      = S1ap_ProtocolIE_ID_id_TimeToWait;
    ie->criticality             = S1ap_Criticality_ignore;
    ie->value.present           = S1ap_S1SetupFailureIEs__value_PR_TimeToWait;
    ie->value.choice.TimeToWait = time_to_wait;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  if (s1ap_mme_encode_pdu(&pdu, &buffer_p, &length) < 0) {
    OAILOG_ERROR(LOG_S1AP, "Failed to encode s1 setup failure\n");
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }

  bstring b = blk2bstr(buffer_p, length);
  free(buffer_p);
  rc = s1ap_mme_itti_send_sctp_request(&b, assoc_id, 0, INVALID_MME_UE_S1AP_ID);
  OAILOG_FUNC_RETURN(LOG_S1AP, rc);
}

////////////////////////////////////////////////////////////////////////////////
//************************** Management procedures ***************************//
////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
int s1ap_mme_handle_s1_setup_request(
    s1ap_state_t* state, const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream, S1ap_S1AP_PDU_t* pdu) {
  int rc = RETURNok;

  S1ap_S1SetupRequest_t* container                = NULL;
  S1ap_S1SetupRequestIEs_t* ie                    = NULL;
  S1ap_S1SetupRequestIEs_t* ie_enb_name           = NULL;
  S1ap_S1SetupRequestIEs_t* ie_supported_tas      = NULL;
  S1ap_S1SetupRequestIEs_t* ie_default_paging_drx = NULL;

  enb_description_t* enb_association = NULL;
  uint32_t enb_id                    = 0;
  char* enb_name                     = NULL;
  int ta_ret                         = 0;
  uint8_t bplmn_list_count           = 0;  // Broadcast PLMN list count

  OAILOG_FUNC_IN(LOG_S1AP);
  increment_counter("s1_setup", 1, NO_LABELS);
  if (!hss_associated) {
    /*
     * Can not process the request, MME is not connected to HSS
     */
    OAILOG_ERROR(
        LOG_S1AP,
        "Rejecting s1 setup request Can not process the request, MME is not "
        "connected to HSS\n");
    rc = s1ap_mme_generate_s1_setup_failure(
        assoc_id, S1ap_Cause_PR_misc, S1ap_CauseMisc_unspecified, -1);
    increment_counter(
        "s1_setup", 1, 2, "result", "failure", "cause", "s6a_interface_not_up");
    OAILOG_FUNC_RETURN(LOG_S1AP, rc);
  }

  DevAssert(pdu != NULL);
  container = &pdu->choice.initiatingMessage.value.choice.S1SetupRequest;
  /*
   * We received a new valid S1 Setup Request on a stream != 0.
   * This should not happen -> reject eNB s1 setup request.
   */

  if (stream != 0) {
    OAILOG_ERROR(LOG_S1AP, "Received new s1 setup request on stream != 0\n");
    /*
     * Send a s1 setup failure with protocol cause unspecified
     */
    rc = s1ap_mme_generate_s1_setup_failure(
        assoc_id, S1ap_Cause_PR_protocol, S1ap_CauseProtocol_unspecified, -1);
    increment_counter(
        "s1_setup", 1, 2, "result", "failure", "cause",
        "sctp_stream_id_non_zero");
    OAILOG_FUNC_RETURN(LOG_S1AP, rc);
  }

  /* Handling of s1setup cases as follows.
   * If we don't know about the association, we haven't processed the new
   * association yet, so hope the eNB will retry the s1 setup. Ignore and
   * return. If we get this message when the S1 interface of the MME state is in
   * READY state then it is protocol error or out of sync state. Ignore it and
   * return. Assume MME would detect SCTP association failure and would S1
   * interface state to accept S1setup from eNB. If we get this message when the
   * s1 interface of the MME is in SHUTDOWN stage, we just hope the eNB will
   * retry and that will result in a new association getting established
   * followed by a subsequent s1 setup, return S1ap_TimeToWait_v20s. If we get
   * this message when the s1 interface of the MME is in RESETTING stage then we
   * return S1ap_TimeToWait_v20s.
   */
  if ((enb_association = s1ap_state_get_enb(state, assoc_id)) == NULL) {
    /*
     *
     * This should not happen as the thread processing new associations is the
     * one that reads data from the socket. Promote to an assert once we have
     * more confidence.
     */
    OAILOG_ERROR(LOG_S1AP, "Ignoring s1 setup from unknown assoc %u", assoc_id);
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNok);
  }

  if (enb_association->s1_state == S1AP_RESETING ||
      enb_association->s1_state == S1AP_SHUTDOWN) {
    OAILOG_WARNING(
        LOG_S1AP, "Ignoring s1setup from eNB in state %s on assoc id %u",
        s1_enb_state2str(enb_association->s1_state), assoc_id);
    rc = s1ap_mme_generate_s1_setup_failure(
        assoc_id, S1ap_Cause_PR_transport,
        S1ap_CauseTransport_transport_resource_unavailable,
        S1ap_TimeToWait_v20s);
    increment_counter(
        "s1_setup", 1, 2, "result", "failure", "cause", "invalid_state");
    OAILOG_FUNC_RETURN(LOG_S1AP, rc);
  }
  log_queue_item_t* context = NULL;
  OAILOG_MESSAGE_START_SYNC(
      OAILOG_LEVEL_DEBUG, LOG_S1AP, (&context),
      "New s1 setup request incoming from ");
  // shared_log_queue_item_t *context = NULL;
  // OAILOG_MESSAGE_START_ASYNC (OAILOG_LEVEL_DEBUG, LOG_S1AP, (&context), "New
  // s1 setup request incoming from ");

  S1AP_FIND_PROTOCOLIE_BY_ID(
      S1ap_S1SetupRequestIEs_t, ie_enb_name, container,
      S1ap_ProtocolIE_ID_id_eNBname, false);
  if (ie_enb_name) {
    OAILOG_MESSAGE_ADD_SYNC(
        context, "%*s ", (int) ie_enb_name->value.choice.ENBname.size,
        ie_enb_name->value.choice.ENBname.buf);
    enb_name = (char*) ie_enb_name->value.choice.ENBname.buf;
  }

  S1AP_FIND_PROTOCOLIE_BY_ID(
      S1ap_S1SetupRequestIEs_t, ie, container,
      S1ap_ProtocolIE_ID_id_Global_ENB_ID, true);
  if (ie->value.choice.Global_ENB_ID.eNB_ID.present ==
      S1ap_ENB_ID_PR_homeENB_ID) {
    // Home eNB ID = 28 bits
    uint8_t* enb_id_buf =
        ie->value.choice.Global_ENB_ID.eNB_ID.choice.homeENB_ID.buf;

    if (ie->value.choice.Global_ENB_ID.eNB_ID.choice.homeENB_ID.size != 28) {
      // TODO: handle case were size != 28 -> notify ? reject ?
    }

    enb_id = (enb_id_buf[0] << 20) + (enb_id_buf[1] << 12) +
             (enb_id_buf[2] << 4) + ((enb_id_buf[3] & 0xf0) >> 4);
    OAILOG_MESSAGE_ADD_SYNC(context, "home eNB id: %07x", enb_id);
  } else {
    // Macro eNB = 20 bits
    uint8_t* enb_id_buf =
        ie->value.choice.Global_ENB_ID.eNB_ID.choice.macroENB_ID.buf;

    if (ie->value.choice.Global_ENB_ID.eNB_ID.choice.macroENB_ID.size != 20) {
      // TODO: handle case were size != 20 -> notify ? reject ?
    }

    enb_id = (enb_id_buf[0] << 12) + (enb_id_buf[1] << 4) +
             ((enb_id_buf[2] & 0xf0) >> 4);
    OAILOG_MESSAGE_ADD_SYNC(context, "macro eNB id: %05x", enb_id);
  }

  OAILOG_MESSAGE_FINISH((void*) context);

  /* Requirement MME36.413R10_8.7.3.4 Abnormal Conditions
   * If the eNB initiates the procedure by sending a S1 SETUP REQUEST message
   * including the PLMN Identity IEs and none of the PLMNs provided by the eNB
   * is identified by the MME, then the MME shall reject the eNB S1 Setup
   * Request procedure with the appropriate cause value, e.g, Unknown PLMN.
   */
  S1AP_FIND_PROTOCOLIE_BY_ID(
      S1ap_S1SetupRequestIEs_t, ie_supported_tas, container,
      S1ap_ProtocolIE_ID_id_SupportedTAs, true);

  ta_ret =
      s1ap_mme_compare_ta_lists(&ie_supported_tas->value.choice.SupportedTAs);

  /*
   * eNB and MME have no common PLMN
   */
  if (ta_ret != TA_LIST_RET_OK) {
    OAILOG_ERROR(
        LOG_S1AP, "No Common PLMN with eNB, generate_s1_setup_failure\n");
    rc = s1ap_mme_generate_s1_setup_failure(
        assoc_id, S1ap_Cause_PR_misc, S1ap_CauseMisc_unknown_PLMN,
        S1ap_TimeToWait_v20s);

    increment_counter(
        "s1_setup", 1, 2, "result", "failure", "cause",
        "plmnid_or_tac_mismatch");
    OAILOG_FUNC_RETURN(LOG_S1AP, rc);
  }

  S1ap_SupportedTAs_t* ta_list = &ie_supported_tas->value.choice.SupportedTAs;
  supported_ta_list_t* supp_ta_list = &enb_association->supported_ta_list;
  supp_ta_list->list_count          = ta_list->list.count;

  /* Storing supported TAI lists received in S1 SETUP REQUEST message */
  for (int tai_idx = 0; tai_idx < supp_ta_list->list_count; tai_idx++) {
    S1ap_SupportedTAs_Item_t* tai = NULL;
    tai                           = ta_list->list.array[tai_idx];
    OCTET_STRING_TO_TAC(
        &tai->tAC, supp_ta_list->supported_tai_items[tai_idx].tac);

    bplmn_list_count = tai->broadcastPLMNs.list.count;
    if (bplmn_list_count > S1AP_MAX_BROADCAST_PLMNS) {
      OAILOG_ERROR(
          LOG_S1AP, "Maximum Broadcast PLMN list count exceeded, count = %d\n",
          bplmn_list_count);
    }
    supp_ta_list->supported_tai_items[tai_idx].bplmnlist_count =
        bplmn_list_count;
    for (int plmn_idx = 0; plmn_idx < bplmn_list_count; plmn_idx++) {
      TBCD_TO_PLMN_T(
          tai->broadcastPLMNs.list.array[plmn_idx],
          &supp_ta_list->supported_tai_items[tai_idx].bplmns[plmn_idx]);
    }
  }
  OAILOG_DEBUG(LOG_S1AP, "Adding eNB to the list of served eNBs\n");

  enb_association->enb_id = enb_id;

  S1AP_FIND_PROTOCOLIE_BY_ID(
      S1ap_S1SetupRequestIEs_t, ie_default_paging_drx, container,
      S1ap_ProtocolIE_ID_id_DefaultPagingDRX, true);

  enb_association->default_paging_drx =
      ie_default_paging_drx->value.choice.PagingDRX;

  if (enb_name != NULL) {
    memcpy(
        enb_association->enb_name, ie_enb_name->value.choice.ENBname.buf,
        ie_enb_name->value.choice.ENBname.size);
    enb_association->enb_name[ie_enb_name->value.choice.ENBname.size] = '\0';
  }

  s1ap_dump_enb(enb_association);
  rc = s1ap_generate_s1_setup_response(state, enb_association);
  if (rc == RETURNok) {
    update_mme_app_stats_connected_enb_add();
    set_gauge("s1_connection", 1, 1, "enb_name", enb_association->enb_name);
    increment_counter("s1_setup", 1, 1, "result", "success");
    s1_setup_success_event(enb_name, enb_id);
  }
  OAILOG_FUNC_RETURN(LOG_S1AP, rc);
}

//------------------------------------------------------------------------------
int s1ap_generate_s1_setup_response(
    s1ap_state_t* state, enb_description_t* enb_association) {
  S1ap_S1AP_PDU_t pdu;
  S1ap_S1SetupResponse_t* out;
  S1ap_S1SetupResponseIEs_t* ie          = NULL;
  S1ap_ServedGUMMEIsItem_t* servedGUMMEI = NULL;
  int i, j;
  int enc_rval    = 0;
  uint8_t* buffer = NULL;
  uint32_t length = 0;
  int rc          = RETURNok;

  OAILOG_FUNC_IN(LOG_S1AP);
  DevAssert(enb_association != NULL);
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = S1ap_S1AP_PDU_PR_successfulOutcome;
  pdu.choice.successfulOutcome.procedureCode = S1ap_ProcedureCode_id_S1Setup;
  pdu.choice.successfulOutcome.criticality   = S1ap_Criticality_reject;
  pdu.choice.successfulOutcome.value.present =
      S1ap_SuccessfulOutcome__value_PR_S1SetupResponse;
  out = &pdu.choice.successfulOutcome.value.choice.S1SetupResponse;

  // Generating response
  ie =
      (S1ap_S1SetupResponseIEs_t*) calloc(1, sizeof(S1ap_S1SetupResponseIEs_t));
  ie->id            = S1ap_ProtocolIE_ID_id_ServedGUMMEIs;
  ie->criticality   = S1ap_Criticality_reject;
  ie->value.present = S1ap_S1SetupResponseIEs__value_PR_ServedGUMMEIs;

  // memset for gcc 4.8.4 instead of {0}, servedGUMMEI.servedPLMNs
  servedGUMMEI = calloc(1, sizeof *servedGUMMEI);

  mme_config_read_lock(&mme_config);
  /*
   * Use the gummei parameters provided by configuration
   * that should be sorted
   */
  for (i = 0; i < mme_config.served_tai.nb_tai; i++) {
    bool plmn_added = false;
    for (j = 0; j < i; j++) {
      if ((mme_config.served_tai.plmn_mcc[j] ==
           mme_config.served_tai.plmn_mcc[i]) &&
          (mme_config.served_tai.plmn_mnc[j] ==
           mme_config.served_tai.plmn_mnc[i]) &&
          (mme_config.served_tai.plmn_mnc_len[j] ==
           mme_config.served_tai.plmn_mnc_len[i])) {
        plmn_added = true;
        break;
      }
    }
    if (false == plmn_added) {
      S1ap_PLMNidentity_t* plmn = NULL;
      plmn                      = calloc(1, sizeof(*plmn));
      MCC_MNC_TO_PLMNID(
          mme_config.served_tai.plmn_mcc[i], mme_config.served_tai.plmn_mnc[i],
          mme_config.served_tai.plmn_mnc_len[i], plmn);
      ASN_SEQUENCE_ADD(&servedGUMMEI->servedPLMNs.list, plmn);
    }
  }

  for (i = 0; i < mme_config.gummei.nb; i++) {
    S1ap_MME_Group_ID_t* mme_gid = NULL;
    S1ap_MME_Code_t* mmec        = NULL;

    mme_gid = calloc(1, sizeof(*mme_gid));
    INT16_TO_OCTET_STRING(mme_config.gummei.gummei[i].mme_gid, mme_gid);
    ASN_SEQUENCE_ADD(&servedGUMMEI->servedGroupIDs.list, mme_gid);

    mmec = calloc(1, sizeof(*mmec));
    INT8_TO_OCTET_STRING(mme_config.gummei.gummei[i].mme_code, mmec);
    ASN_SEQUENCE_ADD(&servedGUMMEI->servedMMECs.list, mmec);
  }
  ASN_SEQUENCE_ADD(&ie->value.choice.ServedGUMMEIs.list, servedGUMMEI);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  ie =
      (S1ap_S1SetupResponseIEs_t*) calloc(1, sizeof(S1ap_S1SetupResponseIEs_t));
  ie->id            = S1ap_ProtocolIE_ID_id_RelativeMMECapacity;
  ie->criticality   = S1ap_Criticality_ignore;
  ie->value.present = S1ap_S1SetupResponseIEs__value_PR_RelativeMMECapacity;
  ie->value.choice.RelativeMMECapacity = mme_config.relative_capacity;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  mme_config_unlock(&mme_config);
  /*
   * The MME is only serving E-UTRAN RAT, so the list contains only one element
   */
  enc_rval = s1ap_mme_encode_pdu(&pdu, &buffer, &length);

  /*
   * Failed to encode s1 setup response...
   */
  if (enc_rval < 0) {
    OAILOG_DEBUG(LOG_S1AP, "Removed eNB %d\n", enb_association->sctp_assoc_id);
    s1ap_remove_enb(state, enb_association);
  } else {
    /*
     * Consider the response as sent. S1AP is ready to accept UE contexts
     */
    enb_association->s1_state = S1AP_READY;
  }

  /*
   * Non-UE signalling -> stream 0
   */
  bstring b = blk2bstr(buffer, length);
  free(buffer);
  rc = s1ap_mme_itti_send_sctp_request(
      &b, enb_association->sctp_assoc_id, 0, INVALID_MME_UE_S1AP_ID);

  OAILOG_FUNC_RETURN(LOG_S1AP, rc);
}

//------------------------------------------------------------------------------
int s1ap_mme_handle_ue_cap_indication(
    s1ap_state_t* state, __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream, S1ap_S1AP_PDU_t* pdu) {
  ue_description_t* ue_ref_p = NULL;
  S1ap_UECapabilityInfoIndication_t* container;
  S1ap_UECapabilityInfoIndicationIEs_t* ie = NULL;
  int rc                                   = RETURNok;
  mme_ue_s1ap_id_t mme_ue_s1ap_id          = 0;
  enb_ue_s1ap_id_t enb_ue_s1ap_id          = 0;
  imsi64_t imsi64                          = INVALID_IMSI64;

  OAILOG_FUNC_IN(LOG_S1AP);
  DevAssert(pdu != NULL);
  container =
      &pdu->choice.initiatingMessage.value.choice.UECapabilityInfoIndication;

  S1AP_FIND_PROTOCOLIE_BY_ID(
      S1ap_UECapabilityInfoIndicationIEs_t, ie, container,
      S1ap_ProtocolIE_ID_id_MME_UE_S1AP_ID, true);

  if (ie) {
    mme_ue_s1ap_id = ie->value.choice.MME_UE_S1AP_ID;
    if ((ue_ref_p = s1ap_state_get_ue_mmeid(mme_ue_s1ap_id)) == NULL) {
      OAILOG_DEBUG(
          LOG_S1AP,
          "No UE is attached to this mme UE s1ap id: " MME_UE_S1AP_ID_FMT "\n",
          mme_ue_s1ap_id);
      OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
    }
  } else {
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }

  s1ap_imsi_map_t* s1ap_imsi_map = get_s1ap_imsi_map();
  hashtable_uint64_ts_get(
      s1ap_imsi_map->mme_ue_id_imsi_htbl, (const hash_key_t) mme_ue_s1ap_id,
      &imsi64);

  S1AP_FIND_PROTOCOLIE_BY_ID(
      S1ap_UECapabilityInfoIndicationIEs_t, ie, container,
      S1ap_ProtocolIE_ID_id_eNB_UE_S1AP_ID, true);

  if (ie) {
    enb_ue_s1ap_id = (enb_ue_s1ap_id_t)(
        ie->value.choice.ENB_UE_S1AP_ID & ENB_UE_S1AP_ID_MASK);
  } else {
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }

  if (ue_ref_p->enb_ue_s1ap_id != enb_ue_s1ap_id) {
    OAILOG_DEBUG_UE(
        LOG_S1AP, imsi64,
        "Mismatch in eNB UE S1AP ID, known: " ENB_UE_S1AP_ID_FMT
        ", received: " ENB_UE_S1AP_ID_FMT "\n",
        ue_ref_p->enb_ue_s1ap_id, (uint32_t)(uint32_t) enb_ue_s1ap_id);
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }

  /*
   * Just display a warning when message received over wrong stream
   */
  if (ue_ref_p->sctp_stream_recv != stream) {
    OAILOG_ERROR_UE(
        LOG_S1AP, imsi64,
        "Received ue capability indication for "
        "(MME UE S1AP ID/eNB UE S1AP ID) (" MME_UE_S1AP_ID_FMT
        "/" ENB_UE_S1AP_ID_FMT
        ") over wrong stream "
        "expecting %u, received on %u\n",
        (uint32_t) mme_ue_s1ap_id, ue_ref_p->enb_ue_s1ap_id,
        ue_ref_p->sctp_stream_recv, stream);
  }

  /*
   * Forward the ue capabilities to MME application layer
   */
  S1AP_FIND_PROTOCOLIE_BY_ID(
      S1ap_UECapabilityInfoIndicationIEs_t, ie, container,
      S1ap_ProtocolIE_ID_id_UERadioCapability, true);

  if (ie) {
    MessageDef* message_p                = NULL;
    itti_s1ap_ue_cap_ind_t* ue_cap_ind_p = NULL;

    message_p = itti_alloc_new_message(TASK_S1AP, S1AP_UE_CAPABILITIES_IND);
    DevAssert(message_p != NULL);
    ue_cap_ind_p                 = &message_p->ittiMsg.s1ap_ue_cap_ind;
    ue_cap_ind_p->enb_ue_s1ap_id = ue_ref_p->enb_ue_s1ap_id;
    ue_cap_ind_p->mme_ue_s1ap_id = ue_ref_p->mme_ue_s1ap_id;
    ue_cap_ind_p->radio_capabilities_length =
        ie->value.choice.UERadioCapability.size;
    ue_cap_ind_p->radio_capabilities = calloc(
        ue_cap_ind_p->radio_capabilities_length,
        sizeof(*ue_cap_ind_p->radio_capabilities));
    memcpy(
        ue_cap_ind_p->radio_capabilities,
        ie->value.choice.UERadioCapability.buf,
        ue_cap_ind_p->radio_capabilities_length);

    message_p->ittiMsgHeader.imsi = imsi64;
    rc = send_msg_to_task(&s1ap_task_zmq_ctx, TASK_MME_APP, message_p);
    OAILOG_FUNC_RETURN(LOG_S1AP, rc);
  } else {
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }
  OAILOG_FUNC_RETURN(LOG_S1AP, RETURNok);
}

////////////////////////////////////////////////////////////////////////////////
//******************* Context Management procedures **************************//
////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
int s1ap_mme_handle_initial_context_setup_response(
    s1ap_state_t* state, __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    S1ap_S1AP_PDU_t* pdu) {
  S1ap_InitialContextSetupResponse_t* container;
  S1ap_InitialContextSetupResponseIEs_t* ie                   = NULL;
  S1ap_E_RABSetupItemCtxtSUResIEs_t* eRABSetupItemCtxtSURes_p = NULL;
  ue_description_t* ue_ref_p      = NULL;
  MessageDef* message_p           = NULL;
  int rc                          = RETURNok;
  mme_ue_s1ap_id_t mme_ue_s1ap_id = 0;
  enb_ue_s1ap_id_t enb_ue_s1ap_id = 0;
  imsi64_t imsi64;

  OAILOG_FUNC_IN(LOG_S1AP);
  container =
      &pdu->choice.successfulOutcome.value.choice.InitialContextSetupResponse;
  S1AP_FIND_PROTOCOLIE_BY_ID(
      S1ap_InitialContextSetupResponseIEs_t, ie, container,
      S1ap_ProtocolIE_ID_id_MME_UE_S1AP_ID, true);

  if (ie) {
    mme_ue_s1ap_id = ie->value.choice.MME_UE_S1AP_ID;
    if ((ue_ref_p = s1ap_state_get_ue_mmeid((uint32_t) mme_ue_s1ap_id)) ==
        NULL) {
      OAILOG_DEBUG(
          LOG_S1AP,
          "No UE is attached to this mme UE s1ap id: " MME_UE_S1AP_ID_FMT
          " %u(10)\n",
          (uint32_t) mme_ue_s1ap_id, (uint32_t) mme_ue_s1ap_id);
      OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
    }
  } else {
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }
  s1ap_imsi_map_t* s1ap_imsi_map = get_s1ap_imsi_map();
  hashtable_uint64_ts_get(
      s1ap_imsi_map->mme_ue_id_imsi_htbl, (const hash_key_t) mme_ue_s1ap_id,
      &imsi64);

  S1AP_FIND_PROTOCOLIE_BY_ID(
      S1ap_InitialContextSetupResponseIEs_t, ie, container,
      S1ap_ProtocolIE_ID_id_eNB_UE_S1AP_ID, true);
  if (ie) {
    enb_ue_s1ap_id = (enb_ue_s1ap_id_t)(
        ie->value.choice.ENB_UE_S1AP_ID & ENB_UE_S1AP_ID_MASK);
  } else {
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }

  if (ue_ref_p->enb_ue_s1ap_id != enb_ue_s1ap_id) {
    OAILOG_DEBUG_UE(
        LOG_S1AP, imsi64,
        "Mismatch in eNB UE S1AP ID, known: " ENB_UE_S1AP_ID_FMT
        " %u(10), received: 0x%06x %u(10)\n",
        ue_ref_p->enb_ue_s1ap_id, ue_ref_p->enb_ue_s1ap_id,
        (uint32_t) enb_ue_s1ap_id, (uint32_t) enb_ue_s1ap_id);
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }

  S1AP_FIND_PROTOCOLIE_BY_ID(
      S1ap_InitialContextSetupResponseIEs_t, ie, container,
      S1ap_ProtocolIE_ID_id_E_RABSetupListCtxtSURes, true);

  if (ie) {
    if (ie->value.choice.E_RABSetupListCtxtSURes.list.count < 1) {
      OAILOG_WARNING_UE(LOG_S1AP, imsi64, "E-RAB creation has failed\n");
      OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
    }
  } else {
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }
  ue_ref_p->s1_ue_state = S1AP_UE_CONNECTED;
  message_p =
      itti_alloc_new_message(TASK_S1AP, MME_APP_INITIAL_CONTEXT_SETUP_RSP);
  AssertFatal(message_p != NULL, "itti_alloc_new_message Failed");
  MME_APP_INITIAL_CONTEXT_SETUP_RSP(message_p).ue_id = ue_ref_p->mme_ue_s1ap_id;
  MME_APP_INITIAL_CONTEXT_SETUP_RSP(message_p).e_rab_setup_list.no_of_items =
      ie->value.choice.E_RABSetupListCtxtSURes.list.count;
  for (int item = 0; item < ie->value.choice.E_RABSetupListCtxtSURes.list.count;
       item++) {
    /*
     * Bad, very bad cast...
     */
    eRABSetupItemCtxtSURes_p =
        (S1ap_E_RABSetupItemCtxtSUResIEs_t*)
            ie->value.choice.E_RABSetupListCtxtSURes.list.array[item];
    MME_APP_INITIAL_CONTEXT_SETUP_RSP(message_p)
        .e_rab_setup_list.item[item]
        .e_rab_id =
        eRABSetupItemCtxtSURes_p->value.choice.E_RABSetupItemCtxtSURes.e_RAB_ID;
    MME_APP_INITIAL_CONTEXT_SETUP_RSP(message_p)
        .e_rab_setup_list.item[item]
        .gtp_teid = htonl(*((uint32_t*) eRABSetupItemCtxtSURes_p->value.choice
                                .E_RABSetupItemCtxtSURes.gTP_TEID.buf));
    MME_APP_INITIAL_CONTEXT_SETUP_RSP(message_p)
        .e_rab_setup_list.item[item]
        .transport_layer_address = blk2bstr(
        eRABSetupItemCtxtSURes_p->value.choice.E_RABSetupItemCtxtSURes
            .transportLayerAddress.buf,
        eRABSetupItemCtxtSURes_p->value.choice.E_RABSetupItemCtxtSURes
            .transportLayerAddress.size);
  }

  // Failed bearers
  itti_mme_app_initial_context_setup_rsp_t* initial_context_setup_rsp = NULL;
  S1AP_FIND_PROTOCOLIE_BY_ID(
      S1ap_InitialContextSetupResponseIEs_t, ie, container,
      S1ap_ProtocolIE_ID_id_E_RABFailedToSetupListBearerSURes, false);
  if (ie) {
    S1ap_E_RABList_t* s1ap_e_rab_list = &ie->value.choice.E_RABList;
    for (int index = 0; index < s1ap_e_rab_list->list.count; index++) {
      S1ap_E_RABItem_t* erab_item =
          (S1ap_E_RABItem_t*) s1ap_e_rab_list->list.array[index];
      initial_context_setup_rsp->e_rab_failed_to_setup_list
          .item[initial_context_setup_rsp->e_rab_failed_to_setup_list
                    .no_of_items]
          .e_rab_id = erab_item->e_RAB_ID;
      initial_context_setup_rsp->e_rab_failed_to_setup_list
          .item[initial_context_setup_rsp->e_rab_failed_to_setup_list
                    .no_of_items]
          .cause = erab_item->cause;
      initial_context_setup_rsp->e_rab_failed_to_setup_list.no_of_items++;
    }
  }
  message_p->ittiMsgHeader.imsi = imsi64;
  rc = send_msg_to_task(&s1ap_task_zmq_ctx, TASK_MME_APP, message_p);
  OAILOG_FUNC_RETURN(LOG_S1AP, rc);
}

//------------------------------------------------------------------------------
int s1ap_mme_handle_ue_context_release_request(
    s1ap_state_t* state, __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    S1ap_S1AP_PDU_t* pdu) {
  S1ap_UEContextReleaseRequest_t* container;
  S1ap_UEContextReleaseRequest_IEs_t* ie = NULL;
  ue_description_t* ue_ref_p             = NULL;
  MessageDef* message_p                  = NULL;
  S1ap_Cause_PR cause_type;
  long cause_value;
  enum s1cause s1_release_cause   = S1AP_RADIO_EUTRAN_GENERATED_REASON;
  int rc                          = RETURNok;
  mme_ue_s1ap_id_t mme_ue_s1ap_id = 0;
  enb_ue_s1ap_id_t enb_ue_s1ap_id = 0;
  imsi64_t imsi64                 = INVALID_IMSI64;

  OAILOG_FUNC_IN(LOG_S1AP);
  container =
      &pdu->choice.initiatingMessage.value.choice.UEContextReleaseRequest;
  // Log the Cause Type and Cause value
  S1AP_FIND_PROTOCOLIE_BY_ID(
      S1ap_UEContextReleaseRequest_IEs_t, ie, container,
      S1ap_ProtocolIE_ID_id_MME_UE_S1AP_ID, true);
  if (ie) {
    mme_ue_s1ap_id = ie->value.choice.MME_UE_S1AP_ID;
  } else {
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }

  S1AP_FIND_PROTOCOLIE_BY_ID(
      S1ap_UEContextReleaseRequest_IEs_t, ie, container,
      S1ap_ProtocolIE_ID_id_eNB_UE_S1AP_ID, true);
  if (ie) {
    enb_ue_s1ap_id = (enb_ue_s1ap_id_t)(
        ie->value.choice.ENB_UE_S1AP_ID & ENB_UE_S1AP_ID_MASK);
  } else {
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }

  // Log the Cause Type and Cause value
  S1AP_FIND_PROTOCOLIE_BY_ID(
      S1ap_UEContextReleaseRequest_IEs_t, ie, container,
      S1ap_ProtocolIE_ID_id_Cause, true);
  if (ie) {
    cause_type = ie->value.choice.Cause.present;
  } else {
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }
  switch (cause_type) {
    case S1ap_Cause_PR_radioNetwork:
      cause_value = ie->value.choice.Cause.choice.radioNetwork;
      OAILOG_INFO(
          LOG_S1AP,
          "UE CONTEXT RELEASE REQUEST with Cause_Type = Radio Network and "
          "Cause_Value = %ld\n",
          cause_value);
      if (cause_value == S1ap_CauseRadioNetwork_user_inactivity) {
        increment_counter(
            "ue_context_release_req", 1, 1, "cause", "user_inactivity");
      } else if (
          cause_value == S1ap_CauseRadioNetwork_radio_connection_with_ue_lost) {
        increment_counter(
            "ue_context_release_req", 1, 1, "cause", "radio_link_failure");
      } else if (
          cause_value ==
          S1ap_CauseRadioNetwork_ue_not_available_for_ps_service) {
        increment_counter(
            "ue_context_release_req", 1, 1, "cause",
            "ue_not_available_for_ps_service");
        s1_release_cause = S1AP_NAS_UE_NOT_AVAILABLE_FOR_PS;
      } else if (cause_value == S1ap_CauseRadioNetwork_cs_fallback_triggered) {
        increment_counter(
            "ue_context_release_req", 1, 1, "cause", "cs_fallback_triggered");
        s1_release_cause = S1AP_CSFB_TRIGGERED;
      }
      break;

    case S1ap_Cause_PR_transport:
      cause_value = ie->value.choice.Cause.choice.transport;
      OAILOG_INFO(
          LOG_S1AP,
          "UE CONTEXT RELEASE REQUEST with Cause_Type = Transport and "
          "Cause_Value = %ld\n",
          cause_value);
      break;

    case S1ap_Cause_PR_nas:
      cause_value = ie->value.choice.Cause.choice.nas;
      OAILOG_INFO(
          LOG_S1AP,
          "UE CONTEXT RELEASE REQUEST with Cause_Type = NAS and Cause_Value = "
          "%ld\n",
          cause_value);
      break;

    case S1ap_Cause_PR_protocol:
      cause_value = ie->value.choice.Cause.choice.protocol;
      OAILOG_INFO(
          LOG_S1AP,
          "UE CONTEXT RELEASE REQUEST with Cause_Type = Transport and "
          "Cause_Value = %ld\n",
          cause_value);
      break;

    case S1ap_Cause_PR_misc:
      cause_value = ie->value.choice.Cause.choice.misc;
      OAILOG_INFO(
          LOG_S1AP,
          "UE CONTEXT RELEASE REQUEST with Cause_Type = MISC and Cause_Value = "
          "%ld\n",
          cause_value);
      break;

    default:
      OAILOG_ERROR(
          LOG_S1AP, "UE CONTEXT RELEASE REQUEST with Invalid Cause_Type = %d\n",
          cause_type);
      OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }

  /* Fix - MME shall handle UE Context Release received from the eNB
  irrespective of the cause. And MME should release the S1-U bearers for the UE
  and move the UE to ECM idle mode. Cause can influence whether to preserve GBR
  bearers or not.Since, as of now EPC doesn't support dedicated bearers, it is
  don't care scenario till we add support for dedicated bearers.
  */

  if ((ue_ref_p = s1ap_state_get_ue_mmeid(mme_ue_s1ap_id)) == NULL) {
    /*
     * MME doesn't know the MME UE S1AP ID provided.
     * No need to do anything. Ignore the message
     */
    OAILOG_DEBUG(
        LOG_S1AP,
        "UE_CONTEXT_RELEASE_REQUEST ignored cause could not get context with "
        "mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT
        " enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT " ",
        (uint32_t) mme_ue_s1ap_id, (uint32_t) enb_ue_s1ap_id);
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  } else {
    if (ue_ref_p->enb_ue_s1ap_id == enb_ue_s1ap_id) {
      /*
       * Both eNB UE S1AP ID and MME UE S1AP ID match.
       * Send a UE context Release Command to eNB after releasing S1-U bearer
       * tunnel mapping for all the bearers.
       */
      s1ap_imsi_map_t* imsi_map = get_s1ap_imsi_map();
      hashtable_uint64_ts_get(
          imsi_map->mme_ue_id_imsi_htbl, (const hash_key_t) mme_ue_s1ap_id,
          &imsi64);

      message_p =
          itti_alloc_new_message(TASK_S1AP, S1AP_UE_CONTEXT_RELEASE_REQ);
      AssertFatal(message_p != NULL, "itti_alloc_new_message Failed");

      // enb_ref_p = s1ap_state_get_enb(state, ue_ref_p->sctp_assoc_id);

      S1AP_UE_CONTEXT_RELEASE_REQ(message_p).mme_ue_s1ap_id =
          ue_ref_p->mme_ue_s1ap_id;
      S1AP_UE_CONTEXT_RELEASE_REQ(message_p).enb_ue_s1ap_id =
          ue_ref_p->enb_ue_s1ap_id;
      S1AP_UE_CONTEXT_RELEASE_REQ(message_p).relCause = s1_release_cause;
      S1AP_UE_CONTEXT_RELEASE_REQ(message_p).cause    = ie->value.choice.Cause;

      message_p->ittiMsgHeader.imsi = imsi64;
      rc = send_msg_to_task(&s1ap_task_zmq_ctx, TASK_MME_APP, message_p);
      OAILOG_FUNC_RETURN(LOG_S1AP, rc);
    } else {
      // abnormal case. No need to do anything. Ignore the message
      OAILOG_DEBUG_UE(
          LOG_S1AP, imsi64,
          "UE_CONTEXT_RELEASE_REQUEST ignored, cause mismatch enb_ue_s1ap_id: "
          "ctxt " ENB_UE_S1AP_ID_FMT " != request " ENB_UE_S1AP_ID_FMT " ",
          (uint32_t) ue_ref_p->enb_ue_s1ap_id, (uint32_t) enb_ue_s1ap_id);
      OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
    }
  }
  OAILOG_FUNC_RETURN(LOG_S1AP, RETURNok);
}

//------------------------------------------------------------------------------
int s1ap_mme_generate_ue_context_release_command(
    s1ap_state_t* state, ue_description_t* ue_ref_p, enum s1cause cause,
    imsi64_t imsi64) {
  uint8_t* buffer = NULL;
  uint32_t length = 0;
  S1ap_S1AP_PDU_t pdu;
  S1ap_UEContextReleaseCommand_t* out;
  S1ap_UEContextReleaseCommand_IEs_t* ie = NULL;
  int rc                                 = RETURNok;
  S1ap_Cause_PR cause_type;
  long cause_value;

  OAILOG_FUNC_IN(LOG_S1AP);
  if (ue_ref_p == NULL) {
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = S1ap_S1AP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage.procedureCode =
      S1ap_ProcedureCode_id_UEContextRelease;
  pdu.choice.initiatingMessage.criticality = S1ap_Criticality_reject;
  pdu.choice.initiatingMessage.value.present =
      S1ap_InitiatingMessage__value_PR_UEContextReleaseCommand;
  out = &pdu.choice.initiatingMessage.value.choice.UEContextReleaseCommand;
  /*
   * Fill in ID pair
   */
  ie = (S1ap_UEContextReleaseCommand_IEs_t*) calloc(
      1, sizeof(S1ap_UEContextReleaseCommand_IEs_t));
  ie->id            = S1ap_ProtocolIE_ID_id_UE_S1AP_IDs;
  ie->criticality   = S1ap_Criticality_reject;
  ie->value.present = S1ap_UEContextReleaseCommand_IEs__value_PR_UE_S1AP_IDs;
  ie->value.choice.UE_S1AP_IDs.present = S1ap_UE_S1AP_IDs_PR_uE_S1AP_ID_pair;
  ie->value.choice.UE_S1AP_IDs.choice.uE_S1AP_ID_pair.mME_UE_S1AP_ID =
      ue_ref_p->mme_ue_s1ap_id;
  ie->value.choice.UE_S1AP_IDs.choice.uE_S1AP_ID_pair.eNB_UE_S1AP_ID =
      ue_ref_p->enb_ue_s1ap_id;
  ie->value.choice.UE_S1AP_IDs.choice.uE_S1AP_ID_pair.iE_Extensions = NULL;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  ie = (S1ap_UEContextReleaseCommand_IEs_t*) calloc(
      1, sizeof(S1ap_UEContextReleaseCommand_IEs_t));
  ie->id            = S1ap_ProtocolIE_ID_id_Cause;
  ie->criticality   = S1ap_Criticality_ignore;
  ie->value.present = S1ap_UEContextReleaseCommand_IEs__value_PR_Cause;
  switch (cause) {
    case S1AP_NAS_DETACH:
      cause_type  = S1ap_Cause_PR_nas;
      cause_value = S1ap_CauseNas_detach;
      break;
    case S1AP_NAS_NORMAL_RELEASE:
      cause_type  = S1ap_Cause_PR_nas;
      cause_value = S1ap_CauseNas_unspecified;
      break;
    case S1AP_RADIO_EUTRAN_GENERATED_REASON:
      cause_type = S1ap_Cause_PR_radioNetwork;
      cause_value =
          S1ap_CauseRadioNetwork_release_due_to_eutran_generated_reason;
      break;
    case S1AP_INITIAL_CONTEXT_SETUP_FAILED:
      cause_type  = S1ap_Cause_PR_radioNetwork;
      cause_value = S1ap_CauseRadioNetwork_unspecified;
      break;
    case S1AP_CSFB_TRIGGERED:
      cause_type  = S1ap_Cause_PR_radioNetwork;
      cause_value = S1ap_CauseRadioNetwork_cs_fallback_triggered;
      break;
    case S1AP_NAS_UE_NOT_AVAILABLE_FOR_PS:
      cause_type  = S1ap_Cause_PR_radioNetwork;
      cause_value = S1ap_CauseRadioNetwork_ue_not_available_for_ps_service;
      break;
    default:
      OAILOG_ERROR_UE(LOG_S1AP, imsi64, "Unknown cause for context release");
      OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }
  s1ap_mme_set_cause(&ie->value.choice.Cause, cause_type, cause_value);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  if (s1ap_mme_encode_pdu(&pdu, &buffer, &length) < 0) {
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }

  bstring b = blk2bstr(buffer, length);
  free(buffer);
  rc = s1ap_mme_itti_send_sctp_request(
      &b, ue_ref_p->sctp_assoc_id, ue_ref_p->sctp_stream_send,
      ue_ref_p->mme_ue_s1ap_id);
  ue_ref_p->s1_ue_state = S1AP_UE_WAITING_CRR;

  // Start timer to track UE context release complete from eNB

  // We can safely remove UE context now, no need for timer
  s1ap_mme_release_ue_context(state, ue_ref_p, imsi64);

  OAILOG_FUNC_RETURN(LOG_S1AP, rc);
}

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
int s1ap_mme_generate_ue_context_modification(
    ue_description_t* ue_ref_p,
    const itti_s1ap_ue_context_mod_req_t* const ue_context_mod_req_pP,
    imsi64_t imsi64) {
  uint8_t* buffer                                = NULL;
  uint32_t length                                = 0;
  S1ap_S1AP_PDU_t pdu                            = {0};
  S1ap_UEContextModificationRequest_t* container = NULL;
  S1ap_UEContextModificationRequestIEs_t* ie     = NULL;
  int rc                                         = RETURNok;

  OAILOG_FUNC_IN(LOG_S1AP);
  if (ue_ref_p == NULL) {
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }
  container =
      &pdu.choice.initiatingMessage.value.choice.UEContextModificationRequest;
  pdu.choice.successfulOutcome.procedureCode =
      S1ap_ProcedureCode_id_UEContextModification;
  pdu.present = S1ap_S1AP_PDU_PR_initiatingMessage;
  /*
   * Fill in ID pair
   */
  ie->value.choice.MME_UE_S1AP_ID = ue_ref_p->mme_ue_s1ap_id;
  ie->value.choice.ENB_UE_S1AP_ID = ue_ref_p->enb_ue_s1ap_id;

  S1AP_FIND_PROTOCOLIE_BY_ID(
      S1ap_UEContextModificationRequestIEs_t, ie, container,
      S1ap_ProtocolIE_ID_id_RegisteredLAI, true);

  if ((ue_context_mod_req_pP->presencemask & S1AP_UE_CONTEXT_MOD_LAI_PRESENT) ==
      S1AP_UE_CONTEXT_MOD_LAI_PRESENT) {
#define PLMN_SIZE 3
    S1ap_LAI_t* lai_item        = &ie->value.choice.LAI;
    lai_item->pLMNidentity.size = PLMN_SIZE;
    lai_item->pLMNidentity.buf  = calloc(PLMN_SIZE, sizeof(uint8_t));
    uint8_t mnc_length          = mme_config_find_mnc_length(
        ue_context_mod_req_pP->lai.mccdigit1,
        ue_context_mod_req_pP->lai.mccdigit2,
        ue_context_mod_req_pP->lai.mccdigit3,
        ue_context_mod_req_pP->lai.mncdigit1,
        ue_context_mod_req_pP->lai.mncdigit2,
        ue_context_mod_req_pP->lai.mncdigit3);
    if (mnc_length != 2 && mnc_length != 3) {
      OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
    }
    LAI_T_TO_TBCD(
        ue_context_mod_req_pP->lai, lai_item->pLMNidentity.buf, mnc_length);

    TAC_TO_ASN1(ue_context_mod_req_pP->lai.lac, &lai_item->lAC);
    lai_item->iE_Extensions = NULL;
  }
  S1AP_FIND_PROTOCOLIE_BY_ID(
      S1ap_UEContextModificationRequestIEs_t, ie, container,
      S1ap_ProtocolIE_ID_id_CSFallbackIndicator, true);

  if ((ue_context_mod_req_pP->presencemask &
       S1AP_UE_CONTEXT_MOD_CSFB_INDICATOR_PRESENT) ==
      S1AP_UE_CONTEXT_MOD_CSFB_INDICATOR_PRESENT) {
    ie->value.choice.CSFallbackIndicator =
        ue_context_mod_req_pP->cs_fallback_indicator;
  }

  S1AP_FIND_PROTOCOLIE_BY_ID(
      S1ap_UEContextModificationRequestIEs_t, ie, container,
      S1ap_ProtocolIE_ID_id_uEaggregateMaximumBitrate, true);
  if ((S1AP_UE_CONTEXT_MOD_UE_AMBR_INDICATOR_PRESENT) ==
      S1AP_UE_CONTEXT_MOD_UE_AMBR_INDICATOR_PRESENT) {
    asn_uint642INTEGER(
        &ie->value.choice.UEAggregateMaximumBitrate.uEaggregateMaximumBitRateDL,
        ue_context_mod_req_pP->ue_ambr.br_dl);
    asn_uint642INTEGER(
        &ie->value.choice.UEAggregateMaximumBitrate.uEaggregateMaximumBitRateUL,
        ue_context_mod_req_pP->ue_ambr.br_ul);
  }

  if (s1ap_mme_encode_pdu(&pdu, &buffer, &length) < 0) {
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }

  bstring b = blk2bstr(buffer, length);
  free(buffer);
  rc = s1ap_mme_itti_send_sctp_request(
      &b, ue_ref_p->sctp_assoc_id, ue_ref_p->sctp_stream_send,
      ue_ref_p->mme_ue_s1ap_id);

  OAILOG_FUNC_RETURN(LOG_S1AP, rc);
}

//------------------------------------------------------------------------------
int s1ap_handle_ue_context_release_command(
    s1ap_state_t* state,
    const itti_s1ap_ue_context_release_command_t* const
        ue_context_release_command_pP,
    imsi64_t imsi64) {
  ue_description_t* ue_ref_p = NULL;
  int rc                     = RETURNok;

  OAILOG_FUNC_IN(LOG_S1AP);
  if ((ue_ref_p = s1ap_state_get_ue_mmeid(
           ue_context_release_command_pP->mme_ue_s1ap_id)) == NULL) {
    OAILOG_DEBUG_UE(
        LOG_S1AP, imsi64,
        "Ignoring UE with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " %u(10)\n",
        ue_context_release_command_pP->mme_ue_s1ap_id,
        ue_context_release_command_pP->mme_ue_s1ap_id);
    rc = RETURNok;
  } else {
    /*
     * Check the cause. If it is implicit detach or sctp reset/shutdown no need
     * to send UE context release command to eNB. Free UE context locally.
     */

    if (ue_context_release_command_pP->cause == S1AP_IMPLICIT_CONTEXT_RELEASE ||
        ue_context_release_command_pP->cause == S1AP_SCTP_SHUTDOWN_OR_RESET ||
        ue_context_release_command_pP->cause ==
            S1AP_INITIAL_CONTEXT_SETUP_TMR_EXPRD ||
        ue_context_release_command_pP->cause == S1AP_INVALID_ENB_ID) {
      s1ap_remove_ue(state, ue_ref_p);
    } else {
      rc = s1ap_mme_generate_ue_context_release_command(
          state, ue_ref_p, ue_context_release_command_pP->cause, imsi64);
    }
  }

  OAILOG_FUNC_RETURN(LOG_S1AP, rc);
}

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
int s1ap_handle_ue_context_mod_req(
    s1ap_state_t* state,
    const itti_s1ap_ue_context_mod_req_t* const ue_context_mod_req_pP,
    imsi64_t imsi64) {
  ue_description_t* ue_ref_p = NULL;
  int rc                     = RETURNok;

  OAILOG_FUNC_IN(LOG_S1AP);
  DevAssert(ue_context_mod_req_pP != NULL);
  if ((ue_ref_p = s1ap_state_get_ue_mmeid(
           ue_context_mod_req_pP->mme_ue_s1ap_id)) == NULL) {
    OAILOG_DEBUG_UE(
        LOG_S1AP, imsi64,
        "Ignoring UE with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " %u(10)\n",
        ue_context_mod_req_pP->mme_ue_s1ap_id,
        ue_context_mod_req_pP->mme_ue_s1ap_id);
    rc = RETURNok;
  } else {
    rc = s1ap_mme_generate_ue_context_modification(
        ue_ref_p, ue_context_mod_req_pP, imsi64);
  }

  OAILOG_FUNC_RETURN(LOG_S1AP, rc);
}

//------------------------------------------------------------------------------
int s1ap_mme_handle_ue_context_release_complete(
    s1ap_state_t* state, __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    S1ap_S1AP_PDU_t* pdu) {
  S1ap_UEContextReleaseComplete_t* container;
  S1ap_UEContextReleaseComplete_IEs_t* ie = NULL;
  ue_description_t* ue_ref_p              = NULL;
  mme_ue_s1ap_id_t mme_ue_s1ap_id         = 0;

  OAILOG_FUNC_IN(LOG_S1AP);
  container =
      &pdu->choice.successfulOutcome.value.choice.UEContextReleaseComplete;

  S1AP_FIND_PROTOCOLIE_BY_ID(
      S1ap_UEContextReleaseComplete_IEs_t, ie, container,
      S1ap_ProtocolIE_ID_id_MME_UE_S1AP_ID, true);

  if (ie) {
    mme_ue_s1ap_id = ie->value.choice.MME_UE_S1AP_ID;
  } else {
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNok);
  }

  if ((ue_ref_p = s1ap_state_get_ue_mmeid(mme_ue_s1ap_id)) == NULL) {
    /*
     * The UE context has already been deleted when the UE context release
     * command was sent
     * Ignore this message.
     */
    OAILOG_DEBUG(
        LOG_S1AP,
        " UE Context Release commplete: S1 context cleared. Ignore message for "
        "ueid " MME_UE_S1AP_ID_FMT "\n",
        (uint32_t) mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNok);
  } else {
    /* This is an error scenario, the S1 UE context should have been deleted
     * when UE context release command was sent
     */
    OAILOG_ERROR(
        LOG_S1AP,
        " UE Context Release commplete: S1 context should have been cleared "
        "for "
        "ueid " MME_UE_S1AP_ID_FMT "\n",
        (uint32_t) mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }
}

//------------------------------------------------------------------------------
int s1ap_mme_handle_initial_context_setup_failure(
    s1ap_state_t* state, __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    S1ap_S1AP_PDU_t* pdu) {
  S1ap_InitialContextSetupFailure_t* container;
  S1ap_InitialContextSetupFailureIEs_t* ie = NULL;
  ue_description_t* ue_ref_p               = NULL;
  MessageDef* message_p                    = NULL;
  S1ap_Cause_PR cause_type;
  long cause_value;
  int rc                          = RETURNok;
  imsi64_t imsi64                 = INVALID_IMSI64;
  mme_ue_s1ap_id_t mme_ue_s1ap_id = 0;
  enb_ue_s1ap_id_t enb_ue_s1ap_id = 0;

  OAILOG_FUNC_IN(LOG_S1AP);
  container =
      &pdu->choice.unsuccessfulOutcome.value.choice.InitialContextSetupFailure;

  S1AP_FIND_PROTOCOLIE_BY_ID(
      S1ap_InitialContextSetupFailureIEs_t, ie, container,
      S1ap_ProtocolIE_ID_id_MME_UE_S1AP_ID, true);
  if (ie) {
    mme_ue_s1ap_id = ie->value.choice.MME_UE_S1AP_ID;
  } else {
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNok);
  }

  S1AP_FIND_PROTOCOLIE_BY_ID(
      S1ap_InitialContextSetupFailureIEs_t, ie, container,
      S1ap_ProtocolIE_ID_id_eNB_UE_S1AP_ID, true);
  if (ie) {
    enb_ue_s1ap_id = (enb_ue_s1ap_id_t)(
        ie->value.choice.ENB_UE_S1AP_ID & ENB_UE_S1AP_ID_MASK);
  } else {
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNok);
  }

  if ((ue_ref_p = s1ap_state_get_ue_mmeid(mme_ue_s1ap_id)) == NULL) {
    /*
     * MME doesn't know the MME UE S1AP ID provided.
     */
    OAILOG_INFO(
        LOG_S1AP,
        "INITIAL_CONTEXT_SETUP_FAILURE ignored. No context with "
        "mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT
        " enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT " ",
        (uint32_t) mme_ue_s1ap_id, (uint32_t) enb_ue_s1ap_id);
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }

  if (ue_ref_p->enb_ue_s1ap_id != enb_ue_s1ap_id) {
    // abnormal case. No need to do anything. Ignore the message
    OAILOG_DEBUG(
        LOG_S1AP,
        "INITIAL_CONTEXT_SETUP_FAILURE ignored, mismatch enb_ue_s1ap_id: "
        "ctxt " ENB_UE_S1AP_ID_FMT " != received " ENB_UE_S1AP_ID_FMT " ",
        (uint32_t) ue_ref_p->enb_ue_s1ap_id, (uint32_t) enb_ue_s1ap_id);
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }

  s1ap_imsi_map_t* imsi_map = get_s1ap_imsi_map();
  hashtable_uint64_ts_get(
      imsi_map->mme_ue_id_imsi_htbl, (const hash_key_t) mme_ue_s1ap_id,
      &imsi64);

  // Pass this message to MME APP for necessary handling
  // Log the Cause Type and Cause value
  S1AP_FIND_PROTOCOLIE_BY_ID(
      S1ap_InitialContextSetupFailureIEs_t, ie, container,
      S1ap_ProtocolIE_ID_id_Cause, true);
  if (ie) {
    cause_type = ie->value.choice.Cause.present;
  } else {
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }

  switch (cause_type) {
    case S1ap_Cause_PR_radioNetwork:
      cause_value = ie->value.choice.Cause.choice.radioNetwork;
      OAILOG_DEBUG_UE(
          LOG_S1AP, imsi64,
          "INITIAL_CONTEXT_SETUP_FAILURE with Cause_Type = Radio Network and "
          "Cause_Value = %ld\n",
          cause_value);
      break;

    case S1ap_Cause_PR_transport:
      cause_value = ie->value.choice.Cause.choice.transport;
      OAILOG_DEBUG_UE(
          LOG_S1AP, imsi64,
          "INITIAL_CONTEXT_SETUP_FAILURE with Cause_Type = Transport and "
          "Cause_Value = %ld\n",
          cause_value);
      break;

    case S1ap_Cause_PR_nas:
      cause_value = ie->value.choice.Cause.choice.nas;
      OAILOG_DEBUG_UE(
          LOG_S1AP, imsi64,
          "INITIAL_CONTEXT_SETUP_FAILURE with Cause_Type = NAS and Cause_Value "
          "= "
          "%ld\n",
          cause_value);
      break;

    case S1ap_Cause_PR_protocol:
      cause_value = ie->value.choice.Cause.choice.protocol;
      OAILOG_DEBUG_UE(
          LOG_S1AP, imsi64,
          "INITIAL_CONTEXT_SETUP_FAILURE with Cause_Type = Protocol and "
          "Cause_Value = %ld\n",
          cause_value);
      break;

    case S1ap_Cause_PR_misc:
      cause_value = ie->value.choice.Cause.choice.misc;
      OAILOG_DEBUG_UE(
          LOG_S1AP, imsi64,
          "INITIAL_CONTEXT_SETUP_FAILURE with Cause_Type = MISC and "
          "Cause_Value "
          "= %ld\n",
          cause_value);
      break;

    default:
      OAILOG_DEBUG_UE(
          LOG_S1AP, imsi64,
          "INITIAL_CONTEXT_SETUP_FAILURE with Invalid Cause_Type = %d\n",
          cause_type);
      OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }
  message_p =
      itti_alloc_new_message(TASK_S1AP, MME_APP_INITIAL_CONTEXT_SETUP_FAILURE);
  AssertFatal(message_p != NULL, "itti_alloc_new_message Failed");
  memset(
      (void*) &message_p->ittiMsg.mme_app_initial_context_setup_failure, 0,
      sizeof(itti_mme_app_initial_context_setup_failure_t));
  MME_APP_INITIAL_CONTEXT_SETUP_FAILURE(message_p).mme_ue_s1ap_id =
      ue_ref_p->mme_ue_s1ap_id;

  message_p->ittiMsgHeader.imsi = imsi64;
  rc = send_msg_to_task(&s1ap_task_zmq_ctx, TASK_MME_APP, message_p);
  OAILOG_FUNC_RETURN(LOG_S1AP, rc);
}

int s1ap_mme_handle_ue_context_modification_response(
    s1ap_state_t* state, __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    S1ap_S1AP_PDU_t* pdu) {
  S1ap_UEContextModificationResponseIEs_t* ie     = NULL;
  S1ap_UEContextModificationResponse_t* container = NULL;
  ue_description_t* ue_ref_p                      = NULL;
  MessageDef* message_p                           = NULL;
  int rc                                          = RETURNok;
  imsi64_t imsi64                                 = INVALID_IMSI64;

  OAILOG_FUNC_IN(LOG_S1AP);
  container =
      &pdu->choice.successfulOutcome.value.choice.UEContextModificationResponse;

  S1AP_FIND_PROTOCOLIE_BY_ID(
      S1ap_UEContextModificationResponseIEs_t, ie, container,
      S1ap_ProtocolIE_ID_id_MME_UE_S1AP_ID, true);
  S1AP_FIND_PROTOCOLIE_BY_ID(
      S1ap_UEContextModificationResponseIEs_t, ie, container,
      S1ap_ProtocolIE_ID_id_eNB_UE_S1AP_ID, true);

  if ((ie) && (ue_ref_p = s1ap_state_get_ue_mmeid(
                   ie->value.choice.MME_UE_S1AP_ID)) == NULL) {
    /*
     * MME doesn't know the MME UE S1AP ID provided.
     * No need to do anything. Ignore the message
     */
    OAILOG_DEBUG(
        LOG_S1AP,
        "UE_CONTEXT_MODIFICATION_RESPONSE ignored cause could not get context "
        "with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT
        " enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT " ",
        (uint32_t) ie->value.choice.MME_UE_S1AP_ID,
        (uint32_t) ie->value.choice.ENB_UE_S1AP_ID);
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  } else {
    if ((ie) && (ue_ref_p->enb_ue_s1ap_id ==
                 (ie->value.choice.ENB_UE_S1AP_ID & ENB_UE_S1AP_ID_MASK))) {
      /*
       * Both eNB UE S1AP ID and MME UE S1AP ID match.
       * Send a UE context Release Command to eNB after releasing S1-U bearer
       * tunnel mapping for all the bearers.
       */

      s1ap_imsi_map_t* imsi_map = get_s1ap_imsi_map();
      hashtable_uint64_ts_get(
          imsi_map->mme_ue_id_imsi_htbl,
          (const hash_key_t) ie->value.choice.MME_UE_S1AP_ID, &imsi64);

      message_p = itti_alloc_new_message(
          TASK_S1AP, S1AP_UE_CONTEXT_MODIFICATION_RESPONSE);
      AssertFatal(message_p != NULL, "itti_alloc_new_message Failed");
      memset(
          (void*) &message_p->ittiMsg.s1ap_ue_context_mod_response, 0,
          sizeof(itti_s1ap_ue_context_mod_resp_t));
      S1AP_UE_CONTEXT_MODIFICATION_RESPONSE(message_p).mme_ue_s1ap_id =
          ue_ref_p->mme_ue_s1ap_id;
      S1AP_UE_CONTEXT_MODIFICATION_RESPONSE(message_p).enb_ue_s1ap_id =
          ue_ref_p->enb_ue_s1ap_id;

      message_p->ittiMsgHeader.imsi = imsi64;
      rc = send_msg_to_task(&s1ap_task_zmq_ctx, TASK_MME_APP, message_p);
      OAILOG_FUNC_RETURN(LOG_S1AP, rc);
    } else {
      // abnormal case. No need to do anything. Ignore the message
      OAILOG_DEBUG_UE(
          LOG_S1AP, imsi64,
          "S1AP_UE_CONTEXT_MODIFICATION_RESPONSE ignored, cause mismatch "
          "enb_ue_s1ap_id: ctxt" ENB_UE_S1AP_ID_FMT
          " != request " ENB_UE_S1AP_ID_FMT " ",
          (uint32_t) ue_ref_p->enb_ue_s1ap_id,
          (uint32_t) ie->value.choice.ENB_UE_S1AP_ID);
      OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
    }
  }

  OAILOG_FUNC_RETURN(LOG_S1AP, RETURNok);
}

int s1ap_mme_handle_ue_context_modification_failure(
    s1ap_state_t* state, __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    S1ap_S1AP_PDU_t* pdu) {
  S1ap_UEContextModificationFailureIEs_t* ie     = NULL;
  S1ap_UEContextModificationFailure_t* container = NULL;
  ue_description_t* ue_ref_p                     = NULL;
  MessageDef* message_p                          = NULL;
  int rc                                         = RETURNok;
  S1ap_Cause_PR cause_type                       = {0};
  int64_t cause_value;
  imsi64_t imsi64 = INVALID_IMSI64;

  OAILOG_FUNC_IN(LOG_S1AP);
  container = &pdu->choice.unsuccessfulOutcome.value.choice
                   .UEContextModificationFailure;

  S1AP_FIND_PROTOCOLIE_BY_ID(
      S1ap_UEContextModificationFailureIEs_t, ie, container,
      S1ap_ProtocolIE_ID_id_MME_UE_S1AP_ID, true);
  S1AP_FIND_PROTOCOLIE_BY_ID(
      S1ap_UEContextModificationFailureIEs_t, ie, container,
      S1ap_ProtocolIE_ID_id_eNB_UE_S1AP_ID, true);

  if ((ie) && (ue_ref_p = s1ap_state_get_ue_mmeid(
                   ie->value.choice.MME_UE_S1AP_ID)) == NULL) {
    /*
     * MME doesn't know the MME UE S1AP ID provided.
     * No need to do anything. Ignore the message
     */
    OAILOG_DEBUG(
        LOG_S1AP,
        "UE_CONTEXT_MODIFICATION_FAILURE ignored cause could not get context "
        "with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT
        " enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT " ",
        (uint32_t) ie->value.choice.MME_UE_S1AP_ID,
        (uint32_t) ie->value.choice.ENB_UE_S1AP_ID);
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  } else {
    if ((ie) && (ue_ref_p->enb_ue_s1ap_id ==
                 (ie->value.choice.ENB_UE_S1AP_ID & ENB_UE_S1AP_ID_MASK))) {
      s1ap_imsi_map_t* imsi_map = get_s1ap_imsi_map();
      hashtable_uint64_ts_get(
          imsi_map->mme_ue_id_imsi_htbl,
          (const hash_key_t) ie->value.choice.MME_UE_S1AP_ID, &imsi64);

      // Pass this message to MME APP for necessary handling
      // Log the Cause Type and Cause value
      S1AP_FIND_PROTOCOLIE_BY_ID(
          S1ap_UEContextModificationFailureIEs_t, ie, container,
          S1ap_ProtocolIE_ID_id_Cause, true);
      if (ie) {
        cause_value = ie->value.choice.Cause.choice.radioNetwork;
      } else {
        OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
      }
      switch (cause_type) {
        case S1ap_Cause_PR_radioNetwork:
          cause_value = ie->value.choice.Cause.choice.radioNetwork;
          OAILOG_DEBUG_UE(
              LOG_S1AP, imsi64,
              "UE_CONTEXT_MODIFICATION_FAILURE with Cause_Type = Radio Network "
              "and Cause_Value = %ld\n",
              cause_value);
          break;

        case S1ap_Cause_PR_transport:
          cause_value = ie->value.choice.Cause.choice.transport;
          OAILOG_DEBUG_UE(
              LOG_S1AP, imsi64,
              "UE_CONTEXT_MODIFICATION_FAILURE with Cause_Type = Transport and "
              "Cause_Value = %ld\n",
              cause_value);
          break;

        case S1ap_Cause_PR_nas:
          cause_value = ie->value.choice.Cause.choice.nas;
          OAILOG_DEBUG_UE(
              LOG_S1AP, imsi64,
              "UE_CONTEXT_MODIFICATION_FAILURE with Cause_Type = NAS and "
              "Cause_Value = %ld\n",
              cause_value);
          break;

        case S1ap_Cause_PR_protocol:
          cause_value = ie->value.choice.Cause.choice.protocol;
          OAILOG_DEBUG_UE(
              LOG_S1AP, imsi64,
              "UE_CONTEXT_MODIFICATION_FAILURE with Cause_Type = Protocol and "
              "Cause_Value = %ld\n",
              cause_value);
          break;

        case S1ap_Cause_PR_misc:
          cause_value = ie->value.choice.Cause.choice.misc;
          OAILOG_DEBUG_UE(
              LOG_S1AP, imsi64,
              "UE_CONTEXT_MODIFICATION_FAILURE with Cause_Type = MISC and "
              "Cause_Value = %ld\n",
              cause_value);
          break;

        default:
          OAILOG_ERROR_UE(
              LOG_S1AP, imsi64,
              "UE_CONTEXT_MODIFICATION_FAILURE with Invalid Cause_Type = %d\n",
              cause_type);
          OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
      }
      message_p = itti_alloc_new_message(
          TASK_S1AP, S1AP_UE_CONTEXT_MODIFICATION_FAILURE);
      AssertFatal(message_p != NULL, "itti_alloc_new_message Failed");
      memset(
          (void*) &message_p->ittiMsg.s1ap_ue_context_mod_response, 0,
          sizeof(itti_s1ap_ue_context_mod_resp_fail_t));
      S1AP_UE_CONTEXT_MODIFICATION_FAILURE(message_p).mme_ue_s1ap_id =
          ue_ref_p->mme_ue_s1ap_id;
      S1AP_UE_CONTEXT_MODIFICATION_FAILURE(message_p).enb_ue_s1ap_id =
          ue_ref_p->enb_ue_s1ap_id;
      S1AP_UE_CONTEXT_MODIFICATION_FAILURE(message_p).cause = cause_value;

      message_p->ittiMsgHeader.imsi = imsi64;
      rc = send_msg_to_task(&s1ap_task_zmq_ctx, TASK_MME_APP, message_p);
      OAILOG_FUNC_RETURN(LOG_S1AP, rc);
    } else {
      // abnormal case. No need to do anything. Ignore the message
      OAILOG_DEBUG_UE(
          LOG_S1AP, imsi64,
          "S1AP_UE_CONTEXT_MODIFICATION_FAILURE ignored, cause mismatch "
          "enb_ue_s1ap_id: ctxt " ENB_UE_S1AP_ID_FMT
          " != request " ENB_UE_S1AP_ID_FMT " ",
          (uint32_t) ue_ref_p->enb_ue_s1ap_id,
          (uint32_t) ie->value.choice.ENB_UE_S1AP_ID);
      OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
    }
  }

  OAILOG_FUNC_RETURN(LOG_S1AP, RETURNok);
}
////////////////////////////////////////////////////////////////////////////////
//************************ Handover signalling *******************************//
////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
int s1ap_mme_handle_path_switch_request(
    s1ap_state_t* state, __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    S1ap_S1AP_PDU_t* pdu) {
  S1ap_PathSwitchRequest_t* container                            = NULL;
  S1ap_PathSwitchRequestIEs_t* ie                                = NULL;
  S1ap_E_RABToBeSwitchedDLItemIEs_t* eRABToBeSwitchedDlItemIEs_p = NULL;
  enb_description_t* enb_association                             = NULL;
  ue_description_t* ue_ref_p                                     = NULL;
  ue_description_t* new_ue_ref_p                                 = NULL;
  mme_ue_s1ap_id_t mme_ue_s1ap_id                                = 0;
  enb_ue_s1ap_id_t enb_ue_s1ap_id                                = 0;
  ecgi_t ecgi = {.plmn = {0}, .cell_identity = {0}};
  tai_t tai   = {0};
  uint16_t encryption_algorithm_capabilitie                            = 0;
  uint16_t integrity_algorithm_capabilities                            = 0;
  e_rab_to_be_switched_in_downlink_list_t e_rab_to_be_switched_dl_list = {0};
  uint32_t num_erab                                                    = 0;
  uint16_t index                                                       = 0;
  itti_s1ap_path_switch_request_failure_t path_switch_req_failure      = {0};
  imsi64_t imsi64           = INVALID_IMSI64;
  s1ap_imsi_map_t* imsi_map = get_s1ap_imsi_map();

  OAILOG_FUNC_IN(LOG_S1AP);

  enb_association = s1ap_state_get_enb(state, assoc_id);
  if (enb_association == NULL) {
    OAILOG_ERROR(
        LOG_S1AP,
        "Ignore Path Switch Request from unknown assoc "
        "%u\n",
        assoc_id);
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }

  container = &pdu->choice.initiatingMessage.value.choice.PathSwitchRequest;

  S1AP_FIND_PROTOCOLIE_BY_ID(
      S1ap_PathSwitchRequestIEs_t, ie, container,
      S1ap_ProtocolIE_ID_id_eNB_UE_S1AP_ID, true);
  // eNB UE S1AP ID is limited to 24 bits
  if (ie) {
    enb_ue_s1ap_id = (enb_ue_s1ap_id_t)(
        ie->value.choice.ENB_UE_S1AP_ID & ENB_UE_S1AP_ID_MASK);
  } else {
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }

  OAILOG_DEBUG_UE(
      LOG_S1AP, imsi64,
      "Path Switch Request message received from eNB UE S1AP "
      "ID: " ENB_UE_S1AP_ID_FMT "\n",
      enb_ue_s1ap_id);

  hashtable_uint64_ts_get(
      imsi_map->mme_ue_id_imsi_htbl, (const hash_key_t) mme_ue_s1ap_id,
      &imsi64);

  /* If all the E-RAB ID IEs in E-RABToBeSwitchedDLList is set to the
   * same value, send PATH SWITCH REQUEST FAILURE message to eNB */
  if (true == is_all_erabId_same(container)) {
    /*send PATH SWITCH REQUEST FAILURE message to eNB*/
    path_switch_req_failure.sctp_assoc_id  = assoc_id;
    path_switch_req_failure.mme_ue_s1ap_id = mme_ue_s1ap_id;
    path_switch_req_failure.enb_ue_s1ap_id = enb_ue_s1ap_id;
    s1ap_handle_path_switch_req_failure(
        state, &path_switch_req_failure, imsi64);
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }

  if ((ue_ref_p = s1ap_state_get_ue_mmeid(mme_ue_s1ap_id)) == NULL) {
    /*
     * The MME UE S1AP ID provided by eNB doesn't point to any valid UE.
     * MME ignore this PATH SWITCH REQUEST.
     */
    OAILOG_ERROR_UE(
        LOG_S1AP, imsi64,
        "source MME_UE_S1AP_ID (" MME_UE_S1AP_ID_FMT
        ") does not point to any valid UE\n",
        mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  } else {
    new_ue_ref_p =
        s1ap_state_get_ue_enbid(enb_association->sctp_assoc_id, enb_ue_s1ap_id);
    if (new_ue_ref_p != NULL) {
      OAILOG_ERROR_UE(
          LOG_S1AP, imsi64,
          "S1AP:Path Switch Request- Recieved ENB_UE_S1AP_ID is not Unique "
          "Drop Path Switch Request for eNBUeS1APId:" ENB_UE_S1AP_ID_FMT "\n",
          enb_ue_s1ap_id);
      OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
    }
    /*
     * Creat New UE Context with target eNB and delete Old UE Context
     * from source eNB.
     */
    if ((new_ue_ref_p = s1ap_new_ue(state, assoc_id, enb_ue_s1ap_id)) == NULL) {
      // If we failed to allocate a new UE return -1
      OAILOG_ERROR_UE(
          LOG_S1AP, imsi64,
          "S1AP:Path Switch Request- Failed to allocate S1AP UE Context, "
          "eNBUeS1APId:" ENB_UE_S1AP_ID_FMT "\n",
          enb_ue_s1ap_id);
      OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
    }
    new_ue_ref_p->s1_ue_state    = ue_ref_p->s1_ue_state;
    new_ue_ref_p->enb_ue_s1ap_id = enb_ue_s1ap_id;
    // Will be allocated by NAS
    new_ue_ref_p->mme_ue_s1ap_id = mme_ue_s1ap_id;

    new_ue_ref_p->s1ap_ue_context_rel_timer.id =
        ue_ref_p->s1ap_ue_context_rel_timer.id;
    new_ue_ref_p->s1ap_ue_context_rel_timer.sec =
        ue_ref_p->s1ap_ue_context_rel_timer.sec;
    // On which stream we received the message
    new_ue_ref_p->sctp_stream_recv = stream;
    new_ue_ref_p->sctp_stream_send = enb_association->next_sctp_stream;
    enb_association->next_sctp_stream += 1;
    if (enb_association->next_sctp_stream >= enb_association->instreams) {
      enb_association->next_sctp_stream = 1;
    }
    /* Remove ue description from source eNB */
    s1ap_remove_ue(state, ue_ref_p);

    /* Mapping between mme_ue_s1ap_id, assoc_id and enb_ue_s1ap_id */
    hashtable_rc_t h_rc = hashtable_ts_insert(
        &state->mmeid2associd, (const hash_key_t) new_ue_ref_p->mme_ue_s1ap_id,
        (void*) (uintptr_t) assoc_id);

    hashtable_uint64_ts_insert(
        &enb_association->ue_id_coll,
        (const hash_key_t) new_ue_ref_p->mme_ue_s1ap_id,
        new_ue_ref_p->comp_s1ap_id);

    OAILOG_DEBUG_UE(
        LOG_S1AP, imsi64,
        "Associated sctp_assoc_id %d, enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT
        ", mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT ":%s \n",
        assoc_id, new_ue_ref_p->enb_ue_s1ap_id, new_ue_ref_p->mme_ue_s1ap_id,
        hashtable_rc_code2string(h_rc));

    s1ap_dump_enb(enb_association);

    S1AP_FIND_PROTOCOLIE_BY_ID(
        S1ap_PathSwitchRequestIEs_t, ie, container,
        S1ap_ProtocolIE_ID_id_E_RABToBeSwitchedDLList, true);
    DevAssert(ie);
    S1ap_E_RABToBeSwitchedDLList_t* e_rab_to_be_switched_dl_list_req =
        &ie->value.choice.E_RABToBeSwitchedDLList;

    // E-RAB To Be Switched in Downlink List mandatory IE
    num_erab = e_rab_to_be_switched_dl_list_req->list.count;
    for (index = 0; index < num_erab; ++index) {
      eRABToBeSwitchedDlItemIEs_p =
          (S1ap_E_RABToBeSwitchedDLItemIEs_t*)
              e_rab_to_be_switched_dl_list_req->list.array[index];
      S1ap_E_RABToBeSwitchedDLItem_t* eRab_ToBeSwitchedDLItem =
          &eRABToBeSwitchedDlItemIEs_p->value.choice.E_RABToBeSwitchedDLItem;

      e_rab_to_be_switched_dl_list.item[index].e_rab_id =
          eRab_ToBeSwitchedDLItem->e_RAB_ID;
      e_rab_to_be_switched_dl_list.item[index].transport_layer_address =
          blk2bstr(
              eRab_ToBeSwitchedDLItem->transportLayerAddress.buf,
              eRab_ToBeSwitchedDLItem->transportLayerAddress.size);
      e_rab_to_be_switched_dl_list.item[index].gtp_teid =
          htonl(*((uint32_t*) eRab_ToBeSwitchedDLItem->gTP_TEID.buf));
      e_rab_to_be_switched_dl_list.no_of_items += 1;
    }

    // CGI mandatory IE
    S1AP_FIND_PROTOCOLIE_BY_ID(
        S1ap_PathSwitchRequestIEs_t, ie, container,
        S1ap_ProtocolIE_ID_id_EUTRAN_CGI, true);
    DevAssert(ie);
    DevAssert(ie->value.choice.EUTRAN_CGI.pLMNidentity.size == 3);
    TBCD_TO_PLMN_T(&ie->value.choice.EUTRAN_CGI.pLMNidentity, &ecgi.plmn);
    BIT_STRING_TO_CELL_IDENTITY(
        &ie->value.choice.EUTRAN_CGI.cell_ID, ecgi.cell_identity);

    // TAI mandatory IE
    S1AP_FIND_PROTOCOLIE_BY_ID(
        S1ap_PathSwitchRequestIEs_t, ie, container, S1ap_ProtocolIE_ID_id_TAI,
        true);
    DevAssert(ie);
    OCTET_STRING_TO_TAC(&ie->value.choice.TAI.tAC, tai.tac);
    DevAssert(ie->value.choice.TAI.pLMNidentity.size == 3);
    TBCD_TO_PLMN_T(&ie->value.choice.TAI.pLMNidentity, &tai.plmn);

    // UE Security Capabilities mandatory IE
    BIT_STRING_TO_INT16(
        &ie->value.choice.UESecurityCapabilities.encryptionAlgorithms,
        encryption_algorithm_capabilitie);
    BIT_STRING_TO_INT16(
        &ie->value.choice.UESecurityCapabilities.integrityProtectionAlgorithms,
        integrity_algorithm_capabilities);
  }

  s1ap_mme_itti_s1ap_path_switch_request(
      assoc_id, enb_association->enb_id, new_ue_ref_p->enb_ue_s1ap_id,
      &e_rab_to_be_switched_dl_list, new_ue_ref_p->mme_ue_s1ap_id, &ecgi, &tai,
      encryption_algorithm_capabilitie, integrity_algorithm_capabilities,
      imsi64);

  OAILOG_FUNC_RETURN(LOG_S1AP, RETURNok);
}

//------------------------------------------------------------------------------
typedef struct arg_s1ap_send_enb_dereg_ind_s {
  uint8_t current_ue_index;
  uint32_t handled_ues;
  MessageDef* message_p;
  uint32_t associated_enb_id;
  uint32_t deregister_ue_count;
} arg_s1ap_send_enb_dereg_ind_t;

//------------------------------------------------------------------------------
bool s1ap_send_enb_deregistered_ind(
    __attribute__((unused)) const hash_key_t keyP, uint64_t const dataP,
    void* argP, void** resultP) {
  arg_s1ap_send_enb_dereg_ind_t* arg = (arg_s1ap_send_enb_dereg_ind_t*) argP;
  ue_description_t* ue_ref_p         = (ue_description_t*) dataP;
  imsi64_t imsi64                    = INVALID_IMSI64;

  // Ask for the release of each UE context associated to the eNB
  hash_table_ts_t* s1ap_ue_state = get_s1ap_ue_state();
  hashtable_ts_get(s1ap_ue_state, (const hash_key_t) dataP, (void**) &ue_ref_p);
  if (ue_ref_p) {
    if (arg->current_ue_index == 0) {
      arg->message_p =
          itti_alloc_new_message(TASK_S1AP, S1AP_ENB_DEREGISTERED_IND);
    }
    if (ue_ref_p->mme_ue_s1ap_id == INVALID_MME_UE_S1AP_ID) {
      /*
       * Send deregistered ind for this also and let MMEAPP find the context
       * using enb_ue_s1ap_id_key
       */
      OAILOG_WARNING(LOG_S1AP, "UE with invalid MME s1ap id found");
    }
    s1ap_imsi_map_t* imsi_map = get_s1ap_imsi_map();
    hashtable_uint64_ts_get(
        imsi_map->mme_ue_id_imsi_htbl,
        (const hash_key_t) ue_ref_p->mme_ue_s1ap_id, &imsi64);

    AssertFatal(
        arg->current_ue_index < S1AP_ITTI_UE_PER_DEREGISTER_MESSAGE,
        "Too many deregistered UEs reported in S1AP_ENB_DEREGISTERED_IND "
        "message ");
    S1AP_ENB_DEREGISTERED_IND(arg->message_p)
        .mme_ue_s1ap_id[arg->current_ue_index] = ue_ref_p->mme_ue_s1ap_id;
    S1AP_ENB_DEREGISTERED_IND(arg->message_p)
        .enb_ue_s1ap_id[arg->current_ue_index] = ue_ref_p->enb_ue_s1ap_id;

    arg->handled_ues++;
    arg->current_ue_index++;

    if (arg->handled_ues == arg->deregister_ue_count ||
        arg->current_ue_index == S1AP_ITTI_UE_PER_DEREGISTER_MESSAGE) {
      // Sending INVALID_IMSI64 because message is not specific to any UE/IMSI
      arg->message_p->ittiMsgHeader.imsi               = INVALID_IMSI64;
      S1AP_ENB_DEREGISTERED_IND(arg->message_p).enb_id = arg->associated_enb_id;
      S1AP_ENB_DEREGISTERED_IND(arg->message_p).nb_ue_to_deregister =
          (uint8_t) arg->current_ue_index;

      // Max UEs reached for this ITTI message, send message to MME App
      OAILOG_DEBUG(
          LOG_S1AP,
          "Reached maximum UE count for this ITTI message. Sending "
          "deregistered indication to MME App for UE count = %u\n",
          S1AP_ENB_DEREGISTERED_IND(arg->message_p).nb_ue_to_deregister);

      if (arg->current_ue_index == S1AP_ITTI_UE_PER_DEREGISTER_MESSAGE) {
        arg->current_ue_index = 0;
      }
      send_msg_to_task(&s1ap_task_zmq_ctx, TASK_MME_APP, arg->message_p);
      arg->message_p = NULL;
    }

    *resultP = arg->message_p;
  } else {
    OAILOG_TRACE(LOG_S1AP, "No valid UE provided in callback: %p\n", ue_ref_p);
  }
  return false;
}

typedef struct arg_s1ap_construct_enb_reset_req_s {
  uint8_t current_ue_index;
  MessageDef* msg;
} arg_s1ap_construct_enb_reset_req_t;

bool construct_s1ap_mme_full_reset_req(
    const hash_key_t keyP, const uint64_t dataP, void* argP, void** resultP) {
  arg_s1ap_construct_enb_reset_req_t* arg = argP;
  ue_description_t* ue_ref                = (ue_description_t*) dataP;
  ;
  uint32_t i = arg->current_ue_index;

  hash_table_ts_t* s1ap_ue_state = get_s1ap_ue_state();
  hashtable_ts_get(s1ap_ue_state, (const hash_key_t) dataP, (void**) &ue_ref);
  if (ue_ref) {
    S1AP_ENB_INITIATED_RESET_REQ(arg->msg).ue_to_reset_list[i].mme_ue_s1ap_id =
        ue_ref->mme_ue_s1ap_id;
    S1AP_ENB_INITIATED_RESET_REQ(arg->msg).ue_to_reset_list[i].enb_ue_s1ap_id =
        ue_ref->enb_ue_s1ap_id;
  } else {
    OAILOG_TRACE(LOG_S1AP, "No valid UE provided in callback: %p\n", ue_ref);
    S1AP_ENB_INITIATED_RESET_REQ(arg->msg).ue_to_reset_list[i].mme_ue_s1ap_id =
        INVALID_MME_UE_S1AP_ID;
  }
  arg->current_ue_index++;

  return false;
}

//------------------------------------------------------------------------------
int s1ap_handle_sctp_disconnection(
    s1ap_state_t* state, const sctp_assoc_id_t assoc_id, bool reset) {
  arg_s1ap_send_enb_dereg_ind_t arg  = {0};
  int i                              = 0;
  MessageDef* message_p              = NULL;
  enb_description_t* enb_association = NULL;

  OAILOG_FUNC_IN(LOG_S1AP);

  // Checking if the assoc id has a valid eNB attached to it
  enb_association = s1ap_state_get_enb(state, assoc_id);
  if (enb_association == NULL) {
    OAILOG_ERROR(LOG_S1AP, "No eNB attached to this assoc_id: %d\n", assoc_id);
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }

  OAILOG_INFO(
      LOG_S1AP,
      "SCTP disconnection request for association id %u, Reset Flag = "
      "%u. Connected UEs = %u \n",
      assoc_id, reset, enb_association->nb_ue_associated);

  // First check if we can just reset the eNB state if there are no UEs
  if (!enb_association->nb_ue_associated) {
    if (reset) {
      OAILOG_INFO(
          LOG_S1AP,
          "SCTP reset request for association id %u. No Connected UEs. "
          "Reset Flag = %u\n",
          assoc_id, reset);

      OAILOG_INFO(
          LOG_S1AP, "Moving eNB with assoc_id %u to INIT state\n", assoc_id);
      enb_association->s1_state = S1AP_INIT;
      update_mme_app_stats_connected_enb_sub();
    } else {
      OAILOG_INFO(
          LOG_S1AP,
          "SCTP Shutdown request for association id %u. No Connected UEs. "
          "Reset Flag = %u\n",
          assoc_id, reset);

      OAILOG_INFO(LOG_S1AP, "Removing eNB with association id %u \n", assoc_id);
      s1ap_remove_enb(state, enb_association);
      update_mme_app_stats_connected_enb_sub();
    }
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNok);
  }

  /*
   * Send S1ap deregister indication to MME app in batches of UEs where
   * UE count in each batch <= S1AP_ITTI_UE_PER_DEREGISTER_MESSAGE
   */
  arg.associated_enb_id   = enb_association->enb_id;
  arg.deregister_ue_count = enb_association->ue_id_coll.num_elements;
  hashtable_uint64_ts_apply_callback_on_elements(
      &enb_association->ue_id_coll, s1ap_send_enb_deregistered_ind,
      (void*) &arg, (void**) &message_p);

  for (i = arg.current_ue_index; i < S1AP_ITTI_UE_PER_DEREGISTER_MESSAGE; i++) {
    S1AP_ENB_DEREGISTERED_IND(message_p).mme_ue_s1ap_id[arg.current_ue_index] =
        0;
    S1AP_ENB_DEREGISTERED_IND(message_p).enb_ue_s1ap_id[arg.current_ue_index] =
        0;
  }
  S1AP_ENB_DEREGISTERED_IND(message_p).enb_id = enb_association->enb_id;
  message_p                                   = NULL;

  OAILOG_FUNC_RETURN(LOG_S1AP, RETURNok);
}

//------------------------------------------------------------------------------
int s1ap_handle_new_association(
    s1ap_state_t* state, sctp_new_peer_t* sctp_new_peer_p) {
  enb_description_t* enb_association = NULL;

  OAILOG_FUNC_IN(LOG_S1AP);
  DevAssert(sctp_new_peer_p != NULL);

  /*
   * Checking that the assoc id has a valid eNB attached to.
   */
  enb_association = s1ap_state_get_enb(state, sctp_new_peer_p->assoc_id);
  if (enb_association == NULL) {
    OAILOG_DEBUG(
        LOG_S1AP, "Create eNB context for assoc_id: %d\n",
        sctp_new_peer_p->assoc_id);
    /*
     * Create new context
     */
    enb_association = s1ap_new_enb(state);

    if (enb_association == NULL) {
      /*
       * We failed to allocate memory
       */
      /*
       * TODO: send reject there
       */
      OAILOG_ERROR(
          LOG_S1AP, "Failed to allocate eNB context for assoc_id: %d\n",
          sctp_new_peer_p->assoc_id);
      OAILOG_FUNC_RETURN(LOG_S1AP, RETURNok);
    }
    enb_association->sctp_assoc_id = sctp_new_peer_p->assoc_id;
    hashtable_rc_t hash_rc         = hashtable_ts_insert(
        &state->enbs, (const hash_key_t) enb_association->sctp_assoc_id,
        (void*) enb_association);
    if (HASH_TABLE_OK != hash_rc) {
      OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
    }
  } else if (
      (enb_association->s1_state == S1AP_SHUTDOWN) ||
      (enb_association->s1_state == S1AP_RESETING)) {
    OAILOG_WARNING(
        LOG_S1AP,
        "Received new association request on an association that is being %s, "
        "ignoring",
        s1_enb_state2str(enb_association->s1_state));
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  } else {
    OAILOG_DEBUG(
        LOG_S1AP, "eNB context already exists for assoc_id: %d, update it\n",
        sctp_new_peer_p->assoc_id);
  }

  enb_association->sctp_assoc_id = sctp_new_peer_p->assoc_id;
  /*
   * Fill in in and out number of streams available on SCTP connection.
   */
  enb_association->instreams  = (sctp_stream_id_t) sctp_new_peer_p->instreams;
  enb_association->outstreams = (sctp_stream_id_t) sctp_new_peer_p->outstreams;
  /*
   * initialize the next sctp stream to 1 as 0 is reserved for non
   * * * * ue associated signalling.
   */
  enb_association->next_sctp_stream = 1;
  enb_association->s1_state         = S1AP_INIT;
  OAILOG_FUNC_RETURN(LOG_S1AP, RETURNok);
}

//------------------------------------------------------------------------------
void s1ap_mme_handle_ue_context_rel_comp_timer_expiry(
    s1ap_state_t* state, ue_description_t* ue_ref_p) {
  MessageDef* message_p = NULL;
  OAILOG_FUNC_IN(LOG_S1AP);
  DevAssert(ue_ref_p != NULL);
  ue_ref_p->s1ap_ue_context_rel_timer.id = S1AP_TIMER_INACTIVE_ID;
  imsi64_t imsi64                        = INVALID_IMSI64;

  s1ap_imsi_map_t* imsi_map = get_s1ap_imsi_map();
  hashtable_uint64_ts_get(
      imsi_map->mme_ue_id_imsi_htbl,
      (const hash_key_t) ue_ref_p->mme_ue_s1ap_id, &imsi64);

  OAILOG_DEBUG_UE(
      LOG_S1AP, imsi64, "Expired- UE Context Release Timer for UE id  %d \n",
      ue_ref_p->mme_ue_s1ap_id);
  /*
   * Remove UE context and inform MME_APP.
   */
  message_p =
      itti_alloc_new_message(TASK_S1AP, S1AP_UE_CONTEXT_RELEASE_COMPLETE);
  AssertFatal(message_p != NULL, "itti_alloc_new_message Failed");
  memset(
      (void*) &message_p->ittiMsg.s1ap_ue_context_release_complete, 0,
      sizeof(itti_s1ap_ue_context_release_complete_t));
  S1AP_UE_CONTEXT_RELEASE_COMPLETE(message_p).mme_ue_s1ap_id =
      ue_ref_p->mme_ue_s1ap_id;

  message_p->ittiMsgHeader.imsi = imsi64;
  send_msg_to_task(&s1ap_task_zmq_ctx, TASK_MME_APP, message_p);
  DevAssert(ue_ref_p->s1_ue_state == S1AP_UE_WAITING_CRR);

  OAILOG_DEBUG_UE(
      LOG_S1AP, imsi64, "Removed S1AP UE " MME_UE_S1AP_ID_FMT "\n",
      (uint32_t) ue_ref_p->mme_ue_s1ap_id);
  s1ap_remove_ue(state, ue_ref_p);

  hashtable_uint64_ts_remove(
      imsi_map->mme_ue_id_imsi_htbl,
      (const hash_key_t) ue_ref_p->mme_ue_s1ap_id);

  OAILOG_FUNC_OUT(LOG_S1AP);
}

//------------------------------------------------------------------------------
void s1ap_mme_release_ue_context(
    s1ap_state_t* state, ue_description_t* ue_ref_p, imsi64_t imsi64) {
  MessageDef* message_p = NULL;
  OAILOG_FUNC_IN(LOG_S1AP);
  DevAssert(ue_ref_p != NULL);
  OAILOG_DEBUG_UE(
      LOG_S1AP, imsi64, "Releasing UE Context for UE id  %d \n",
      ue_ref_p->mme_ue_s1ap_id);
  /*
   * Remove UE context and inform MME_APP.
   */
  message_p =
      itti_alloc_new_message(TASK_S1AP, S1AP_UE_CONTEXT_RELEASE_COMPLETE);
  AssertFatal(message_p != NULL, "itti_alloc_new_message Failed");
  memset(
      (void*) &message_p->ittiMsg.s1ap_ue_context_release_complete, 0,
      sizeof(itti_s1ap_ue_context_release_complete_t));
  S1AP_UE_CONTEXT_RELEASE_COMPLETE(message_p).mme_ue_s1ap_id =
      ue_ref_p->mme_ue_s1ap_id;

  message_p->ittiMsgHeader.imsi = imsi64;
  send_msg_to_task(&s1ap_task_zmq_ctx, TASK_MME_APP, message_p);
  DevAssert(ue_ref_p->s1_ue_state == S1AP_UE_WAITING_CRR);
  OAILOG_DEBUG_UE(
      LOG_S1AP, imsi64, "Removed S1AP UE " MME_UE_S1AP_ID_FMT "\n",
      (uint32_t) ue_ref_p->mme_ue_s1ap_id);

  s1ap_remove_ue(state, ue_ref_p);
  OAILOG_FUNC_OUT(LOG_S1AP);
}

//------------------------------------------------------------------------------
int s1ap_mme_handle_error_ind_message(
    s1ap_state_t* state, const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream, S1ap_S1AP_PDU_t* message) {
  OAILOG_FUNC_IN(LOG_S1AP);
  OAILOG_WARNING(
      LOG_S1AP, "ERROR IND RCVD on Stream id %d, ignoring it\n", stream);
  increment_counter("s1ap_error_ind_rcvd", 1, NO_LABELS);
  OAILOG_FUNC_RETURN(LOG_S1AP, RETURNok);
}

//------------------------------------------------------------------------------
int s1ap_mme_handle_erab_setup_response(
    s1ap_state_t* state, const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream, S1ap_S1AP_PDU_t* pdu) {
  OAILOG_FUNC_IN(LOG_S1AP);
  S1ap_E_RABSetupResponse_t* container = NULL;
  S1ap_E_RABSetupResponseIEs_t* ie     = NULL;
  ue_description_t* ue_ref_p           = NULL;
  MessageDef* message_p                = NULL;
  enb_ue_s1ap_id_t enb_ue_s1ap_id      = 0;
  mme_ue_s1ap_id_t mme_ue_s1ap_id      = 0;
  int rc                               = RETURNok;
  imsi64_t imsi64                      = INVALID_IMSI64;

  container = &pdu->choice.successfulOutcome.value.choice.E_RABSetupResponse;

  S1AP_FIND_PROTOCOLIE_BY_ID(
      S1ap_E_RABSetupResponseIEs_t, ie, container,
      S1ap_ProtocolIE_ID_id_MME_UE_S1AP_ID, true);
  if (ie) {
    mme_ue_s1ap_id = ie->value.choice.MME_UE_S1AP_ID;
  } else {
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }
  S1AP_FIND_PROTOCOLIE_BY_ID(
      S1ap_E_RABSetupResponseIEs_t, ie, container,
      S1ap_ProtocolIE_ID_id_eNB_UE_S1AP_ID, true);
  if (ie) {
    // eNB UE S1AP ID is limited to 24 bits
    enb_ue_s1ap_id = (enb_ue_s1ap_id_t)(
        ie->value.choice.ENB_UE_S1AP_ID & ENB_UE_S1AP_ID_MASK);
  } else {
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }
  if ((ue_ref_p = s1ap_state_get_ue_mmeid((uint32_t) mme_ue_s1ap_id)) == NULL) {
    OAILOG_DEBUG(
        LOG_S1AP,
        "No UE is attached to this mme UE s1ap id: " MME_UE_S1AP_ID_FMT "\n",
        mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }

  if (ue_ref_p->enb_ue_s1ap_id != enb_ue_s1ap_id) {
    OAILOG_DEBUG(
        LOG_S1AP,
        "Mismatch in eNB UE S1AP ID, known: " ENB_UE_S1AP_ID_FMT
        ", received: " ENB_UE_S1AP_ID_FMT "\n",
        ue_ref_p->enb_ue_s1ap_id, enb_ue_s1ap_id);
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }

  s1ap_imsi_map_t* imsi_map = get_s1ap_imsi_map();
  hashtable_uint64_ts_get(
      imsi_map->mme_ue_id_imsi_htbl,
      (const hash_key_t) ue_ref_p->mme_ue_s1ap_id, &imsi64);

  message_p = itti_alloc_new_message(TASK_S1AP, S1AP_E_RAB_SETUP_RSP);
  AssertFatal(message_p != NULL, "itti_alloc_new_message Failed");
  S1AP_E_RAB_SETUP_RSP(message_p).mme_ue_s1ap_id = ue_ref_p->mme_ue_s1ap_id;
  S1AP_E_RAB_SETUP_RSP(message_p).enb_ue_s1ap_id = ue_ref_p->enb_ue_s1ap_id;
  S1AP_E_RAB_SETUP_RSP(message_p).e_rab_setup_list.no_of_items           = 0;
  S1AP_E_RAB_SETUP_RSP(message_p).e_rab_failed_to_setup_list.no_of_items = 0;

  S1AP_FIND_PROTOCOLIE_BY_ID(
      S1ap_E_RABSetupResponseIEs_t, ie, container,
      S1ap_ProtocolIE_ID_id_E_RABSetupListBearerSURes, false);

  if (ie) {
    int num_erab = ie->value.choice.E_RABSetupListBearerSURes.list.count;
    for (int index = 0; index < num_erab; index++) {
      S1ap_E_RABSetupItemBearerSUResIEs_t* erab_setup_item =
          (S1ap_E_RABSetupItemBearerSUResIEs_t*)
              ie->value.choice.E_RABSetupListBearerSURes.list.array[index];
      S1ap_E_RABSetupItemBearerSURes_t* e_rab_setup_item_bearer_su_res =
          &erab_setup_item->value.choice.E_RABSetupItemBearerSURes;
      S1AP_E_RAB_SETUP_RSP(message_p).e_rab_setup_list.item[index].e_rab_id =
          e_rab_setup_item_bearer_su_res->e_RAB_ID;
      S1AP_E_RAB_SETUP_RSP(message_p)
          .e_rab_setup_list.item[index]
          .transport_layer_address = blk2bstr(
          e_rab_setup_item_bearer_su_res->transportLayerAddress.buf,
          e_rab_setup_item_bearer_su_res->transportLayerAddress.size);
      S1AP_E_RAB_SETUP_RSP(message_p).e_rab_setup_list.item[index].gtp_teid =
          htonl(*((uint32_t*) e_rab_setup_item_bearer_su_res->gTP_TEID.buf));
      S1AP_E_RAB_SETUP_RSP(message_p).e_rab_setup_list.no_of_items += 1;
    }
  }

  S1AP_FIND_PROTOCOLIE_BY_ID(
      S1ap_E_RABSetupResponseIEs_t, ie, container,
      S1ap_ProtocolIE_ID_id_E_RABFailedToSetupListBearerSURes, false);
  if (ie) {
    int num_erab = ie->value.choice.E_RABList.list.count;
    for (int index = 0; index < num_erab; index++) {
      S1ap_E_RABItem_t* erab_item =
          (S1ap_E_RABItem_t*) ie->value.choice.E_RABList.list.array[index];
      S1AP_E_RAB_SETUP_RSP(message_p)
          .e_rab_failed_to_setup_list.item[index]
          .e_rab_id = erab_item->e_RAB_ID;
      S1AP_E_RAB_SETUP_RSP(message_p)
          .e_rab_failed_to_setup_list.item[index]
          .cause = erab_item->cause;
      S1AP_E_RAB_SETUP_RSP(message_p).e_rab_failed_to_setup_list.no_of_items +=
          1;
    }
  }

  message_p->ittiMsgHeader.imsi = imsi64;
  rc = send_msg_to_task(&s1ap_task_zmq_ctx, TASK_MME_APP, message_p);
  OAILOG_FUNC_RETURN(LOG_S1AP, rc);
}

//------------------------------------------------------------------------------
int s1ap_mme_handle_erab_setup_failure(
    s1ap_state_t* state, const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream, S1ap_S1AP_PDU_t* message) {
  AssertFatal(0, "TODO");
}

//------------------------------------------------------------------------------
int s1ap_mme_handle_enb_reset(
    s1ap_state_t* state, const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream, S1ap_S1AP_PDU_t* pdu) {
  MessageDef* msg                                = NULL;
  itti_s1ap_enb_initiated_reset_req_t* reset_req = NULL;
  ue_description_t* ue_ref_p                     = NULL;
  enb_description_t* enb_association             = NULL;
  s1ap_reset_type_t s1ap_reset_type;
  S1ap_Reset_t* container                                        = NULL;
  S1ap_ResetIEs_t* ie                                            = NULL;
  S1ap_UE_associatedLogicalS1_ConnectionItem_t* s1_sig_conn_id_p = NULL;
  mme_ue_s1ap_id_t mme_ue_s1ap_id;
  enb_ue_s1ap_id_t enb_ue_s1ap_id;
  imsi64_t imsi64                        = INVALID_IMSI64;
  arg_s1ap_construct_enb_reset_req_t arg = {0};
  uint32_t i                             = 0;
  int rc                                 = RETURNok;

  OAILOG_FUNC_IN(LOG_S1AP);

  enb_association = s1ap_state_get_enb(state, assoc_id);

  if (enb_association == NULL) {
    OAILOG_ERROR(LOG_S1AP, "No eNB attached to this assoc_id: %d\n", assoc_id);
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }

  if (enb_association->s1_state != S1AP_READY) {
    // ignore the message if s1 not ready
    OAILOG_INFO(
        LOG_S1AP,
        "S1 setup is not done.Invalid state.Ignoring ENB Initiated Reset.eNB "
        "Id "
        "= %d , S1AP state = %d \n",
        enb_association->enb_id, enb_association->s1_state);
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNok);
  }

  if (enb_association->nb_ue_associated == 0) {
    // ignore the message if there are no UEs connected
    OAILOG_INFO(
        LOG_S1AP,
        "No UEs is connected.Ignoring ENB Initiated Reset.eNB Id = %d\n",
        enb_association->enb_id);
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNok);
  }

  // Check the reset type - partial_reset OR reset_all
  container = &pdu->choice.initiatingMessage.value.choice.Reset;

  S1AP_FIND_PROTOCOLIE_BY_ID(
      S1ap_ResetIEs_t, ie, container, S1ap_ProtocolIE_ID_id_ResetType, true);
  S1ap_ResetType_t* resetType = &ie->value.choice.ResetType;

  switch (resetType->present) {
    case S1ap_ResetType_PR_s1_Interface:
      s1ap_reset_type = RESET_ALL;
      break;
    case S1ap_ResetType_PR_partOfS1_Interface:
      s1ap_reset_type = RESET_PARTIAL;
      break;
    default:
      OAILOG_ERROR(
          LOG_S1AP, "Reset Request from eNB  with Invalid reset_type = %d\n",
          resetType->present);
      // TBD - Here MME should send Error Indication as it is abnormal scenario.
      OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }
  if (s1ap_reset_type == RESET_PARTIAL) {
    int reset_count = resetType->choice.partOfS1_Interface.list.count;
    if (reset_count == 0) {
      OAILOG_ERROR(
          LOG_S1AP,
          "Partial Reset Request without any S1 signaling connection. Ignoring "
          "it \n");
      // TBD - Here MME should send Error Indication as it is abnormal scenario.
      OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
    }
    if (reset_count > enb_association->nb_ue_associated) {
      OAILOG_ERROR(
          LOG_S1AP,
          "Partial Reset Request. Requested number of UEs %d to be reset is "
          "more "
          "than connected UEs %d \n",
          reset_count, enb_association->nb_ue_associated);
      // TBD - Here MME should send Error Indication as it is abnormal scenario.
      OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
    }
  }
  msg       = itti_alloc_new_message(TASK_S1AP, S1AP_ENB_INITIATED_RESET_REQ);
  reset_req = &S1AP_ENB_INITIATED_RESET_REQ(msg);

  reset_req->s1ap_reset_type = s1ap_reset_type;
  reset_req->enb_id          = enb_association->enb_id;
  reset_req->sctp_assoc_id   = assoc_id;
  reset_req->sctp_stream_id  = stream;

  switch (s1ap_reset_type) {
    case RESET_ALL:
      increment_counter("s1_reset_from_enb", 1, 1, "type", "reset_all");

      reset_req->num_ue = enb_association->nb_ue_associated;

      reset_req->ue_to_reset_list = calloc(
          enb_association->nb_ue_associated,
          sizeof(*reset_req->ue_to_reset_list));

      DevAssert(reset_req->ue_to_reset_list != NULL);

      arg.msg              = msg;
      arg.current_ue_index = 0;
      hashtable_uint64_ts_apply_callback_on_elements(
          &enb_association->ue_id_coll, construct_s1ap_mme_full_reset_req, &arg,
          NULL);
      // EURECOM LG 2020-07-16 added break here
      break;
    case RESET_PARTIAL:
      // Partial Reset
      increment_counter("s1_reset_from_enb", 1, 1, "type", "reset_partial");
      reset_req->num_ue = resetType->choice.partOfS1_Interface.list.count;
      reset_req->ue_to_reset_list = calloc(
          resetType->choice.partOfS1_Interface.list.count,
          sizeof(*(reset_req->ue_to_reset_list)));
      DevAssert(reset_req->ue_to_reset_list != NULL);
      for (i = 0; i < resetType->choice.partOfS1_Interface.list.count; i++) {
        s1_sig_conn_id_p =
            (S1ap_UE_associatedLogicalS1_ConnectionItem_t*)
                resetType->choice.partOfS1_Interface.list.array[i];
        DevAssert(s1_sig_conn_id_p != NULL);

        if (s1_sig_conn_id_p->mME_UE_S1AP_ID != NULL) {
          mme_ue_s1ap_id =
              (mme_ue_s1ap_id_t) * (s1_sig_conn_id_p->mME_UE_S1AP_ID);
          s1ap_imsi_map_t* imsi_map = get_s1ap_imsi_map();
          hashtable_uint64_ts_get(
              imsi_map->mme_ue_id_imsi_htbl, (const hash_key_t) mme_ue_s1ap_id,
              &imsi64);
          if ((ue_ref_p = s1ap_state_get_ue_mmeid(mme_ue_s1ap_id)) != NULL) {
            if (s1_sig_conn_id_p->eNB_UE_S1AP_ID != NULL) {
              enb_ue_s1ap_id =
                  (enb_ue_s1ap_id_t) * (s1_sig_conn_id_p->eNB_UE_S1AP_ID);
              free_wrapper((void**) &s1_sig_conn_id_p->eNB_UE_S1AP_ID);
              if (ue_ref_p->enb_ue_s1ap_id ==
                  (enb_ue_s1ap_id & ENB_UE_S1AP_ID_MASK)) {
                reset_req->ue_to_reset_list[i].mme_ue_s1ap_id =
                    ue_ref_p->mme_ue_s1ap_id;
                enb_ue_s1ap_id &= ENB_UE_S1AP_ID_MASK;
                reset_req->ue_to_reset_list[i].enb_ue_s1ap_id = enb_ue_s1ap_id;
              } else {
                // mismatch in enb_ue_s1ap_id sent by eNB and stored in S1AP ue
                // context in EPC. Abnormal case.
                reset_req->ue_to_reset_list[i].mme_ue_s1ap_id =
                    INVALID_MME_UE_S1AP_ID;
                reset_req->ue_to_reset_list[i].enb_ue_s1ap_id = -1;
                OAILOG_ERROR_UE(
                    LOG_S1AP, imsi64,
                    "Partial Reset Request:enb_ue_s1ap_id mismatch between id "
                    "%d "
                    "sent by eNB and id %d stored in epc for mme_ue_s1ap_id %d "
                    "\n",
                    enb_ue_s1ap_id, ue_ref_p->enb_ue_s1ap_id, mme_ue_s1ap_id);
              }
            } else {
              reset_req->ue_to_reset_list[i].mme_ue_s1ap_id =
                  ue_ref_p->mme_ue_s1ap_id;
              reset_req->ue_to_reset_list[i].enb_ue_s1ap_id = -1;
            }
          } else {
            OAILOG_ERROR_UE(
                LOG_S1AP, imsi64,
                "Partial Reset Request - No UE context found for "
                "mme_ue_s1ap_id "
                "%d "
                "\n",
                mme_ue_s1ap_id);
            // TBD - Here MME should send Error Indication as it is abnormal
            // scenario.
            OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
          }
        } else {
          if (s1_sig_conn_id_p->eNB_UE_S1AP_ID != NULL) {
            enb_ue_s1ap_id =
                (enb_ue_s1ap_id_t) * (s1_sig_conn_id_p->eNB_UE_S1AP_ID);
            if ((ue_ref_p = s1ap_state_get_ue_enbid(
                     enb_association->sctp_assoc_id, enb_ue_s1ap_id)) != NULL) {
              enb_ue_s1ap_id &= ENB_UE_S1AP_ID_MASK;
              reset_req->ue_to_reset_list[i].enb_ue_s1ap_id = enb_ue_s1ap_id;
              reset_req->ue_to_reset_list[i].mme_ue_s1ap_id =
                  ue_ref_p->mme_ue_s1ap_id;
            } else {
              OAILOG_ERROR_UE(
                  LOG_S1AP, imsi64,
                  "Partial Reset Request without any valid S1 signaling "
                  "connection.Ignoring it \n");
              // TBD - Here MME should send Error Indication as it is abnormal
              // scenario.
              OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
            }
          } else {
            OAILOG_ERROR_UE(
                LOG_S1AP, imsi64,
                "Partial Reset Request without any valid S1 signaling "
                "connection.Ignoring it \n");
            // TBD - Here MME should send Error Indication as it is abnormal
            // scenario.
            OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
          }
        }
      }
  }

  msg->ittiMsgHeader.imsi = imsi64;
  rc = send_msg_to_task(&s1ap_task_zmq_ctx, TASK_MME_APP, msg);
  OAILOG_FUNC_RETURN(LOG_S1AP, rc);
}
//------------------------------------------------------------------------------
int s1ap_handle_enb_initiated_reset_ack(
    const itti_s1ap_enb_initiated_reset_ack_t* const enb_reset_ack_p,
    imsi64_t imsi64) {
  uint8_t* buffer = NULL;
  uint32_t length = 0;
  S1ap_S1AP_PDU_t pdu;
  /** Reset Acknowledgment. */
  S1ap_ResetAcknowledge_t* out;
  S1ap_ResetAcknowledgeIEs_t* ie = NULL;
  int rc                         = RETURNok;

  OAILOG_FUNC_IN(LOG_S1AP);

  memset(&pdu, 0, sizeof(pdu));
  pdu.present = S1ap_S1AP_PDU_PR_successfulOutcome;
  pdu.choice.successfulOutcome.procedureCode = S1ap_ProcedureCode_id_Reset;
  pdu.choice.successfulOutcome.criticality   = S1ap_Criticality_ignore;
  pdu.choice.successfulOutcome.value.present =
      S1ap_SuccessfulOutcome__value_PR_ResetAcknowledge;
  out = &pdu.choice.successfulOutcome.value.choice.ResetAcknowledge;

  if (enb_reset_ack_p->s1ap_reset_type == RESET_PARTIAL) {
    DevAssert(enb_reset_ack_p->num_ue > 0);
    ie = (S1ap_ResetAcknowledgeIEs_t*) calloc(
        1, sizeof(S1ap_ResetAcknowledgeIEs_t));
    ie->id = S1ap_ProtocolIE_ID_id_UE_associatedLogicalS1_ConnectionListResAck;
    ie->criticality = S1ap_Criticality_ignore;
    ie->value.present =
        S1ap_ResetAcknowledgeIEs__value_PR_UE_associatedLogicalS1_ConnectionListResAck;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    /** MME UE S1AP ID. */
    S1ap_UE_associatedLogicalS1_ConnectionListResAck_t* ie_p =
        &ie->value.choice.UE_associatedLogicalS1_ConnectionListResAck;
    for (uint32_t i = 0; i < enb_reset_ack_p->num_ue; i++) {
      S1ap_UE_associatedLogicalS1_ConnectionItemResAck_t* sig_conn_item =
          calloc(1, sizeof(S1ap_UE_associatedLogicalS1_ConnectionItemResAck_t));
      sig_conn_item->id =
          S1ap_ProtocolIE_ID_id_UE_associatedLogicalS1_ConnectionItem;
      sig_conn_item->criticality = S1ap_Criticality_ignore;
      sig_conn_item->value.present =
          S1ap_UE_associatedLogicalS1_ConnectionItemResAck__value_PR_UE_associatedLogicalS1_ConnectionItem;
      S1ap_UE_associatedLogicalS1_ConnectionItem_t* item =
          &sig_conn_item->value.choice.UE_associatedLogicalS1_ConnectionItem;
      if (enb_reset_ack_p->ue_to_reset_list[i].mme_ue_s1ap_id !=
          INVALID_MME_UE_S1AP_ID) {
        item->mME_UE_S1AP_ID = calloc(1, sizeof(S1ap_MME_UE_S1AP_ID_t));
        *item->mME_UE_S1AP_ID =
            enb_reset_ack_p->ue_to_reset_list[i].mme_ue_s1ap_id;
      } else {
        item->mME_UE_S1AP_ID = NULL;
      }
      if (enb_reset_ack_p->ue_to_reset_list[i].enb_ue_s1ap_id != -1) {
        item->eNB_UE_S1AP_ID = calloc(1, sizeof(S1ap_ENB_UE_S1AP_ID_t));
        *item->eNB_UE_S1AP_ID =
            enb_reset_ack_p->ue_to_reset_list[i].enb_ue_s1ap_id;
      } else {
        item->eNB_UE_S1AP_ID = NULL;
      }
      ASN_SEQUENCE_ADD(&ie_p->list, sig_conn_item);
    }
  }
  if (s1ap_mme_encode_pdu(&pdu, &buffer, &length) < 0) {
    OAILOG_ERROR(LOG_S1AP, "Failed to S1 Reset command \n");
    /** We rely on the handover_notify timeout to remove the UE context. */
    DevAssert(!buffer);
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }

  if (buffer) {
    bstring b = blk2bstr(buffer, length);
    free(buffer);
    rc = s1ap_mme_itti_send_sctp_request(
        &b, enb_reset_ack_p->sctp_assoc_id, enb_reset_ack_p->sctp_stream_id,
        INVALID_MME_UE_S1AP_ID);
  }
  OAILOG_FUNC_RETURN(LOG_S1AP, rc);
}

//-------------------------------------------------------------------------------
int s1ap_handle_paging_request(
    s1ap_state_t* state, const itti_s1ap_paging_request_t* paging_request,
    imsi64_t imsi64) {
  OAILOG_FUNC_IN(LOG_S1AP);
  DevAssert(paging_request != NULL);
  int rc                  = RETURNok;
  uint8_t num_of_tac      = 0;
  uint16_t tai_list_count = paging_request->tai_list_count;

  bool is_tai_found    = false;
  uint32_t idx         = 0;
  uint8_t* buffer_p    = NULL;
  uint32_t length      = 0;
  S1ap_S1AP_PDU_t pdu  = {0};
  S1ap_Paging_t* out   = NULL;
  S1ap_PagingIEs_t* ie = NULL;

  memset(&pdu, 0, sizeof(pdu));
  pdu.present = S1ap_S1AP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage.procedureCode = S1ap_ProcedureCode_id_Paging;
  pdu.choice.initiatingMessage.criticality   = S1ap_Criticality_ignore;
  pdu.choice.initiatingMessage.value.present =
      S1ap_InitiatingMessage__value_PR_Paging;
  out = &pdu.choice.initiatingMessage.value.choice.Paging;

  // Encode and set the UE Identity Index Value.
  ie                = (S1ap_PagingIEs_t*) calloc(1, sizeof(S1ap_PagingIEs_t));
  ie->id            = S1ap_ProtocolIE_ID_id_UEIdentityIndexValue;
  ie->criticality   = S1ap_Criticality_ignore;
  ie->value.present = S1ap_PagingIEs__value_PR_UEIdentityIndexValue;
  UE_ID_INDEX_TO_BIT_STRING(
      (uint16_t)(imsi64 % 1024), &ie->value.choice.UEIdentityIndexValue);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  // Set UE Paging Identity
  ie                = (S1ap_PagingIEs_t*) calloc(1, sizeof(S1ap_PagingIEs_t));
  ie->id            = S1ap_ProtocolIE_ID_id_UEPagingID;
  ie->criticality   = S1ap_Criticality_ignore;
  ie->value.present = S1ap_PagingIEs__value_PR_UEPagingID;
  if (paging_request->paging_id == S1AP_PAGING_ID_STMSI) {
    ie->value.choice.UEPagingID.present = S1ap_UEPagingID_PR_s_TMSI;
    M_TMSI_TO_OCTET_STRING(
        paging_request->m_tmsi,
        &ie->value.choice.UEPagingID.choice.s_TMSI.m_TMSI);
    // todo: chose the right gummei or get it from the request!
    MME_CODE_TO_OCTET_STRING(
        paging_request->mme_code,
        &ie->value.choice.UEPagingID.choice.s_TMSI.mMEC);
  } else if (paging_request->paging_id == S1AP_PAGING_ID_IMSI) {
    ie->value.choice.UEPagingID.present = S1ap_UEPagingID_PR_iMSI;
    IMSI_TO_OCTET_STRING(
        paging_request->imsi, paging_request->imsi_length,
        &ie->value.choice.UEPagingID.choice.iMSI);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  // Encode the CN Domain.
  ie                = (S1ap_PagingIEs_t*) calloc(1, sizeof(S1ap_PagingIEs_t));
  ie->id            = S1ap_ProtocolIE_ID_id_CNDomain;
  ie->criticality   = S1ap_Criticality_ignore;
  ie->value.present = S1ap_PagingIEs__value_PR_CNDomain;
  if (paging_request->domain_indicator == CN_DOMAIN_PS) {
    ie->value.choice.CNDomain = S1ap_CNDomain_ps;
  } else if (paging_request->domain_indicator == CN_DOMAIN_CS) {
    ie->value.choice.CNDomain = S1ap_CNDomain_cs;
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  // Set TAI list
  ie                = (S1ap_PagingIEs_t*) calloc(1, sizeof(S1ap_PagingIEs_t));
  ie->id            = S1ap_ProtocolIE_ID_id_TAIList;
  ie->criticality   = S1ap_Criticality_ignore;
  ie->value.present = S1ap_PagingIEs__value_PR_TAIList;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  S1ap_TAIList_t* const tai_list = &ie->value.choice.TAIList;

  mme_config_read_lock(&mme_config);
  for (int tai_idx = 0; tai_idx < tai_list_count; tai_idx++) {
    num_of_tac = paging_request->paging_tai_list[tai_idx].numoftac;
    // Total number of TACs = number of tac + current ENB's tac(1)
    for (int idx = 0; idx < (num_of_tac + 1); idx++) {
      S1ap_TAIItemIEs_t* tai_item_ies = calloc(1, sizeof(S1ap_TAIItemIEs_t));
      if (tai_item_ies == NULL) {
        OAILOG_ERROR_UE(LOG_S1AP, imsi64, "Failed to allocate memory\n");
        OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
      }

      tai_item_ies->id            = S1ap_ProtocolIE_ID_id_TAIItem;
      tai_item_ies->criticality   = S1ap_Criticality_ignore;
      tai_item_ies->value.present = S1ap_TAIItemIEs__value_PR_TAIItem;
      S1ap_TAIItem_t* tai_item    = &tai_item_ies->value.choice.TAIItem;

      PLMN_T_TO_PLMNID(
          paging_request->paging_tai_list[tai_idx].tai_list[idx].plmn,
          &tai_item->tAI.pLMNidentity);
      TAC_TO_ASN1(
          paging_request->paging_tai_list[tai_idx].tai_list[idx].tac,
          &tai_item->tAI.tAC);
      ASN_SEQUENCE_ADD(&tai_list->list, tai_item);
    }
  }

  mme_config_unlock(&mme_config);

  // Encoding without allocating?
  int err = 0;
  if (s1ap_mme_encode_pdu(&pdu, &buffer_p, &length) < 0) {
    err = 1;
  }
  // TODO look why called proc s1ap_mme_encode_pdu do not return value < 0
  if (length <= 0) {
    err = 1;
  }
  if (err) {
    OAILOG_ERROR_UE(
        LOG_S1AP, imsi64, "Failed to encode paging message for IMSI %s\n",
        paging_request->imsi);
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }

  /*Fetching eNB list to send paging request message*/
  hashtable_element_array_t* enb_array = NULL;
  enb_description_t* enb_ref_p         = NULL;
  if (state == NULL) {
    OAILOG_ERROR(LOG_S1AP, "eNB Information is NULL!\n");
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }
  enb_array = hashtable_ts_get_elements(&state->enbs);
  if (enb_array == NULL) {
    OAILOG_ERROR(LOG_S1AP, "Could not find eNB hashlist!\n");
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }
  const paging_tai_list_t* p_tai_list = paging_request->paging_tai_list;
  for (idx = 0; idx < enb_array->num_elements; idx++) {
    enb_ref_p = (enb_description_t*) enb_array->elements[idx];
    if (enb_ref_p->s1_state == S1AP_READY) {
      supported_ta_list_t* enb_ta_list = &enb_ref_p->supported_ta_list;

      if ((is_tai_found = s1ap_paging_compare_ta_lists(
               enb_ta_list, p_tai_list, paging_request->tai_list_count))) {
        bstring paging_msg_buffer = blk2bstr(buffer_p, length);
        rc                        = s1ap_mme_itti_send_sctp_request(
            &paging_msg_buffer, enb_ref_p->sctp_assoc_id,
            0,   // Stream id 0 for non UE related
                 // S1AP message
            0);  // mme_ue_s1ap_id 0 because UE in idle
      }
    }
  }
  free(buffer_p);
  if (rc != RETURNok) {
    OAILOG_ERROR(
        LOG_S1AP, "Failed to send paging message over sctp for IMSI %s\n",
        paging_request->imsi);
  } else {
    OAILOG_INFO(
        LOG_S1AP, "Sent paging message over sctp for IMSI %s\n",
        paging_request->imsi);
  }

  OAILOG_FUNC_RETURN(LOG_S1AP, rc);
}

//----------------------------------------------------------------
int s1ap_mme_handle_enb_configuration_transfer(
    s1ap_state_t* state, const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream, S1ap_S1AP_PDU_t* pdu) {
  S1ap_ENBConfigurationTransfer_t* container = NULL;
  S1ap_ENBConfigurationTransferIEs_t* ie     = NULL;
  S1ap_TargeteNB_ID_t* targeteNB_ID          = NULL;
  uint8_t* enb_id_buf                        = NULL;
  enb_description_t* enb_association         = NULL;
  enb_description_t* target_enb_association  = NULL;
  hashtable_element_array_t* enb_array       = NULL;
  uint32_t target_enb_id                     = 0;
  uint8_t* buffer                            = NULL;
  uint32_t length                            = 0;
  uint32_t idx                               = 0;
  int rc                                     = RETURNok;

  // Not done according to Rel-15 (Target TAI and Source TAI)
  OAILOG_FUNC_IN(LOG_S1AP);
  container =
      &pdu->choice.initiatingMessage.value.choice.ENBConfigurationTransfer;

  S1AP_FIND_PROTOCOLIE_BY_ID(
      S1ap_ENBConfigurationTransferIEs_t, ie, container,
      S1ap_ProtocolIE_ID_id_SONConfigurationTransferECT, false);

  OAILOG_DEBUG(
      LOG_S1AP, "Recieved eNB Confiuration Request from assoc_id %u\n",
      assoc_id);
  enb_association = s1ap_state_get_enb(state, assoc_id);
  if (enb_association == NULL) {
    OAILOG_ERROR(
        LOG_S1AP, "Ignoring eNB Confiuration Request from unknown assoc %u\n",
        assoc_id);
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }

  if (enb_association->s1_state != S1AP_READY) {
    // ignore the message if s1 not ready
    OAILOG_INFO(
        LOG_S1AP,
        "S1 setup is not done.Invalid state.Ignoring eNB Configuration Request "
        "eNB Id = %d , S1AP state = %d \n",
        enb_association->enb_id, enb_association->s1_state);
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNok);
  }

  targeteNB_ID = &ie->value.choice.SONConfigurationTransfer.targeteNB_ID;
  if (targeteNB_ID->global_ENB_ID.eNB_ID.present == S1ap_ENB_ID_PR_homeENB_ID) {
    // Home eNB ID = 28 bits
    enb_id_buf = targeteNB_ID->global_ENB_ID.eNB_ID.choice.homeENB_ID.buf;

    target_enb_id = (enb_id_buf[0] << 20) + (enb_id_buf[1] << 12) +
                    (enb_id_buf[2] << 4) + ((enb_id_buf[3] & 0xf0) >> 4);
    OAILOG_INFO(LOG_S1AP, "home eNB id: %u\n", target_enb_id);
  } else {
    // Macro eNB = 20 bits
    enb_id_buf = targeteNB_ID->global_ENB_ID.eNB_ID.choice.macroENB_ID.buf;

    target_enb_id = (enb_id_buf[0] << 12) + (enb_id_buf[1] << 4) +
                    ((enb_id_buf[2] & 0xf0) >> 4);
    OAILOG_INFO(LOG_S1AP, "macro eNB id: %u\n", target_enb_id);
  }

  // retrieve enb_description using hash table and match target_enb_id
  if ((enb_array = hashtable_ts_get_elements(&state->enbs)) != NULL) {
    for (idx = 0; idx < enb_array->num_elements; idx++) {
      target_enb_association =
          (enb_description_t*) (uintptr_t) enb_array->elements[idx];
      if (target_enb_association->enb_id == target_enb_id) {
        break;
      }
    }
    free_wrapper((void**) &enb_array->elements);
    free_wrapper((void**) &enb_array);
    if (target_enb_association->enb_id != target_enb_id) {
      OAILOG_ERROR(LOG_S1AP, "No eNB for enb_id %d\n", target_enb_id);
      OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
    }
  }

  pdu->choice.initiatingMessage.procedureCode =
      S1ap_ProcedureCode_id_MMEConfigurationTransfer;
  pdu->present = S1ap_S1AP_PDU_PR_initiatingMessage;
  // Encode message
  int enc_rval = s1ap_mme_encode_pdu(pdu, &buffer, &length);
  if (enc_rval < 0) {
    OAILOG_ERROR(
        LOG_S1AP,
        "Failed to encode MME Configuration Transfer message for enb_id %u\n",
        target_enb_id);
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }

  bstring b = blk2bstr(buffer, length);
  free(buffer);

  // Send message
  rc = s1ap_mme_itti_send_sctp_request(
      &b, target_enb_association->sctp_assoc_id,
      0,   // Stream id 0 for non UE related S1AP message
      0);  // mme_ue_s1ap_id 0 because UE in idle

  if (rc != RETURNok) {
    OAILOG_ERROR(
        LOG_S1AP,
        "Failed to send MME Configuration Transfer message over sctp for"
        "enb_id %u\n",
        target_enb_id);
  } else {
    OAILOG_INFO(
        LOG_S1AP,
        "Sent MME Configuration Transfer message over sctp for "
        "target_enb_id %u\n",
        target_enb_id);
  }
  OAILOG_FUNC_RETURN(LOG_S1AP, rc);
}

//------------------------------------------------------------------------------
bool is_all_erabId_same(S1ap_PathSwitchRequest_t* container) {
  S1ap_PathSwitchRequestIEs_t* ie                                = NULL;
  S1ap_E_RABToBeSwitchedDLItemIEs_t* eRABToBeSwitchedDlItemIEs_p = NULL;
  uint8_t item                                                   = 0;
  uint8_t firstItem                                              = 0;
  uint8_t rc                                                     = true;

  S1AP_FIND_PROTOCOLIE_BY_ID(
      S1ap_PathSwitchRequestIEs_t, ie, container,
      S1ap_ProtocolIE_ID_id_E_RABToBeSwitchedDLList, true);
  DevAssert(ie);
  S1ap_E_RABToBeSwitchedDLList_t* e_rab_to_be_switched_dl_list =
      &ie->value.choice.E_RABToBeSwitchedDLList;

  if (1 == e_rab_to_be_switched_dl_list->list.count) {
    rc = false;
    OAILOG_FUNC_RETURN(LOG_S1AP, rc);
  }

  eRABToBeSwitchedDlItemIEs_p = (S1ap_E_RABToBeSwitchedDLItemIEs_t*)
                                    e_rab_to_be_switched_dl_list->list.array[0];
  firstItem = eRABToBeSwitchedDlItemIEs_p->value.choice.E_RABToBeSwitchedDLItem
                  .e_RAB_ID;

  for (item = 1; item < e_rab_to_be_switched_dl_list->list.count; ++item) {
    eRABToBeSwitchedDlItemIEs_p =
        (S1ap_E_RABToBeSwitchedDLItemIEs_t*)
            e_rab_to_be_switched_dl_list->list.array[item];
    if (firstItem == eRABToBeSwitchedDlItemIEs_p->value.choice
                         .E_RABToBeSwitchedDLItem.e_RAB_ID) {
      continue;
    } else {
      rc = false;
      break;
    }
  }
  OAILOG_FUNC_RETURN(LOG_S1AP, rc);
}
//------------------------------------------------------------------------------
int s1ap_handle_path_switch_req_ack(
    s1ap_state_t* state,
    const itti_s1ap_path_switch_request_ack_t* path_switch_req_ack_p,
    imsi64_t imsi64) {
  OAILOG_FUNC_IN(LOG_S1AP);

  uint8_t* buffer                            = NULL;
  uint32_t length                            = 0;
  ue_description_t* ue_ref_p                 = NULL;
  S1ap_S1AP_PDU_t pdu                        = {0};
  S1ap_PathSwitchRequestAcknowledge_t* out   = NULL;
  S1ap_PathSwitchRequestAcknowledgeIEs_t* ie = NULL;
  int rc                                     = RETURNok;

  memset(&pdu, 0, sizeof(pdu));
  pdu.choice.initiatingMessage.procedureCode =
      S1ap_ProcedureCode_id_PathSwitchRequest;
  pdu.choice.successfulOutcome.criticality = S1ap_Criticality_ignore;
  pdu.choice.successfulOutcome.value.present =
      S1ap_SuccessfulOutcome__value_PR_PathSwitchRequestAcknowledge;
  out = &pdu.choice.successfulOutcome.value.choice.PathSwitchRequestAcknowledge;

  /* mandatory */
  ie = (S1ap_PathSwitchRequestAcknowledgeIEs_t*) calloc(
      1, sizeof(S1ap_PathSwitchRequestAcknowledgeIEs_t));
  ie->id          = S1ap_ProtocolIE_ID_id_eNB_UE_S1AP_ID;
  ie->criticality = S1ap_Criticality_reject;
  ie->value.present =
      S1ap_PathSwitchRequestAcknowledgeIEs__value_PR_ENB_UE_S1AP_ID;
  ie->value.choice.ENB_UE_S1AP_ID = ue_ref_p->enb_ue_s1ap_id;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /** Add the security context. */
  ie = (S1ap_PathSwitchRequestAcknowledgeIEs_t*) calloc(
      1, sizeof(S1ap_PathSwitchRequestAcknowledgeIEs_t));
  ie->id          = S1ap_ProtocolIE_ID_id_SecurityContext;
  ie->criticality = S1ap_Criticality_reject;
  ie->value.present =
      S1ap_PathSwitchRequestAcknowledgeIEs__value_PR_SecurityContext;
  if (path_switch_req_ack_p->nh) {
    ie->value.choice.SecurityContext.nextHopParameter.buf =
        calloc(AUTH_NEXT_HOP_SIZE, sizeof(uint8_t));
    memcpy(
        ie->value.choice.SecurityContext.nextHopParameter.buf,
        path_switch_req_ack_p->nh, AUTH_NEXT_HOP_SIZE);
    ie->value.choice.SecurityContext.nextHopParameter.size = AUTH_NEXT_HOP_SIZE;
  } else {
    OAILOG_WARNING(LOG_S1AP, "No nh for PSReqAck.\n");
    ie->value.choice.SecurityContext.nextHopParameter.buf  = NULL;
    ie->value.choice.SecurityContext.nextHopParameter.size = 0;
  }
  ie->value.choice.SecurityContext.nextHopParameter.bits_unused = 0;
  ie->value.choice.SecurityContext.nextHopChainingCount =
      path_switch_req_ack_p->ncc;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  if (s1ap_mme_encode_pdu(&pdu, &buffer, &length) < 0) {
    OAILOG_ERROR_UE(
        LOG_S1AP, imsi64, "Path Switch Request Ack encoding failed \n");
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }
  bstring b = blk2bstr(buffer, length);
  OAILOG_DEBUG_UE(
      LOG_S1AP, imsi64,
      "Send PATH_SWITCH_REQUEST_ACK, mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT "\n",
      (uint32_t) path_switch_req_ack_p->mme_ue_s1ap_id);

  rc = s1ap_mme_itti_send_sctp_request(
      &b, path_switch_req_ack_p->sctp_assoc_id, ue_ref_p->sctp_stream_send,
      path_switch_req_ack_p->mme_ue_s1ap_id);

  OAILOG_FUNC_RETURN(LOG_S1AP, rc);
}
//------------------------------------------------------------------------------
int s1ap_handle_path_switch_req_failure(
    s1ap_state_t* state,
    const itti_s1ap_path_switch_request_failure_t* path_switch_req_failure_p,
    imsi64_t imsi64) {
  S1ap_PathSwitchRequestFailure_t* container = NULL;
  uint8_t* buffer                            = NULL;
  uint32_t length                            = 0;
  ue_description_t* ue_ref_p                 = NULL;
  S1ap_S1AP_PDU_t* pdu                       = {0};
  S1ap_PathSwitchRequestFailureIEs_t* ie     = NULL;
  int rc                                     = RETURNok;
  mme_ue_s1ap_id_t mme_ue_s1ap_id            = 0;
  OAILOG_FUNC_IN(LOG_S1AP);
  container =
      &pdu->choice.unsuccessfulOutcome.value.choice.PathSwitchRequestFailure;

  S1AP_FIND_PROTOCOLIE_BY_ID(
      S1ap_PathSwitchRequestFailureIEs_t, ie, container,
      S1ap_ProtocolIE_ID_id_MME_UE_S1AP_ID, true);
  mme_ue_s1ap_id = ie->value.choice.MME_UE_S1AP_ID;
  if ((ie) &&
      (ue_ref_p = s1ap_state_get_ue_mmeid((uint32_t) mme_ue_s1ap_id)) == NULL) {
    OAILOG_DEBUG_UE(
        LOG_S1AP, imsi64,
        "could not get ue context for mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT "\n",
        (uint32_t) path_switch_req_failure_p->mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);

  } else {
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNok);
  }
  pdu->choice.unsuccessfulOutcome.procedureCode =
      S1ap_ProcedureCode_id_PathSwitchRequest;
  pdu->present                    = S1ap_S1AP_PDU_PR_unsuccessfulOutcome;
  ie->value.choice.MME_UE_S1AP_ID = path_switch_req_failure_p->mme_ue_s1ap_id;
  ie->value.choice.ENB_UE_S1AP_ID = path_switch_req_failure_p->enb_ue_s1ap_id;
  s1ap_mme_set_cause(
      &ie->value.choice.Cause, S1ap_Cause_PR_radioNetwork,
      S1ap_CauseRadioNetwork_ho_failure_in_target_EPC_eNB_or_target_system);

  if (s1ap_mme_encode_pdu(pdu, &buffer, &length) < 0) {
    OAILOG_ERROR(LOG_S1AP, "Path Switch Request Failure encoding failed \n");
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }
  bstring b = blk2bstr(buffer, length);
  OAILOG_DEBUG_UE(
      LOG_S1AP, imsi64,
      "send PATH_SWITCH_REQUEST_Failure for mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT
      "\n",
      (uint32_t) path_switch_req_failure_p->mme_ue_s1ap_id);

  rc = s1ap_mme_itti_send_sctp_request(
      &b, path_switch_req_failure_p->sctp_assoc_id, ue_ref_p->sctp_stream_send,
      path_switch_req_failure_p->mme_ue_s1ap_id);

  OAILOG_FUNC_RETURN(LOG_S1AP, rc);
}

const char* s1_enb_state2str(enum mme_s1_enb_state_s state) {
  switch (state) {
    case S1AP_INIT:
      return "S1AP_INIT";
    case S1AP_RESETING:
      return "S1AP_RESETING";
    case S1AP_READY:
      return "S1AP_READY";
    case S1AP_SHUTDOWN:
      return "S1AP_SHUTDOWN";
    default:
      return "unknown s1ap_enb_state";
  }
}

const char* s1ap_direction2str(uint8_t dir) {
  switch (dir) {
    case S1ap_S1AP_PDU_PR_NOTHING:
      return "<nothing>";
    case S1ap_S1AP_PDU_PR_initiatingMessage:
      return "originating message";
    case S1ap_S1AP_PDU_PR_successfulOutcome:
      return "successful outcome";
    case S1ap_S1AP_PDU_PR_unsuccessfulOutcome:
      return "unsuccessful outcome";
    default:
      return "unknown direction";
  }
}

//------------------------------------------------------------------------------
int s1ap_mme_handle_erab_release_response(
    s1ap_state_t* state, const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream, S1ap_S1AP_PDU_t* pdu) {
  OAILOG_FUNC_IN(LOG_S1AP);
  S1ap_E_RABReleaseResponseIEs_t* ie     = NULL;
  S1ap_E_RABReleaseResponse_t* container = NULL;
  ue_description_t* ue_ref_p             = NULL;
  MessageDef* message_p                  = NULL;
  int rc                                 = RETURNok;
  imsi64_t imsi64                        = INVALID_IMSI64;
  enb_ue_s1ap_id_t enb_ue_s1ap_id        = 0;
  mme_ue_s1ap_id_t mme_ue_s1ap_id        = 0;

  container = &pdu->choice.successfulOutcome.value.choice.E_RABReleaseResponse;

  S1AP_FIND_PROTOCOLIE_BY_ID(
      S1ap_E_RABReleaseResponseIEs_t, ie, container,
      S1ap_ProtocolIE_ID_id_MME_UE_S1AP_ID, true);
  mme_ue_s1ap_id = ie->value.choice.MME_UE_S1AP_ID;

  if ((ie) &&
      (ue_ref_p = s1ap_state_get_ue_mmeid((uint32_t) mme_ue_s1ap_id)) == NULL) {
    OAILOG_ERROR(
        LOG_S1AP,
        "No UE is attached to this mme UE s1ap id: " MME_UE_S1AP_ID_FMT "\n",
        (mme_ue_s1ap_id_t) mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }

  S1AP_FIND_PROTOCOLIE_BY_ID(
      S1ap_E_RABReleaseResponseIEs_t, ie, container,
      S1ap_ProtocolIE_ID_id_eNB_UE_S1AP_ID, true);
  // eNB UE S1AP ID is limited to 24 bits
  enb_ue_s1ap_id =
      (enb_ue_s1ap_id_t)(ie->value.choice.ENB_UE_S1AP_ID & ENB_UE_S1AP_ID_MASK);

  if ((ie) && ue_ref_p->enb_ue_s1ap_id != enb_ue_s1ap_id) {
    OAILOG_ERROR(
        LOG_S1AP,
        "Mismatch in eNB UE S1AP ID, known: " ENB_UE_S1AP_ID_FMT
        ", received: " ENB_UE_S1AP_ID_FMT "\n",
        ue_ref_p->enb_ue_s1ap_id, (enb_ue_s1ap_id_t) enb_ue_s1ap_id);
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }

  s1ap_imsi_map_t* imsi_map = get_s1ap_imsi_map();
  hashtable_uint64_ts_get(
      imsi_map->mme_ue_id_imsi_htbl,
      (const hash_key_t) ie->value.choice.MME_UE_S1AP_ID, &imsi64);

  message_p = itti_alloc_new_message(TASK_S1AP, S1AP_E_RAB_REL_RSP);
  if (message_p == NULL) {
    OAILOG_ERROR(LOG_S1AP, "itti_alloc_new_message Failed\n");
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
  }
  S1AP_E_RAB_REL_RSP(message_p).mme_ue_s1ap_id = ue_ref_p->mme_ue_s1ap_id;
  S1AP_E_RAB_REL_RSP(message_p).enb_ue_s1ap_id = ue_ref_p->enb_ue_s1ap_id;
  S1AP_E_RAB_REL_RSP(message_p).e_rab_rel_list.no_of_items           = 1;
  S1AP_E_RAB_REL_RSP(message_p).e_rab_failed_to_rel_list.no_of_items = 0;

  S1AP_FIND_PROTOCOLIE_BY_ID(
      S1ap_E_RABReleaseResponseIEs_t, ie, container,
      S1ap_ProtocolIE_ID_id_E_RABReleasedList, true);

  const S1ap_E_RABList_t* const e_rab_list = &ie->value.choice.E_RABList;
  int num_erab                             = e_rab_list->list.count;
  if (ie) {
    for (int index = 0; index < num_erab; index++) {
      const S1ap_E_RABItemIEs_t* const erab_item_ies =
          (S1ap_E_RABItemIEs_t*) e_rab_list->list.array[index];
      const S1ap_E_RABItem_t* const erab_item =
          (S1ap_E_RABItem_t*) &erab_item_ies->value.choice.E_RABItem;
      S1AP_E_RAB_REL_RSP(message_p).e_rab_rel_list.item[index].e_rab_id =
          erab_item->e_RAB_ID;
      S1AP_E_RAB_REL_RSP(message_p).e_rab_rel_list.item[index].cause =
          erab_item->cause;
      S1AP_E_RAB_REL_RSP(message_p).e_rab_rel_list.no_of_items++;
    }
  }
  message_p->ittiMsgHeader.imsi = imsi64;
  rc = send_msg_to_task(&s1ap_task_zmq_ctx, TASK_MME_APP, message_p);
  OAILOG_FUNC_RETURN(LOG_S1AP, rc);
}
