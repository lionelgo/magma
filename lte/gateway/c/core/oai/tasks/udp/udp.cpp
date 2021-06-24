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

#include "udp.h"
#include "log.h"

#include <cstdlib>

//------------------------------------------------------------------------------
void UdpApplication::HandleReceive(
    char* recv_buffer, const std::size_t bytes_transferred,
    const EndPoint& r_endpoint) {
  OAILOG_WARNING(
      LOG_UDP, "Missing implementation of interface udp_application\n");
}
//------------------------------------------------------------------------------
void UdpApplication::StartReceive(
    UdpApplication* gtp_stack, const ThreadSchedParams& sched_params) {
  OAILOG_WARNING(
      LOG_UDP, "Missing implementation of interface udp_application\n");
}
//------------------------------------------------------------------------------
void UdpServer::UdpReadLoop(const ThreadSchedParams& sched_params) {
  EndPoint r_endpoint   = {};
  size_t bytes_received = 0;

  sched_params.Apply(LOG_UDP);

  while (1) {
    r_endpoint.addr_storage_len = sizeof(struct sockaddr_storage);
    if ((bytes_received = recvfrom(
             socket_, recv_buffer_, UDP_RECV_BUFFER_SIZE, 0,
             (struct sockaddr*) &r_endpoint.addr_storage,
             &r_endpoint.addr_storage_len)) > 0) {
      app_->HandleReceive(recv_buffer_, bytes_received, r_endpoint);
    } else {
      OAILOG_ERROR(LOG_UDP, "Recvfrom failed %s\n", strerror(errno));
    }
  }
}
//------------------------------------------------------------------------------
int UdpServer::CreateSocket(
    const struct in_addr& address, const uint16_t port) {
  struct sockaddr_in addr = {};
  int sd                  = 0;

  /*
   * Create UDP socket
   */
  if ((sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    /*
     * Socket creation has failed...
     */
    OAILOG_ERROR(LOG_UDP, "Socket creation failed (%s)\n", strerror(errno));
    return errno;
  }

  addr.sin_family      = AF_INET;
  addr.sin_port        = htons(port);
  addr.sin_addr.s_addr = address.s_addr;

  std::string ipv4 = Conv::toString(address);
  OAILOG_DEBUG(
      LOG_UDP,
      "Creating new listen socket on address %s and port %" PRIu16 "\n",
      ipv4.c_str(), port);

  if (bind(sd, (struct sockaddr*) &addr, sizeof(struct sockaddr_in)) < 0) {
    /*
     * Bind failed
     */
    OAILOG_ERROR(
        LOG_UDP,
        "Socket bind failed (%s) for address %s and port %" PRIu16 "\n",
        strerror(errno), ipv4.c_str(), port);
    close(sd);
    return errno;
  }
  sa_family = AF_INET;
  return sd;
}
//------------------------------------------------------------------------------
int UdpServer::CreateSocket(
    const struct in6_addr& address, const uint16_t port) {
  struct sockaddr_in6 addr = {};
  int sd                   = 0;

  /*
   * Create UDP socket
   */
  if ((sd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    /*
     * Socket creation has failed...
     */
    OAILOG_ERROR(LOG_UDP, "Socket creation failed (%s)\n", strerror(errno));
    return errno;
  }

  addr.sin6_family = AF_INET6;
  addr.sin6_port   = htons(port);
  addr.sin6_addr   = address;

  std::string ipv6 = Conv::toString(address);
  OAILOG_DEBUG(
      LOG_UDP,
      "Creating new listen socket on address %s and port %" PRIu16 "\n",
      ipv6.c_str(), port);

  if (bind(sd, (struct sockaddr*) &addr, sizeof(struct sockaddr_in6)) < 0) {
    /*
     * Bind failed
     */
    OAILOG_ERROR(
        LOG_UDP,
        "Socket bind failed (%s) for address %s and port %" PRIu16 "\n",
        strerror(errno), ipv6.c_str(), port);
    close(sd);
    return errno;
  }
  sa_family = AF_INET6;
  return sd;
}
//------------------------------------------------------------------------------
int UdpServer::CreateSocket(const char* address, const uint16_t port_num) {
  unsigned char buf_in_addr[sizeof(struct in6_addr)];
  if (inet_pton(AF_INET, address, buf_in_addr) == 1) {
    struct in_addr addr4 = {};
    memcpy(&addr4, buf_in_addr, sizeof(struct in_addr));
    return CreateSocket(addr4, port_num);
  } else if (inet_pton(AF_INET6, address, buf_in_addr) == 1) {
    struct in6_addr addr6 = {};
    memcpy(&addr6, buf_in_addr, sizeof(struct in6_addr));
    return CreateSocket(addr6, port_num);
  } else {
    OAILOG_ERROR(LOG_UDP, "udp_server::CreateSocket(%s:%d)", address, port_num);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    throw std::system_error(
        socket_, std::generic_category(), "UDP socket creation failed!");
  }
}
//------------------------------------------------------------------------------
void UdpServer::StartReceive(
    UdpApplication* app, const ThreadSchedParams& sched_params) {
  app_ = app;
  OAILOG_TRACE(LOG_UDP, "udp_server::start_receive");
  thread_ = std::thread(&UdpServer::UdpReadLoop, this, sched_params);
  thread_.detach();
}
