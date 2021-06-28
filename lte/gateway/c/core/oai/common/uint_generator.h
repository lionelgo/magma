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

#include <mutex>
#include <set>

template<class UINT>
class UintGenerator {
 private:
  UINT uid_generator_;
  std::mutex m_uid_generator_;

  std::set<UINT> uid_generated_;
  std::mutex m_uid_generated_;

 public:
  UintGenerator() : m_uid_generator_(), m_uid_generated_() {
    uid_generator_ = 0;
    uid_generated_ = {};
  };

  UintGenerator(UintGenerator const&) = delete;
  void operator=(UintGenerator const&) = delete;

  UINT GetUid() {
    std::unique_lock<std::mutex> lr(m_uid_generator_);
    UINT uid = ++uid_generator_;
    while (true) {
      // may happen race conditions here
      std::unique_lock<std::mutex> ld(m_uid_generated_);
      if (uid_generated_.count(uid) == 0) {
        uid_generated_.insert(uid);
        ld.unlock();
        lr.unlock();
        return uid;
      }
      uid = ++uid_generator_;
    }
  }

  void FreeUid(UINT uid) {
    std::unique_lock<std::mutex> l(m_uid_generated_);
    uid_generated_.erase(uid);
    l.unlock();
  }
};

template<class UINT>
class UintUidGenerator {
 private:
  UINT uid_generator_;
  std::mutex m_uid_generator_;

  std::set<UINT> uid_generated_;
  std::mutex m_uid_generated_;

  UintUidGenerator() : m_uid_generator_(), m_uid_generated_() {
    uid_generator_ = 0;
    uid_generated_ = {};
  };

 public:
  static UintUidGenerator& GetInstance() {
    static UintUidGenerator instance;
    return instance;
  }

  UintUidGenerator(UintUidGenerator const&) = delete;
  void operator=(UintUidGenerator const&) = delete;

  UINT GetUid() {
    std::unique_lock<std::mutex> lr(m_uid_generator_);
    UINT uid = ++uid_generator_;
    while (true) {
      // may happen race conditions here
      std::unique_lock<std::mutex> ld(m_uid_generated_);
      if (uid_generated_.count(uid) == 0) {
        uid_generated_.insert(uid);
        lr.unlock();
        ld.unlock();
        return uid;
      }
      uid = ++uid_generator_;
    }
  }

  void FreeUid(UINT uid) {
    std::unique_lock<std::mutex> l(m_uid_generated_);
    uid_generated_.erase(uid);
    l.unlock();
  }
};
