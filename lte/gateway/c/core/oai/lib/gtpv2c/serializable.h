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

#ifndef FILE_SERIALIZABLE_HPP_SEEN
#define FILE_SERIALIZABLE_HPP_SEEN

#include <iostream>
#include <string>

class stream_serializable {
 public:
  virtual void dump_to(std::ostream& os)   = 0;
  virtual void load_from(std::istream& is) = 0;
  // virtual ~serializable() = 0;
};

#endif /* FILE_SERIALIZABLE_HPP_SEEN */
