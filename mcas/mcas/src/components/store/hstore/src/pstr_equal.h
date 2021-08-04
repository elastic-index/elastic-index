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

#ifndef _MCAS_PSTR_EQUAL_H_
#define _MCAS_PSTR_EQUAL_H_

#include <common/string_view.h>
#include <cstring>

template <typename Key>
  struct pstr_equal
  {
    using argument_type = Key;
    using result_type = bool;
    result_type operator()(const argument_type &a, const common::string_view b) const
    {
      return a.size() == b.size() && 0 == std::memcmp(a.data(), b.data(), a.size());
    }
  };

#endif
