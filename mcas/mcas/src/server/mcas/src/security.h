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

#ifndef __MCAS_SECURITY_H__
#define __MCAS_SECURITY_H__

#include <api/crypto_itf.h>
#include <common/logging.h>
#include <future>
#include <memory>
#include <string>
#include "config_file.h"

namespace mcas
{
class Shard_security_state;

class Shard_security : private common::log_source {

private:
  
  enum class security_mode_t
    {
     NONE,
     TLS_AUTH,
     TLS_HMAC,
    };
  
public:
  Shard_security(const boost::optional<std::string> cert_path,
                 const boost::optional<std::string> key_path,
                 const boost::optional<std::string> mode,
                 const boost::optional<std::string> ipaddr,
                 const boost::optional<std::string> net_device,
                 const unsigned port,
                 const unsigned debug_level);

  virtual ~Shard_security();

  inline bool auth_enabled() const { return _auth_enabled; }
  inline security_mode_t security_mode() const { return _mode; }
  inline const std::string& ipaddr() const { return _ipaddr; }
  inline const std::string& cert_path() const { return _cert_path; }
  inline const std::string& key_path() const { return _key_path; }
  inline unsigned port() const { return _port; }

private:
  const std::string   _cert_path;
  const std::string   _key_path;
  bool                _auth_enabled;
  security_mode_t     _mode;
  std::string         _ipaddr;
  unsigned            _port;
};


}  // namespace mcas

#endif  // __MCAS_SECURITY_H__
