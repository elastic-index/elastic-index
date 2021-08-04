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
#include "pingpong_server.h"

#include "eyecatcher.h"
#include "pingpong_cb_pack.h"
#include "pingpong_server_cb.h"
#include "pingpong_server_client_state.h"
#include <exception>
#include <cstdint> /* uint64_t */
#include <chrono>
#include <functional> /* ref */
#include <iostream> /* cerr */

namespace component
{
  class IFabric_server_factory;
}

void pingpong_server::listener(
  component::IFabric_server_factory &factory_
  , std::size_t buffer_size_
  , std::uint64_t remote_key_base_
  , unsigned iteration_count_
  , std::size_t msg_size_
)
try
{
  client_state c{
    factory_
    , iteration_count_
    , msg_size_
  };

  /* executes a poll_recv per receive buffer, establishing the callbacks */
  cb_pack pack{
    c.st
    , c.st.comm()
    , pingpong_server_cb::send_cb
    , pingpong_server_cb::recv_cb
    , buffer_size_
    , remote_key_base_
    , msg_size_
  };

  auto &st = c.st;

  std::uint64_t poll_count = 0U;
  auto polled_any = true;
  while ( polled_any )
  {
    polled_any = false;

    if ( ! st.done )
    {
      if ( _stat.start() == std::chrono::high_resolution_clock::time_point::min() )
      {
        _stat.do_start();
      }
      st.comm().poll_completions(cb_ctxt::cb);
      ++poll_count;
      polled_any = true;
    }
  }

  _stat.do_stop(poll_count);
}
catch ( std::exception &e )
{
  std::cerr << "pingpong_server::" << __func__ << ": " << e.what() << "\n";
  throw;
}

pingpong_server::pingpong_server(
  component::IFabric_server_factory &factory_
  , std::uint64_t buffer_size_
  , std::uint64_t remote_key_base_
  , unsigned iteration_count_
  , std::uint64_t msg_size_
)
  : _stat{}
  , _th(
      std::async(
        std::launch::async
        , &pingpong_server::listener
        , this
        , std::ref(factory_), buffer_size_, remote_key_base_, iteration_count_, msg_size_
      )
    )
{
}

pingpong_stat pingpong_server::time()
{
  if ( _th.valid() )
  {
    _th.get();
  }
  return _stat;
}

pingpong_server::~pingpong_server()
{
  if ( _th.valid() )
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
}
