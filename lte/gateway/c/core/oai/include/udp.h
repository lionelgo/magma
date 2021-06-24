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

#include <string>
#include <system_error>
#include <thread>
#include <utility>
#include <iostream>

#ifdef __cplusplus
extern "C" {
#endif
#include "intertask_interface.h"
#ifdef __cplusplus
}
#endif

#include "conversions2.h"
#include "EndPoint.h"
#include "thread_sched.hpp"

#include <arpa/inet.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <stdint.h>

class UdpApplication {
 public:
  virtual void HandleReceive(
      char* recv_buffer, const std::size_t bytes_transferred,
      const EndPoint& r_endpoint);
  virtual void StartReceive(
      UdpApplication* gtp_stack, const ThreadSchedParams& sched_params);
};

class UdpServer {
 public:
  UdpServer(const struct in_addr& address, const uint16_t port_num)
      : app_(nullptr), port_(port_num) {
    socket_ = CreateSocket(address, port_);
    if (socket_ > 0) {
      OAILOG_WARNING(
          LOG_UDP, "UdpServer::UdpServer(%s:%d)",
          Conv::toString(address).c_str(), port_);
      sa_family = AF_INET;
    } else {
      OAILOG_ERROR(
          LOG_UDP, "UdpServer::UdpServer(%s:%d)",
          Conv::toString(address).c_str(), port_);
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      throw std::system_error(
          socket_, std::generic_category(),
          "UdpServer socket creation failed!");
    }
  }

  UdpServer(const struct in6_addr& address, const uint16_t port_num)
      : app_(nullptr), port_(port_num) {
    socket_ = CreateSocket(address, port_);
    if (socket_ > 0) {
      OAILOG_DEBUG(
          LOG_UDP, "UdpServer::UdpServer(%s:%d)",
          Conv::toString(address).c_str(), port_);
      sa_family = AF_INET6;
    } else {
      OAILOG_ERROR(
          LOG_UDP, "UdpServer::UdpServer(%s:%d)",
          Conv::toString(address).c_str(), port_);
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      throw std::system_error(
          socket_, std::generic_category(),
          "UdpServer socket creation failed!");
    }
  }

  UdpServer(const char* address, const uint16_t port_num)
      : app_(nullptr), port_(port_num) {
    socket_ = CreateSocket(address, port_);
    if (socket_ > 0) {
      OAILOG_DEBUG(LOG_UDP, "UdpServer::UdpServer(%s:%d)", address, port_);
    } else {
      OAILOG_ERROR(LOG_UDP, "UdpServer::UdpServer(%s:%d)", address, port_);
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      throw std::system_error(
          socket_, std::generic_category(),
          "UdpServer socket creation failed!");
    }
  }

  ~UdpServer() { close(socket_); }

  uint16_t GetPort() const { return port_; }

  void UdpReadLoop(const ThreadSchedParams& sched_params);

  void AsyncSendTo(
      const char* send_buffer, const ssize_t num_bytes,
      const EndPoint& r_endpoint) {
    ssize_t bytes_written = sendto(
        socket_, send_buffer, num_bytes, 0,
        (struct sockaddr*) &r_endpoint.addr_storage,
        r_endpoint.addr_storage_len);
    if (bytes_written != num_bytes) {
      OAILOG_ERROR(LOG_UDP, "sendto failed(%d:%s)\n", errno, strerror(errno));
    }
  }

  void AsyncSendTo(
      const char* send_buffer, const ssize_t num_bytes,
      const struct sockaddr_in& r_endpoint) {
    ssize_t bytes_written = sendto(
        socket_, send_buffer, num_bytes, 0, (struct sockaddr*) &r_endpoint,
        sizeof(struct sockaddr_in));
    if (bytes_written != num_bytes) {
      OAILOG_ERROR(LOG_UDP, "sendto failed(%d:%s)\n", errno, strerror(errno));
    }
  }

  void AsyncSendTo(
      const char* send_buffer, const ssize_t num_bytes,
      const struct sockaddr_in6& r_endpoint) {
    ssize_t bytes_written = sendto(
        socket_, send_buffer, num_bytes, 0, (struct sockaddr*) &r_endpoint,
        sizeof(struct sockaddr_in6));
    if (bytes_written != num_bytes) {
      OAILOG_ERROR(LOG_UDP, "sendto failed(%d:%s)\n", errno, strerror(errno));
    }
  }

  void StartReceive(
      UdpApplication* gtp_stack, const ThreadSchedParams& sched_params);

 protected:
  int CreateSocket(const struct in_addr& address, const uint16_t port);
  int CreateSocket(const struct in6_addr& address, const uint16_t port);
  int CreateSocket(const char* address, const uint16_t port_num);

  static void HandleSend(
      const char*, /*buffer*/
      const int& /*error*/, std::size_t /*bytes_transferred*/) {}

  UdpApplication* app_;
  std::thread thread_;
  int socket_;
  uint16_t port_;
  sa_family_t sa_family;
#define UDP_RECV_BUFFER_SIZE 8192
  char recv_buffer_[UDP_RECV_BUFFER_SIZE];
};
