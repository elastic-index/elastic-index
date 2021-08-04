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

#include "channel_wrap.h"

#include "uipc.h"

void Channel_wrap::open(const std::string &s)
{
  _ch = ::uipc_connect_channel(s.c_str());
}

void Channel_wrap::create(const std::string &s, size_t message_size, size_t queue_size)
{
  _ch = ::uipc_create_channel(s.c_str(), message_size, queue_size);
}

void Channel_wrap::close()
{
  if ( _ch )
  {
    ::uipc_close_channel(_ch);
  }
}
