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

#pragma once

#include "../../lib/gtpv2c/gtpv2c.h"
extern "C" {
#include "intertask_interface.h"
}
#include "EndPoint.h"
#include <thread>

namespace magma {
namespace lte {

class S11 : public Gtpv2cStack {
 private:
  std::thread::id thread_id;
  std::thread thread;

  void HandleReceiveGtpv2cMsg(
      gtpv2c_msg& msg, const EndPoint& remote_endpoint);
  void HandleReceive_createSessionResponse(
      gtpv2c_msg& msg, const EndPoint& remote_endpoint);
  void HandleReceiveDeleteSessionResponse(
      gtpv2c_msg& msg, const EndPoint& remote_endpoint);
  void HandleReceiveModifyBearerResponse(
      gtpv2c_msg& msg, const EndPoint& remote_endpoint);
  void HandleReceiveReleaseAccessBearersResponse(
      gtpv2c_msg& msg, const EndPoint& remote_endpoint);
  void HandleReceiveDownlinkDataNotification(
      gtpv2c_msg& msg, const EndPoint& remote_endpoint);

 public:
  S11();
  S11(S11 const&) = delete;
  void operator=(S11 const&) = delete;

  void HandleReceive(
      char* recv_buffer, const std::size_t bytes_transferred,
      const EndPoint& remote_endpoint);
  void NotifyUlError(
      const EndPoint& r_endpoint, const teid_t l_teid,
      const cause_value_e cause, const uint64_t gtpc_tx_id);

  void SendMsg(itti_s11_create_session_request_t& m);
  void SendMsg(itti_s11_delete_session_request_t& m);
  void SendMsg(itti_s11_modify_bearer_request_t& m);
  void SendMsg(itti_s11_release_access_bearers_request_t& m);
  void SendMsg(itti_s11_downlink_data_notification_acknowledge_t& m);

  void TimeOutIttiEvent(const uint32_t timer_id);
};
}  // namespace lte
}  // namespace magma
