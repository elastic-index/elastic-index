/*
   Copyright [2017-2019] [IBM Corporation]
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


#ifndef _FABRIC_UTIL_WAIT_H_
#define _FABRIC_UTIL_WAIT_H_

/*
 * Authors:
 *
 */

#include "fabric_ptr.h" /* fid_unique_ptr */

struct fi_wait_attr;

struct fid_fabric;
struct fid_wait;

/**
 * Fabric/RDMA-based network component
 *
 */

/**
 *
 * @throw fabric_runtime_error : std::runtime_error : ::fi_wait_open fail
 */
fid_unique_ptr<::fid_wait> make_fid_wait(::fid_fabric &fabric, ::fi_wait_attr &attr);

#endif
