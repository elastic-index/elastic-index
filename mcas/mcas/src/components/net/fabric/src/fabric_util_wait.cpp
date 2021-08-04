/*
   Copyright [2017-2020] [IBM Corporation]
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
       http://www.apache.org/licenses/LICENSE-2.0
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/


/*
 * Authors:
 *
 */

#include "fabric_util_wait.h"

#include "fabric_check.h" /* CHECK_FI_ERR */

#include "rdma-fi_domain.h" /* fid_wait, fi_wait_open */

/**
 * Fabric/RDMA-based network component
 *
 */

fid_unique_ptr<::fid_wait> make_fid_wait(::fid_fabric &fabric, ::fi_wait_attr &attr)
{
  ::fid_wait *wait_set;
  CHECK_FI_ERR(::fi_wait_open(&fabric, &attr, &wait_set));
  return fid_unique_ptr<::fid_wait>(wait_set);
}
