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
#include "remote_memory_server_grouped.h"

#include "eyecatcher.h"
#include "registered_memory.h"
#include "server_grouped_connection.h"
#include "wait_poll.h"
#include <api/fabric_itf.h> /* IFabric, IFabric_server_grouped_factory */
#include <common/errors.h> /* S_OK */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-compare"
#include <gtest/gtest.h>
#pragma GCC diagnostic pop

#include <sys/uio.h> /* iovec */
#include <exception>
#include <functional> /* ref */
#include <iostream> /* cerr */
#include <string>
#include <memory> /* make_shared, shared_ptr */
#include <thread>
#include <vector>

void remote_memory_server_grouped::listener(
  component::IFabric_server_grouped_factory &ep_
  , std::size_t memory_size_
  , std::uint64_t remote_key_index_
)
{
  auto quit = false;
  for ( ; ! quit ; ++remote_key_index_ )
  {
    /* Get a client to work with */
    server_grouped_connection sc(ep_);
    /* register an RDMA memory region */
    registered_memory rm{sc.cnxn(), memory_size_, remote_key_index_};
    /* send the client address and key to memory */
    auto &cnxn = sc.comm();
    EXPECT_EQ(sc.cnxn().max_message_size(), this->max_message_size());
    send_memory_info(cnxn, rm);
    /* wait for client indicate exit (by sending one byte to us) */
    try
    {
      std::vector<::iovec> v;
      ::iovec iv;
      iv.iov_base = &rm[0];
      iv.iov_len = 1;
      v.emplace_back(iv);
      cnxn.post_recv(v, this);
      ::wait_poll(
        cnxn
        , [&quit, &rm, this] (void *ctxt_, ::status_t stat_, std::uint64_t, std::size_t len_, void *) -> void
          {
            ASSERT_EQ(ctxt_, this);
            ASSERT_EQ(stat_, S_OK);
            ASSERT_EQ(len_, 1);
            /* did client leave with the "quit byte" set to 'q'? */
            quit |= rm[0] == 'q';
          }
      );
    }
    catch ( std::exception &e )
    {
      std::cerr << "remote_memory_server_grouped::" << __func__ << ": " << e.what() << "\n";
      throw;
    }
  }
}

remote_memory_server_grouped::remote_memory_server_grouped(
  component::IFabric &fabric_
  , const std::string &fabric_spec_
  , std::uint16_t control_port_
  , std::size_t memory_size_
  , std::uint64_t remote_key_base_
)
  : _ep(fabric_.open_server_grouped_factory(fabric_spec_, control_port_))
  , _th(
    std::async(
      std::launch::async
      , &remote_memory_server_grouped::listener
      , this
      , std::ref(*_ep)
      , memory_size_
      , remote_key_base_
    )
  )
{}

remote_memory_server_grouped::~remote_memory_server_grouped()
{
  try
  {
    _th.get();
  }
  catch ( std::exception &e )
  {
    std::cerr << __func__ << " exception " << e.what() << eyecatcher << std::endl;
  }
}

std::size_t remote_memory_server_grouped::max_message_size() const
{
  return _ep->max_message_size();
}
