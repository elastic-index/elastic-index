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
#ifndef _TEST_PINGPONG_SERVER_CB_H_
#define _TEST_PINGPONG_SERVER_CB_H_

#include <common/types.h> /* status_t */
#include <cstddef> /* size_t */

struct cb_ctxt;

namespace pingpong_server_cb
{
  void recv_cb(cb_ctxt *rx_ctxt_, ::status_t stat_, std::size_t len_);
  void send_cb(cb_ctxt *tx_ctxt_, ::status_t stat_, std::size_t len_);
}

#endif
