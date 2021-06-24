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
#include <sched.h>
#include "log.h"


class ThreadSchedParams {
 public:
  ThreadSchedParams()
      : cpu_id(0), sched_policy(SCHED_FIFO), sched_priority(84) {}
  int cpu_id;
  int sched_policy;
  int sched_priority;
  void Apply(const log_proto_t task_id) const {
    if (cpu_id >= 0) {
      cpu_set_t cpuset;
      CPU_SET(cpu_id, &cpuset);
      if (int rc = pthread_setaffinity_np(
              pthread_self(), sizeof(cpu_set_t), &cpuset)) {
        OAILOG_WARNING(task_id,
            "Could not set affinity to ITTI task err=%d", rc);
      }
    }

    struct sched_param sparam;
    memset(&sparam, 0, sizeof(sparam));
    sparam.sched_priority = sched_priority;
    if (int rc = pthread_setschedparam(pthread_self(), sched_policy, &sparam)) {
      OAILOG_WARNING(task_id,
          "Could not set schedparam to ITTI task, err=%d", rc);
    }
  }
};

