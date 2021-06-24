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

#include "conversions2.h"

#include <arpa/inet.h>
#include <inttypes.h>
#include <string.h>
#include <sys/socket.h>

class EndPoint {
 public:
  struct sockaddr_storage addr_storage;
  socklen_t addr_storage_len;
  EndPoint()
      : addr_storage(), addr_storage_len(sizeof(struct sockaddr_storage)){};
  EndPoint(const EndPoint& e)
      : addr_storage(e.addr_storage), addr_storage_len(e.addr_storage_len){};
  EndPoint(const struct sockaddr_storage& addr, const socklen_t len)
      : addr_storage(addr), addr_storage_len(len){};
  EndPoint(const struct in_addr& addr, const uint16_t port) {
    struct sockaddr_in* addr_in = (struct sockaddr_in*) &addr_storage;
    addr_in->sin_family         = AF_INET;
    addr_in->sin_port           = htons(port);
    addr_in->sin_addr.s_addr    = addr.s_addr;

    addr_storage_len = sizeof(struct sockaddr_in);
  };

  EndPoint(const struct in6_addr& addr6, const uint16_t port) {
    struct sockaddr_in6* addr_in6 = (struct sockaddr_in6*) &addr_storage;
    addr_in6->sin6_family         = AF_INET6;
    addr_in6->sin6_port           = htons(port);
    addr_in6->sin6_flowinfo       = 0;
    memcpy(&addr_in6->sin6_addr, &addr6, sizeof(struct in6_addr));
    addr_in6->sin6_scope_id = 0;

    addr_storage_len = sizeof(struct sockaddr_in6);
  };

  uint16_t Port() const {
    return ntohs(((struct sockaddr_in*) &addr_storage)->sin_port);
  }

  void SetPort(uint16_t port) const {
    ((struct sockaddr_in*) &addr_storage)->sin_port = htons(port);
  }

  sa_family_t Family() const { return addr_storage.ss_family; }

  bool operator==(const EndPoint& e) const {
    if ((addr_storage_len == e.addr_storage_len) &&
        (memcmp(
             (const void*) &addr_storage, (const void*) &e.addr_storage,
             addr_storage_len) == 0)) {
      return true;
    } else {
      return false;
    }
  };

  std::string toString() const {
    std::string str;
    if (addr_storage.ss_family == AF_INET) {
      struct sockaddr_in* addr_in = (struct sockaddr_in*) &addr_storage;
      str.append(Conv::toString(addr_in->sin_addr));
      str.append(":").append(std::to_string(ntohs(addr_in->sin_port)));
    } else if (addr_storage.ss_family == AF_INET6) {
      struct sockaddr_in6* addr_in6 = (struct sockaddr_in6*) &addr_storage;
      str.append(Conv::toString(addr_in6->sin6_addr));
      str.append(":").append(std::to_string(ntohs(addr_in6->sin6_port)));
    }
    return str;
  }
};
